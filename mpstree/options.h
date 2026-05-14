/* options.h */
#ifndef OPTIONS_H
#define OPTIONS_H

typedef struct {
    int show_pids;       /* -p: show PID next to process name */
    int highlight;       /* -h: highlight current process path */
    int sort_by_name;    /* -n: sort children alphabetically */
    int show_args;       /* -a: show command line arguments */
    int compact;         /* -c: don't compact identical subtrees */
    int target_pid;      /* optional: root tree at this PID */
} Options;

/* Parse argv and populate opts. Returns 0 on success, -1 on error. */
int parse_options(int argc, char *argv[], Options *opts);

/* Print usage message */
void print_usage(const char *program_name);

#endif /* OPTIONS_H */
