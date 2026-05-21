#ifndef OPTIONS_H
#define OPTIONS_H

typedef struct {
    int show_pids;
    int highlight;
    int sort_by_name;
    int show_args;
    int compact;
    int target_pid;
    /* Ajout des 5 nouvelles options */
    int ascii_trace;       /* -A */
    int show_pgid;         /* -g */
    int show_parents_only; /* -s */
    int hide_threads;      /* -T */
} Options;

void print_usage(const char *program_name);
int parse_options(int argc, char *argv[], Options *opts);

#endif
