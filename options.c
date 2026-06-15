#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "options.h"

#define VERSION "2.0.0"

void print_usage(const char *program_name) {
    fprintf(stderr,
        "Usage: %s [options] [PID]\n"
        "Options:\n"
        "  -p    Afficher les PIDs\n"
        "  -h    Mettre en evidence le processus actuel et ses ancetres\n"
        "  -n    Trier les processus par PID (numerique)\n"
        "  -a    Afficher les arguments de ligne de commande\n"
        "  -c    Desactiver le compactage de l'arbre\n"
        "  -A    Utiliser le trace ASCII\n"
        "  -g    Afficher les PGID\n"
        "  -s    Parents uniquement\n"
        "  -T    Masquer les threads\n"
        "  -t    Afficher les noms complets des threads\n"
        "  -C    Coloriser (ex: -C age)\n"
        "  -N    Trier par namespace (ex: -N net)\n"
        "  -S    Afficher les transitions de namespace\n"
        "  -V    Afficher la version\n"
        "  -H    Aide\n"
        "  -Z    Afficher les contextes SELinux\n"
        "  -l    Ne pas tronquer les lignes longues (lignes entieres)\n"
        "  -U    forcer l'utilisation des caracteres de dessin UTF-8(unicode)\n"
        "  -u    Afficher les transitions d'utilisateurs (UID)\n"
        "  -G    Utiliser les caracteres de dessin de ligne VT100\n"
        "  -d    Limiter la profondeur d'affichage de l'arbre\n",
        program_name);
}

int parse_options(int argc, char *argv[], Options *opts) {
    int opt;

    opts->show_pids = 0;
    opts->highlight = 0;
    opts->sort_by_pid = 0;
    opts->show_args = 0;
    opts->disable_compact = 0;
    opts->target_pid = -1;
    opts->ascii_trace = 0;
    opts->show_pgid = 0;
    opts->show_parents_only = 0;
    opts->hide_threads = 0;
    opts->show_full_threads = 0;
    opts->color_attr = NULL;
    opts->ns_sort = NULL;
    opts->show_ns_changes = 0;
    opts->show_selinux = 0;
    opts->long_lines = 0;
    opts->force_utf8 = 0;
    opts->show_uid_changes = 0;
    opts->vt100_trace = 0;
    opts->max_depth = -1;
    while ((opt = getopt(argc, argv, "phnacHAgTsVtC:N:SZlUud:G")) != -1) {
        switch (opt) {
            case 'p': opts->show_pids = 1; break;
            case 'h': opts->highlight = 1; break;
            case 'n': opts->sort_by_pid = 1; break;
            case 'a': opts->show_args = 1; break;
            case 'c': opts->disable_compact = 1; break;
            case 'A': opts->ascii_trace = 1;  opts->force_utf8 =0;opts->vt100_trace = 0; break;
            case 'g': opts->show_pgid = 1; break;
            case 's': opts->show_parents_only = 1; break;
            case 'T': opts->hide_threads = 1; break;
            case 't': opts->show_full_threads = 1; break;
            case 'C': opts->color_attr = optarg; break;
            case 'N': opts->ns_sort = optarg; break;
            case 'S': opts->show_ns_changes = 1; break;
            case 'Z': opts->show_selinux = 1; break;
            case 'l': opts->long_lines = 1; break;
            case 'u': opts->show_uid_changes = 1;break;
            case 'G':opts->vt100_trace = 1;opts->ascii_trace = 0;opts->force_utf8 = 0; break;
            case 'd': opts->max_depth = atoi(optarg);
            if (opts->max_depth < 0) {
                fprintf(stderr, "Erreur : La profondeur doit être un entier positif.\n");
                exit(EXIT_FAILURE);
            }
            break;
            case 'V':
                printf("mypstree version %s\n", VERSION);
                exit(EXIT_SUCCESS);
            case 'H':
                print_usage(argv[0]);
                exit(EXIT_SUCCESS);
            case 'U':
                opts->force_utf8 = 1;
                opts->ascii_trace = 0;
            break;
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
