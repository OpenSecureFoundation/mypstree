#ifndef OPTIONS_H
#define OPTIONS_H

typedef struct {
    int show_pids;
    int highlight;
    int sort_by_pid;       /* -n: Tri numérique par PID (Standard pstree) */
    int show_args;
    int disable_compact;   /* -c: Désactive le regroupement (ex: 3*[bash]) */
    int target_pid;
    int ascii_trace;       /* -A */
    int show_pgid;         /* -g */
    int show_parents_only; /* -s */
    int hide_threads;      /* -T */
    int show_full_threads; /* -t: Affiche les noms bruts au lieu de {nom} */
    char *color_attr;      /* -C: Colorisation (ex: age) */
    char *ns_sort;         /* -N: Tri par namespace */
    int show_ns_changes;   /* -S: Transitions de namespace */
    int show_selinux;      /* -Z: Affiche les contextes SELinux */
    int long_lines;
    int force_utf8;
} Options;

void print_usage(const char *program_name);
int parse_options(int argc, char *argv[], Options *opts);

#endif
