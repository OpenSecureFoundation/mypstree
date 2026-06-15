
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include "tree_printer.h"

/* UTF-8 */
#define U_BRANCH  "├─"
#define U_LAST    "└─"
#define U_VERT    "│ "
#define U_EMPTY   "  "

/* ASCII */
#define A_BRANCH  "|-"
#define A_LAST    "`-"
#define A_VERT    "| "
#define A_EMPTY   "  "

/* VT100 Graphics (-G) */
#define G_BRANCH  "\033(0tq\033(B"
#define G_LAST    "\033(0mq\033(B"
#define G_VERT    "\033(0x\033(B "
#define G_EMPTY   "  "

/* ANSI Colors */
#define ANSI_BOLD    "\033[1m"
#define ANSI_RESET   "\033[0m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_RED     "\033[31m"


// Fonction interne pour détecter dynamiquement la largeur du terminal
static int get_terminal_width() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
        return w.ws_col;
    }
    char *cols = getenv("COLUMNS");
    if (cols) {
        int c = atoi(cols);
        if (c > 0) return c;
    }
    return 80; // Largeur par défaut standard si non détectée
}

// Fonction pour imprimer intelligemment en gérant la troncature UTF-8 et ANSI
static void print_truncated_line(const char *s, int max_width, int long_lines) {
    if (long_lines) {
        printf("%s\n", s);
        return;
    }

    // Étape 1 : Calculer la largeur visuelle réelle de la ligne (sans les codes ANSI)
    int total_visual = 0;
    const char *ptr = s;
    while (*ptr) {
        if (*ptr == '\033') { // On saute les codes couleur ANSI complets
            while (*ptr && *ptr != 'm') ptr++;
            if (*ptr) ptr++;
            continue;
        }
        if ((*ptr & 0xC0) != 0x80) total_visual++; // Compte les caractères UTF-8 valides
        ptr++;
    }

    // Si la ligne entière rentre à l'écran, on l'affiche directement
    if (total_visual <= max_width) {
        printf("%s\n", s);
        return;
    }

    // On tronque à (max_width - 1) et on ajoute un '+' à la fin
    int visual_count = 0;
    int i = 0;
    while (s[i]) {
        if (s[i] == '\033') { // On laisse passer les balises ANSI pour ne pas casser la couleur
            while (s[i] && s[i] != 'm') { putchar(s[i]); i++; }
            if (s[i]) { putchar(s[i]); i++; }
            continue;
        }
        if ((s[i] & 0xC0) != 0x80) {
            if (visual_count >= max_width - 1) {
                printf("+%s\n", ANSI_RESET); // Ajout du "+" de troncature et reset couleur
                return;
            }
            visual_count++;
        }
        putchar(s[i]);
        i++;
    }
    printf("\n");
}

static void print_node(TreeNode *node,uid_t parent_uid,const char *prefix, int is_last, const Options *opts, int duplicate_count,int depth) {
    if (!node) return;
    const char *branch = U_BRANCH;
    const char *last   = U_LAST;
    const char *vert   = U_VERT;
    const char *empty  = U_EMPTY;

    if (opts->ascii_trace) {
        branch = A_BRANCH;
        last   = A_LAST;
        vert   = A_VERT;
        empty  = A_EMPTY;
    } else if (opts->vt100_trace) {
        branch = G_BRANCH;
        last   = G_LAST;
        vert   = G_VERT;
        empty  = G_EMPTY;
    }
    char buf[4096];
    int len = 0;
    buf[0] = '\0';
    
    // Assemblage de la ligne dans le buffer
    len += snprintf(buf + len, sizeof(buf) - len, "%s", prefix);
    if (opts->ascii_trace) {
        len += snprintf(buf + len, sizeof(buf) - len, "%s", is_last ? A_LAST : A_BRANCH);
    } else {
        len += snprintf(buf + len, sizeof(buf) - len, "%s", is_last ? U_LAST : U_BRANCH);
    }

    if (opts->show_ns_changes && node->proc->ppid != 0) {
        len += snprintf(buf + len, sizeof(buf) - len, "(ns)%s", opts->ascii_trace ? "-" : "─");
    }

    // Gestion de la couleur
    if (node->highlight_path) {
        len += snprintf(buf + len, sizeof(buf) - len, "%s", ANSI_BOLD);
    } else if (opts->color_attr && strcmp(opts->color_attr, "age") == 0) {
        if (node->proc->starttime < 10000) len += snprintf(buf + len, sizeof(buf) - len, "%s", ANSI_RED);
        else len += snprintf(buf + len, sizeof(buf) - len, "%s", ANSI_GREEN);
    }

    if (duplicate_count > 1) len += snprintf(buf + len, sizeof(buf) - len, "%d*[", duplicate_count);

    if (node->proc->is_thread) {
        if (opts->show_full_threads) len += snprintf(buf + len, sizeof(buf) - len, "%s", node->proc->name);
        else len += snprintf(buf + len, sizeof(buf) - len, "{%s}", node->proc->name);
    } else {
        len += snprintf(buf + len, sizeof(buf) - len, "%s", node->proc->name);
    }
    if (opts->show_uid_changes && node->proc->uid != parent_uid) {
        struct passwd *pw = getpwuid(node->proc->uid);
        if (pw) {
            // Si on trouve le nom de l'user (ex: root, www-data)
            len += snprintf(buf + len, sizeof(buf) - len, "(%s)", pw->pw_name);
        } else {
            // Repli sécurisé : si l'UID n'a pas de nom, on affiche le chiffre
            len += snprintf(buf + len, sizeof(buf) - len, "(%d)", node->proc->uid);
        }
    }

    if (duplicate_count > 1) len += snprintf(buf + len, sizeof(buf) - len, "]");

    // Attributs optionnels
    if (opts->show_selinux) len += snprintf(buf + len, sizeof(buf) - len, " [%s]", node->proc->selinux_context);
    if (opts->show_pids) len += snprintf(buf + len, sizeof(buf) - len, "(%d)", node->proc->pid);
    if (opts->show_pgid) len += snprintf(buf + len, sizeof(buf) - len, "[pgid:%d]", node->proc->pgid);
    if (opts->show_args && node->proc->cmdline[0] != '\0') len += snprintf(buf + len, sizeof(buf) - len, " %s", node->proc->cmdline);
    
    len += snprintf(buf + len, sizeof(buf) - len, "%s", ANSI_RESET);

    // Envoi à l'afficheur intelligent avec détection de largeur
    print_truncated_line(buf, get_terminal_width(), opts->long_lines);

    // Préparation préfixe enfant
    char child_prefix[1024];
    if (opts->ascii_trace) snprintf(child_prefix, sizeof(child_prefix), "%s%s", prefix, is_last ? A_EMPTY : A_VERT);
    else snprintf(child_prefix, sizeof(child_prefix), "%s%s", prefix, is_last ? U_EMPTY : U_VERT);

    int do_compact = !opts->disable_compact && !opts->show_pids;
    if (opts->max_depth != -1 && depth >= opts->max_depth) {
        if (node->child_count > 0) {
            printf("%s%s[...]\n", prefix, is_last ? last : branch);
            // printf(" [...]"); // Petit indicateur visuel pour dire qu'il y a des sous-processus masqués
        }
        printf("\n");
        return; // On s'arrête ici, on ne descend pas chez les enfants !
    }

    for (int i = 0; i < node->child_count; ) {
        int count = 1;
        char child_prefix[1024];
        snprintf(child_prefix, sizeof(child_prefix), "%s%s", prefix, is_last ? empty : vert);
        if (do_compact) {
            while (i + count < node->child_count && 
                   strcmp(node->children[i]->proc->name, node->children[i + count]->proc->name) == 0 &&
                   node->children[i]->child_count == 0 && node->children[i + count]->child_count == 0) {
                count++;
            }
        }
        int child_is_last = (i + count >= node->child_count);
        print_node(node->children[i],node->proc->uid,child_prefix, child_is_last, opts, count,depth + 1);
        i += count;
    }
}

void print_tree(TreeNode *node, const Options *opts) {
    if (!node) return;

    char buf[4096];
    int len = 0;
    buf[0] = '\0';

    if (node->highlight_path) {
        len += snprintf(buf + len, sizeof(buf) - len, "%s", ANSI_BOLD);
    } else if (opts->color_attr && strcmp(opts->color_attr, "age") == 0) {
        if (node->proc->starttime < 10000) len += snprintf(buf + len, sizeof(buf) - len, "%s", ANSI_RED);
        else len += snprintf(buf + len, sizeof(buf) - len, "%s", ANSI_GREEN);
    }

    len += snprintf(buf + len, sizeof(buf) - len, "%s", node->proc->name);
    if (opts->show_selinux) len += snprintf(buf + len, sizeof(buf) - len, " [%s]", node->proc->selinux_context);
    if (opts->show_pids) len += snprintf(buf + len, sizeof(buf) - len, "(%d)", node->proc->pid);
    len += snprintf(buf + len, sizeof(buf) - len, "%s", ANSI_RESET);

    print_truncated_line(buf, get_terminal_width(), opts->long_lines);

    int do_compact = !opts->disable_compact && !opts->show_pids;

    for (int i = 0; i < node->child_count; ) {
        int count = 1;
        if (do_compact) {
            while (i + count < node->child_count && 
                   strcmp(node->children[i]->proc->name, node->children[i + count]->proc->name) == 0 &&
                   node->children[i]->child_count == 0 && node->children[i + count]->child_count == 0) {
                count++;
            }
        }
        int is_last = (i + count >= node->child_count);
        print_node(node->children[i],node->proc->uid, "", is_last, opts, count,1);
        i += count;
    }
}
