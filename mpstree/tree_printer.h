/* tree_printer.h */
#ifndef TREE_PRINTER_H
#define TREE_PRINTER_H

#include "tree_builder.h"
#include "options.h"

/*
 * Print the tree rooted at 'node'.
 * highlight_pid: PID to highlight (-h option), or -1 to disable.
 */
void print_tree(TreeNode *node, const Options *opts, int highlight_pid);

#endif /* TREE_PRINTER_H */
