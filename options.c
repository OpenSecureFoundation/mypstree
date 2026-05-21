#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "options.h"

#define VERSION "1.0.0"

void print_usage(const char *program_name) {
    fprintf(stderr,
        "Usage: %s [options] [PID]\n"
        "Options:\n"
        "  -p    Afficher les PIDs\n"
        "  -h    Mettre en evidence le processus actuel et ses ancetres\n"
        "  -n    Trier les processus par nom\n"
        "  -a    Afficher les arguments de ligne de commande\n"
        "  -c    Desactiver le compactage (optionnel pour l'affichage)\n"
        "  -H    Afficher ce message d'aide\n"
        "  -A    Utiliser le trace ASCII (Nouvelle option)\n"
        "  -g    Afficher les identifiants de groupe PGID (Nouvelle option)\n"
        "  -s    Afficher uniquement les parents d'un processus specifie (Nouvelle option)\n"
        "  -T    Masquer les threads (Nouvelle option)\n"
        "  -V    Afficher la version (Nouvelle option)\n",
        program_name);
}

int parse_options(int argc, char *argv[], Options *opts) {
    int opt;

    /* Initialisation a zero */
    opts->show_pids = 0;
    opts->highlight = 0;
    opts->sort_by_name = 0;
    opts->show_args = 0;
    opts->compact = 0;
    opts->target_pid = -1;
    opts->ascii_trace = 0;
    opts->show_pgid = 0;
    opts->show_parents_only = 0;
    opts->hide_threads = 0;

    /* Ajout des nouveaux drapeaux dans la chaine getopt */
    while ((opt = getopt(argc, argv, "phnacHAgTsV")) != -1) {
        switch (opt) {
            case 'p': opts->show_pids = 1; break;
            case 'h': opts->highlight = 1; break;
            case 'n': opts->sort_by_name = 1; break;
            case 'a': opts->show_args = 1; break;
            case 'c': opts->compact = 1; break;
            case 'A': opts->ascii_trace = 1; break;
            case 'g': opts->show_pgid = 1; break;
            case 's': opts->show_parents_only = 1; break;
            case 'T': opts->hide_threads = 1; break;
            case 'V':
                printf("mypstree version %s\n", VERSION);
                exit(EXIT_SUCCESS);
            case 'H':
                print_usage(argv[0]);
                exit(EXIT_SUCCESS);
            default:
                print_usage(argv[0]);
                return -1;
        }
    }

    if (optind < argc) {
        opts->target_pid = atoi(argv[optind]);
        if (opts->target_pid <= 0) {
            fprintf(stderr, "Erreur: PID invalide '%s'\n", argv[optind]);
            return -1;
        }
    }

    return 0;
}
