#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree_builder.h"

static TreeNode *create_node(ProcessInfo *proc) {
    TreeNode *node = malloc(sizeof(TreeNode));
    if (!node) {
        perror("malloc TreeNode");
        return NULL;
    }
    node->proc           = proc;
    node->children       = NULL;
    node->child_count    = 0;
    node->child_capacity = 0;
    node->keep_for_parents = 0;
    return node;
}

static int add_child(TreeNode *parent, TreeNode *child) {
    if (parent->child_count >= parent->child_capacity) {
        int new_cap = (parent->child_capacity == 0) ? 4 : parent->child_capacity * 2;
        TreeNode **new_children = realloc(parent->children,
                                          new_cap * sizeof(TreeNode *));
        if (!new_children) {
            perror("realloc children");
            return -1;
        }
        parent->children       = new_children;
        parent->child_capacity = new_cap;
    }
    parent->children[parent->child_count++] = child;
    return 0;
}

static TreeNode *find_node_by_pid(TreeNode **nodes, int count, int pid) {
    for (int i = 0; i < count; i++) {
        if (nodes[i] && nodes[i]->proc->pid == pid) return nodes[i];
    }
    return NULL;
}

static int compare_by_name(const void *a, const void *b) {
    TreeNode *na = *(TreeNode **)a;
    TreeNode *nb = *(TreeNode **)b;
    return strcmp(na->proc->name, nb->proc->name);
}

void sort_tree_by_name(TreeNode *node) {
    if (!node || node->child_count == 0) return;

    qsort(node->children, node->child_count, sizeof(TreeNode *), compare_by_name);

    for (int i = 0; i < node->child_count; i++) {
        sort_tree_by_name(node->children[i]);
    }
}

TreeNode *build_tree(ProcessInfo *procs, int proc_count, int root_pid) {
    TreeNode **nodes = calloc(proc_count, sizeof(TreeNode *));
    if (!nodes) {
        perror("calloc nodes");
        return NULL;
    }

    for (int i = 0; i < proc_count; i++) {
        nodes[i] = create_node(&procs[i]);
        if (!nodes[i]) {
            for (int j = 0; j < i; j++) free(nodes[j]);
            free(nodes);
            return NULL;
        }
    }

    TreeNode *root = NULL;

    for (int i = 0; i < proc_count; i++) {
        int pid  = nodes[i]->proc->pid;
        int ppid = nodes[i]->proc->ppid;

        if (pid == root_pid) {
            root = nodes[i];
            continue;
        }

        TreeNode *parent = find_node_by_pid(nodes, proc_count, ppid);
        if (parent) {
            add_child(parent, nodes[i]);
        }
    }

    free(nodes);
    return root;
}

void free_tree(TreeNode *node) {
    if (!node) return;
    for (int i = 0; i < node->child_count; i++) {
        free_tree(node->children[i]);
    }
    free(node->children);
    free(node);
}

/* Helper récursif pour marquer la lignée d'un processus (Option -s) */
static int mark_parents(TreeNode *node, int target_pid) {
    if (!node) return 0;

    if (node->proc->pid == target_pid) {
        node->keep_for_parents = 1;
        return 1;
    }

    int keep_this = 0;
    for (int i = 0; i < node->child_count; i++) {
        if (mark_parents(node->children[i], target_pid)) {
            keep_this = 1;
        }
    }

    if (keep_this) {
        node->keep_for_parents = 1;
    }
    return keep_this;
}

/* Nettoyer l'arbre en supprimant les branches non marquées */
static void prune_tree(TreeNode *node) {
    if (!node) return;

    int new_count = 0;
    for (int i = 0; i < node->child_count; i++) {
        if (node->children[i]->keep_for_parents) {
            node->children[new_count++] = node->children[i];
            prune_tree(node->children[i]);
        } else {
            free_tree(node->children[i]); /* Libérer la mémoire des branches non pertinentes */
        }
    }
    node->child_count = new_count;
}

void filter_parents_only(TreeNode *node, int target_pid) {
    if (!node) return;
    mark_parents(node, target_pid);
    prune_tree(node);
}
