/* proc_reader.h */
#ifndef PROC_READER_H
#define PROC_READER_H

#define MAX_NAME_LEN   256
#define MAX_PROCESSES  65536

typedef struct {
    int  pid;
    int  ppid;
    char name[MAX_NAME_LEN];
    char cmdline[MAX_NAME_LEN];
} ProcessInfo;

/*
 * Scan /proc and fill the procs[] array.
 * Returns the number of processes found, or -1 on error.
 */
int read_all_processes(ProcessInfo *procs, int max_procs);

#endif /* PROC_READER_H */
