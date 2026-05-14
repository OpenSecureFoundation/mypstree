/* proc_reader.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>    /* opendir, readdir */
#include <ctype.h>     /* isdigit */
#include "proc_reader.h"

/*
 * Returns 1 if every character in str is a digit, 0 otherwise.
 * Used to check if a /proc entry is a PID directory.
 */
static int is_all_digits(const char *str) {
    if (!str || *str == '\0') return 0;
    while (*str) {
        if (!isdigit((unsigned char)*str)) return 0;
        str++;
    }
    return 1;
}

/*
 * Parse /proc/[pid]/status to extract Name, Pid, and PPid.
 * Returns 0 on success, -1 if the file cannot be read.
 */
static int parse_status_file(int pid, ProcessInfo *proc) {
    char path[64];
    char line[256];
    FILE *fp;
    int got_name = 0, got_pid = 0, got_ppid = 0;

    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    fp = fopen(path, "r");
    if (!fp) return -1;  /* Process may have died */

    while (fgets(line, sizeof(line), fp)) {
        /* Remove trailing newline */
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
        }

        /* Stop early once we have what we need */
        if (got_name && got_pid && got_ppid) break;
    }

    fclose(fp);
    return (got_name && got_pid && got_ppid) ? 0 : -1;
}

/*
 * Read /proc/[pid]/cmdline for the full command line.
 * cmdline arguments are separated by null bytes — we replace them with spaces.
 */
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

    /* Replace null bytes (argument separators) with spaces */
    for (i = 0; i < len - 1; i++) {
        if (cmdline_buf[i] == '\0') cmdline_buf[i] = ' ';
    }
}

/*
 * Main function: scan all of /proc and fill the procs[] array.
 */
int read_all_processes(ProcessInfo *procs, int max_procs) {
    DIR *proc_dir;
    struct dirent *entry;
    int count = 0;

    proc_dir = opendir("/proc");
    if (!proc_dir) {
        perror("opendir /proc");
        return -1;
    }

    while ((entry = readdir(proc_dir)) != NULL && count < max_procs) {
        /* Skip non-PID entries like "self", "net", etc. */
        if (!is_all_digits(entry->d_name)) continue;

        int pid = atoi(entry->d_name);
        if (pid <= 0) continue;

        ProcessInfo *p = &procs[count];
        memset(p, 0, sizeof(ProcessInfo));

        /* Try to read the status file */
        if (parse_status_file(pid, p) != 0) {
            /* Process vanished between readdir() and fopen() — skip it */
            continue;
        }

        /* Also read cmdline for the -a option */
        parse_cmdline_file(pid, p->cmdline, MAX_NAME_LEN);

        count++;
    }

    closedir(proc_dir);
    return count;
}
