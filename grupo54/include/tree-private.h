//GRUPO 54
//Sara Queimado 52806
//Diogo Fernandes 52802
//Eduardo Salvadinha 52768

#ifndef _TREE_PRIVATE_H
#define _TREE_PRIVATE_H

#include "tree.h"
#include "entry.h"

struct tree_t
{
	/** a preencher pelo grupo */
	struct entry_t *entry;
	struct tree_t *left;
	struct tree_t *right;
};

int tree_put_aux(struct tree_t *tree, struct entry_t *entry);
struct tree_t *minValueNode(struct tree_t *tree);
int max(int value1, int value2);
int add_to_array(struct tree_t *tree, char **array, int index);
struct tree_t *tree_delete_aux(struct tree_t *tree, char *key);

#endif
