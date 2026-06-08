#ifndef TREE_BUILDER_H
#define TREE_BUILDER_H

#include "proc_reader.h"
#include "options.h"

typedef struct TreeNode {
    ProcessInfo *proc;
    struct TreeNode **children;
    int child_count;
    int child_capacity;
    int keep_for_parents;
    int highlight_path; // Flag pour la lignee -h
} TreeNode;

TreeNode *build_tree(ProcessInfo *procs, int proc_count, int root_pid);
void sort_tree(TreeNode *node, const Options *opts);
void free_tree(TreeNode *node);
void filter_parents_only(TreeNode *node, int target_pid);
void mark_highlight_path(TreeNode *node, int target_pid);

#endif