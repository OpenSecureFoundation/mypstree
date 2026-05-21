#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include "proc_reader.h"

static int is_all_digits(const char *str) {
    if (!str || *str == '\0') return 0;
    while (*str) {
        if (!isdigit((unsigned char)*str)) return 0;
        str++;
    }
    return 1;
}

static int parse_status_file(int pid, ProcessInfo *proc) {
    char path[64];
    char line[256];
    FILE *fp;
    int got_name = 0, got_pid = 0, got_ppid = 0, got_tgid = 0;

    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    fp = fopen(path, "r");
    if (!fp) return -1; /* Géré proprement : si le processus disparaît, on ignore */

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = '\0';

        if (strncmp(line, "Name:\t", 6) == 0) {
            strncpy(proc->name, line + 6, MAX_NAME_LEN - 1);
            proc->name[MAX_NAME_LEN - 1] = '\0';
            got_name = 1;
        } else if (strncmp(line, "Pid:\t", 5) == 0) {
            proc->pid = atoi(line + 5);
            got_pid = 1;
        } else if (strncmp(line, "PPid:\t", 6) == 0) {
            proc->ppid = atoi(line + 6);
            got_ppid = 1;
        } else if (strncmp(line, "Tgid:\t", 6) == 0) {
            proc->tgid = atoi(line + 6);
            got_tgid = 1;
        }
    }
    fclose(fp);

    /* Récupération du PGID via stat pour l'option -g */
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    fp = fopen(path, "r");
    if (fp) {
        int unused_pid;
        char unused_comm[256];
        char unused_state;
        int unused_ppid;
        int pgid = 0;
        if (fscanf(fp, "%d %s %c %d %d", &unused_pid, unused_comm, &unused_state, &unused_ppid, &pgid) == 5) {
            proc->pgid = pgid;
        }
        fclose(fp);
    }

    return (got_name && got_pid && got_ppid) ? 0 : -1;
}

static void parse_cmdline_file(int pid, char *cmdline_buf, int buf_size) {
    char path[64];
    FILE *fp;
    int i, len;

    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    fp = fopen(path, "r");
    if (!fp) {
        cmdline_buf[0] = '\0';
        return;
    }

    len = fread(cmdline_buf, 1, buf_size - 1, fp);
    fclose(fp);

    if (len <= 0) {
        cmdline_buf[0] = '\0';
        return;
    }

    cmdline_buf[len] = '\0';

    /* Remplacement des octets nuls par des espaces pour la gestion de l'option -a */
    for (i = 0; i < len - 1; i++) {
        if (cmdline_buf[i] == '\0') cmdline_buf[i] = ' ';
    }
}

int read_all_processes(ProcessInfo *procs, int max_procs, int hide_threads) {
    DIR *proc_dir;
    struct dirent *entry;
    int count = 0;

    proc_dir = opendir("/proc");
    if (!proc_dir) {
        perror("opendir /proc");
        return -1;
    }

    while ((entry = readdir(proc_dir)) != NULL && count < max_procs) {
        if (!is_all_digits(entry->d_name)) continue;

        int pid = atoi(entry->d_name);
        if (pid <= 0) continue;

        ProcessInfo *p = &procs[count];
        memset(p, 0, sizeof(ProcessInfo));

        if (parse_status_file(pid, p) != 0) {
            continue;
        }

        /* Option -T: Ignorer les processus qui sont des threads */
        if (hide_threads && (p->pid != p->tgid)) {
            continue;
        }

        parse_cmdline_file(pid, p->cmdline, MAX_NAME_LEN);

        count++;
    }

    closedir(proc_dir);
    return count;
}
