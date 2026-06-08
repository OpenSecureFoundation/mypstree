#ifndef PROC_READER_H
#define PROC_READER_H

#include <sys/types.h>

#define MAX_NAME_LEN 256
#define MAX_PROCESSES 65536 // Augmente pour absorber les threads

typedef struct {
    int pid;
    int ppid;
    int tgid;
    int pgid;
    int is_thread;
    unsigned long long starttime; // Pour -C age
    ino_t ns_net;                 // Pour -N et -S (inode Namespace)
    char name[MAX_NAME_LEN];
    char cmdline[MAX_NAME_LEN];
} ProcessInfo;

int read_all_processes(ProcessInfo *procs, int max_procs, int hide_threads);

#endif