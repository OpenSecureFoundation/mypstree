#ifndef PROC_READER_H
#define PROC_READER_H

#define MAX_NAME_LEN 256
#define MAX_PROCESSES 32768

typedef struct {
    int pid;
    int ppid;
    int tgid;  /* Thread Group ID, utile pour -T */
    int pgid;  /* Process Group ID, utile pour -g */
    char name[MAX_NAME_LEN];
    char cmdline[MAX_NAME_LEN];
} ProcessInfo;

int read_all_processes(ProcessInfo *procs, int max_procs, int hide_threads);

#endif
