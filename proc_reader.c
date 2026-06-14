#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>
#include "proc_reader.h"

static int is_all_digits(const char *str) {
    if (!str || *str == '\0') return 0;
    while (*str) {
        if (!isdigit((unsigned char)*str)) return 0;
        str++;
    }
    return 1;
}

static void parse_stat_file(int pid, int is_task, int tgid, ProcessInfo *proc) {
    char path[128];
    if (is_task) snprintf(path, sizeof(path), "/proc/%d/task/%d/stat", tgid, pid);
    else snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    
    FILE *fp = fopen(path, "r");
    if (fp) {
        char line[1024];
        if (fgets(line, sizeof(line), fp)) {
            char *p = strrchr(line, ')'); // Ignore le nom de processus qui peut contenir des espaces
            if (p) {
                p += 2;
                int field = 3;
                while (*p && field < 22) {
                    if (*p == ' ') field++;
                    p++;
                }
                if (*p) proc->starttime = strtoull(p, NULL, 10);
            }
        }
        fclose(fp);
    }
}

static int parse_status_file(int pid, int is_task, int tgid, ProcessInfo *proc) {
    char path[128];
    if (is_task) snprintf(path, sizeof(path), "/proc/%d/task/%d/status", tgid, pid);
    else snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    int got_name = 0, got_pid = 0;
    char line[256];
    
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = '\0';
        if (strncmp(line, "Name:\t", 6) == 0) {
            strncpy(proc->name, line + 6, MAX_NAME_LEN - 1);
            got_name = 1;
        } else if (strncmp(line, "Pid:\t", 5) == 0) {
            proc->pid = atoi(line + 5);
            got_pid = 1;
        } else if (!is_task && strncmp(line, "PPid:\t", 6) == 0) {
            proc->ppid = atoi(line + 6);
        } else if (!is_task && strncmp(line, "Tgid:\t", 6) == 0) {
            proc->tgid = atoi(line + 6);
        }
    }
    fclose(fp);
    
    if (is_task) {
        proc->ppid = tgid; // Le parent logique d'un thread est son tgid
        proc->is_thread = 1;
    }

    // Lecture Namespace pour -S et -N
    char ns_path[128];
    struct stat st;
    if (is_task) snprintf(ns_path, sizeof(ns_path), "/proc/%d/task/%d/ns/net", tgid, pid);
    else snprintf(ns_path, sizeof(ns_path), "/proc/%d/ns/net", pid);
    if (stat(ns_path, &st) == 0) proc->ns_net = st.st_ino;
    // --- mypstree -Z --- //
    char attr_path[128];
    if (is_task) {
        snprintf(attr_path, sizeof(attr_path), "/proc/%d/task/%d/attr/current", tgid, pid);
    } else {
        snprintf(attr_path, sizeof(attr_path), "/proc/%d/attr/current", pid);
    }

    FILE *fattr = fopen(attr_path, "r");
    if (fattr) {
        if (fgets(proc->selinux_context, sizeof(proc->selinux_context), fattr)) {
            // On retire le saut de ligne (\n) s'il y en a un
            proc->selinux_context[strcspn(proc->selinux_context, "\n")] = '\0';
        } else {
            strncpy(proc->selinux_context, "unconfined", sizeof(proc->selinux_context));
        }
        fclose(fattr);
    } else {
        // Si SELinux est désactivé sur la machine ou inaccessible
        strncpy(proc->selinux_context, "n/a", sizeof(proc->selinux_context));
    }
    
    return (got_name && got_pid) ? 0 : -1;
}

static void parse_cmdline_file(int pid, char *cmdline_buf, int buf_size) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    FILE *fp = fopen(path, "r");
    if (!fp) {
        cmdline_buf[0] = '\0';
        return;
    }
    int len = fread(cmdline_buf, 1, buf_size - 1, fp);
    fclose(fp);
    if (len <= 0) {
        cmdline_buf[0] = '\0';
        return;
    }
    cmdline_buf[len] = '\0';
    for (int i = 0; i < len - 1; i++) {
        if (cmdline_buf[i] == '\0') cmdline_buf[i] = ' ';
    }
}

int read_all_processes(ProcessInfo *procs, int max_procs, int hide_threads) {
    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) return -1;
    
    int count = 0;
    struct dirent *entry;

    while ((entry = readdir(proc_dir)) != NULL && count < max_procs) {
        if (!is_all_digits(entry->d_name)) continue;
        int pid = atoi(entry->d_name);
        
        ProcessInfo *p = &procs[count];
        memset(p, 0, sizeof(ProcessInfo));
        
        if (parse_status_file(pid, 0, 0, p) != 0) continue;
        parse_cmdline_file(pid, p->cmdline, MAX_NAME_LEN);
        parse_stat_file(pid, 0, 0, p);
        count++;

        // Extraction des sous-threads
        if (!hide_threads) {
            char task_path[128];
            snprintf(task_path, sizeof(task_path), "/proc/%d/task", pid);
            DIR *task_dir = opendir(task_path);
            if (task_dir) {
                struct dirent *t_entry;
                while ((t_entry = readdir(task_dir)) != NULL && count < max_procs) {
                    if (!is_all_digits(t_entry->d_name)) continue;
                    int tid = atoi(t_entry->d_name);
                    if (tid == pid) continue; // Thread principal deja enregistre
                    
                    ProcessInfo *t = &procs[count];
                    memset(t, 0, sizeof(ProcessInfo));
                    if (parse_status_file(tid, 1, pid, t) == 0) {
                        parse_stat_file(tid, 1, pid, t);
                        count++;
                    }
                }
                closedir(task_dir);
            }
        }
    }
    closedir(proc_dir);
    return count;
}