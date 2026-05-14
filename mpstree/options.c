/* options.c */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   /* getopt */
#include "options.h"

void print_usage(const char *program_name) {
    fprintf(stderr,
        "Usage: %s [options] [PID]\n"
        "Options:\n"
        "  -p    Show PIDs in parentheses\n"
        "  -h    Highlight the current process and its ancestors\n"
        "  -n    Sort processes by name instead of PID\n"
        "  -a    Show command line arguments\n"
        "  -c    Disable compaction of identical subtrees\n"
        "  -H    Print this help message\n",
        program_name);
}

int parse_options(int argc, char *argv[], Options *opts) {
    int opt;

    /* Zero out the struct first */
    opts->show_pids    = 0;
    opts->highlight    = 0;
    opts->sort_by_name = 0;
    opts->show_args    = 0;
    opts->compact      = 0;
    opts->target_pid   = -1;  /* -1 means "start from PID 1" */

    while ((opt = getopt(argc, argv, "phnacH")) != -1) {
        switch (opt) {
            case 'p': opts->show_pids    = 1; break;
            case 'h': opts->highlight    = 1; break;
            case 'n': opts->sort_by_name = 1; break;
            case 'a': opts->show_args    = 1; break;
            case 'c': opts->compact      = 1; break;
            case 'H':
                print_usage(argv[0]);
                exit(EXIT_SUCCESS);
            default:
                print_usage(argv[0]);
                return -1;
        }
    }

    /* Optional positional argument: a PID to root the tree at */
    if (optind < argc) {
        opts->target_pid = atoi(argv[optind]);
        if (opts->target_pid <= 0) {
            fprintf(stderr, "Error: invalid PID '%s'\n", argv[optind]);
            return -1;
        }
    }

    return 0;
}
