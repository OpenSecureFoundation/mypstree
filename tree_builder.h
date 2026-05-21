#ifndef TREE_BUILDER_H
#define TREE_BUILDER_H

#include "proc_reader.h"

typedef struct TreeNode {
    ProcessInfo *proc;
    struct TreeNode **children;
    int child_count;
    int child_capacity;
    int keep_for_parents; /* Utilisé pour l'option -s */
} TreeNode;

TreeNode *build_tree(ProcessInfo *procs, int proc_count, int root_pid);
void sort_tree_by_name(TreeNode *node);
void free_tree(TreeNode *node);
void filter_parents_only(TreeNode *node, int target_pid);

#endif
