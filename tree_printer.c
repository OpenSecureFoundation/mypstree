/*#include <stdio.h>
#include <string.h>
#include "tree_printer.h"*/

/* UTF-8 
#define U_BRANCH  "├─"
#define U_LAST    "└─"
#define U_VERT    "│ "
#define U_EMPTY   "  "

ASCII 
#define A_BRANCH  "|-"
#define A_LAST    "`-"
#define A_VERT    "| "
#define A_EMPTY   "  "

 ANSI Colors 
#define ANSI_BOLD    "\033[1m"
#define ANSI_RESET   "\033[0m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_RED     "\033[31m"

static void set_color(TreeNode *node, const Options *opts) {
    if (node->highlight_path) {
        printf("%s", ANSI_BOLD);
    } else if (opts->color_attr && strcmp(opts->color_attr, "age") == 0) {
        // Logique fictive age : rouge ancien, vert recent
        if (node->proc->starttime < 10000) printf("%s", ANSI_RED);
        else printf("%s", ANSI_GREEN);
    }
}

static void print_node(TreeNode *node, const char *prefix, int is_last, const Options *opts, int duplicate_count) {
    if (!node) return;
    
    printf("%s", prefix);
    if (opts->ascii_trace) printf("%s", is_last ? A_LAST : A_BRANCH);
    else printf("%s", is_last ? U_LAST : U_BRANCH);

    // Transitions Namespace (-S)
    if (opts->show_ns_changes && node->proc->ppid != 0) {
        printf("(ns)%s", opts->ascii_trace ? "-" : "─");
    }

    set_color(node, opts);

    // Affichage compact
    if (duplicate_count > 1) printf("%d*[", duplicate_count);

    // Format threads
    if (node->proc->is_thread) {
        if (opts->show_full_threads) printf("%s", node->proc->name);
        else printf("{%s}", node->proc->name);
    } else {
        printf("%s", node->proc->name);
    }

    if (duplicate_count > 1) printf("]");

    // Arguments supplementaires
    if (opts->show_selinux) printf(" [%s]", node->proc->selinux_context);
    if (opts->show_pids) printf("(%d)", node->proc->pid);
    if (opts->show_pgid) printf("[pgid:%d]", node->proc->pgid);
    if (opts->show_args && node->proc->cmdline[0] != '\0') printf(" %s", node->proc->cmdline);
    
    printf("%s\n", ANSI_RESET);

    // Preparation prefixe enfant
    char child_prefix[1024];
    if (opts->ascii_trace) snprintf(child_prefix, sizeof(child_prefix), "%s%s", prefix, is_last ? A_EMPTY : A_VERT);
    else snprintf(child_prefix, sizeof(child_prefix), "%s%s", prefix, is_last ? U_EMPTY : U_VERT);

    // Logique de compactage enfant
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
        int child_is_last = (i + count >= node->child_count);
        print_node(node->children[i], child_prefix, child_is_last, opts, count);
        i += count;
    }
}

void print_tree(TreeNode *node, const Options *opts) {
    if (!node) return;

    set_color(node, opts);
    printf("%s", node->proc->name);
    if (opts->show_selinux) printf(" [%s]", node->proc->selinux_context);
    if (opts->show_pids) printf("(%d)", node->proc->pid);
    printf("%s\n", ANSI_RESET);

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
        print_node(node->children[i], "", is_last, opts, count);
        i += count;
    }
}*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
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

    // Étape 2 : Ça dépasse ! On tronque à (max_width - 1) et on ajoute un '+' à la fin
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

static void print_node(TreeNode *node, const char *prefix, int is_last, const Options *opts, int duplicate_count) {
    if (!node) return;
    
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

    for (int i = 0; i < node->child_count; ) {
        int count = 1;
        if (do_compact) {
            while (i + count < node->child_count && 
                   strcmp(node->children[i]->proc->name, node->children[i + count]->proc->name) == 0 &&
                   node->children[i]->child_count == 0 && node->children[i + count]->child_count == 0) {
                count++;
            }
        }
        int child_is_last = (i + count >= node->child_count);
        print_node(node->children[i], child_prefix, child_is_last, opts, count);
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
        print_node(node->children[i], "", is_last, opts, count);
        i += count;
    }
}
