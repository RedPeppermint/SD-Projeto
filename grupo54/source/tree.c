//GRUPO 54
//Sara Queimado 52806
//Diogo Fernandes 52802
//Eduardo Salvadinha 52768

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data.h"
#include "tree.h"
#include "entry.h"
#include "tree-private.h"

/* Função para criar uma nova árvore tree vazia.
 * Em caso de erro retorna NULL.
 */
struct tree_t *tree_create()
{
    struct tree_t *node = malloc(sizeof(struct tree_t));
    if (node == NULL)
        return NULL;
    node->entry = malloc(sizeof(struct entry_t));
    entry_initialize(node->entry);
    node->left = NULL;
    node->right = NULL;
    return node;
}

/* Função para libertar toda a memória ocupada por uma árvore.
 */
void tree_destroy(struct tree_t *tree)
{

    if (tree != NULL)
    {
        tree_destroy(tree->left);
        tree_destroy(tree->right);
        if (tree->entry->key != NULL)
            entry_destroy(tree->entry);
        free(tree);
        tree = NULL;
    }
}

int tree_put_aux(struct tree_t *tree, struct entry_t *entry)
{
    if (tree->entry->key == NULL)
    {
        tree->entry = entry_dup(entry);
        tree->left = NULL;
        tree->right = NULL;
        entry_destroy(entry);
        return 0;
    }
    int cpr = entry_compare(tree->entry, entry);
    if (cpr > 0)
    {

        if (tree->left == NULL)
        {
            struct tree_t *child = tree_create();
            if (child == NULL)
                return -1;
            child->entry = entry_dup(entry);
            tree->left = child;
            entry_destroy(entry);
            return 0;
        }
        return tree_put_aux(tree->left, entry);
    }
    else if (cpr < 0)
    {
        if (tree->right == NULL)
        {
            struct tree_t *child = tree_create();
            if (child == NULL)
                return -1;
            child->entry = entry_dup(entry);
            tree->right = child;
            entry_destroy(entry);
            return 0;
        }
        return tree_put_aux(tree->right, entry);
    }
    else
    {
        entry_replace(tree->entry, entry->key, entry->value);
        return 0;
    }
}

/* Função para adicionar um par chave-valor à árvore.
 * Os dados de entrada desta função deverão ser copiados, ou seja, a
 * função vai *COPIAR* a key (string) e os dados para um novo espaço de
 * memória que tem de ser reservado. Se a key já existir na árvore,
 * a função tem de substituir a entrada existente pela nova, fazendo
 * a necessária gestão da memória para armazenar os novos dados.
 * Retorna 0 (ok) ou -1 em caso de erro.
 */
int tree_put(struct tree_t *tree, char *key, struct data_t *value)
{
    struct entry_t *create_aux = entry_create(key, value);
    struct entry_t *new_aux = entry_dup(create_aux);
    int i = tree_put_aux(tree, new_aux);
    entry_destroy(create_aux);

    return i;
}

/* Função para obter da árvore o valor associado à chave key.
 * A função deve devolver uma cópia dos dados que terão de ser
 * libertados no contexto da função que chamou tree_get, ou seja, a
 * função aloca memória para armazenar uma *CÓPIA* dos dados da árvore,
 * retorna o endereço desta memória com a cópia dos dados, assumindo-se
 * que esta memória será depois libertada pelo programa que chamou
 * a função.
 * Devolve NULL em caso de erro.
 */
struct data_t *tree_get(struct tree_t *tree, char *key)
{
    if (tree == NULL)
        return NULL;
    if (tree->entry == NULL)
    {
        return NULL;
    }
    if (tree->entry->key == NULL)
    {
        return NULL;
    }

    int cpr = strcmp(tree->entry->key, key);
    if (cpr == 0)
        return data_dup(tree->entry->value);
    else if (cpr > 0)
        return tree_get(tree->left, key);
    else
        return tree_get(tree->right, key);
}

/*
Devolve o nó pai do nó de valor mínimo
*/
struct tree_t *minValueNode(struct tree_t *tree)
{
    struct tree_t *curr_tree = tree;

    /* loop down to find the leftmost leaf */
    while (curr_tree->left && curr_tree->left->left != NULL)
        curr_tree = curr_tree->left;

    return curr_tree;
}

/* Função para remover um elemento da árvore, indicado pela chave key,
 * libertando toda a memória alocada na respetiva operação tree_put.
 * Retorna 0 (ok) ou -1 (key not found).
 */
int tree_del(struct tree_t *tree, char *key)
{
    if (tree == NULL)
    {
        return -1;
    }
    struct data_t *aux = tree_get(tree, key);
    if (aux == NULL)
    {
        return -1;
    }
    data_destroy(aux);
    if (tree->right == NULL && tree->left == NULL)
    {
        entry_destroy(tree->entry);
        tree->entry = malloc(sizeof(struct entry_t));
        entry_initialize(tree->entry);
    }
    else
    {
        tree = tree_delete_aux(tree, key);
        if (tree == NULL)
        {
            tree = tree_create();
        }
    }
    return 0;
}

struct tree_t *tree_delete_aux(struct tree_t *tree, char *key)
{

    // If the key to be deleted is smaller than the root's key,
    int cpr = strcmp(key, tree->entry->key);
    if (cpr < 0)
        tree->left = tree_delete_aux(tree->left, key);
    // If the key to be deleted is greater than the root's key,
    else if (cpr > 0)
        tree->right = tree_delete_aux(tree->right, key);
    else
    {
        if (tree->left == NULL)
        {
            struct tree_t *temp = tree->right;

            if (temp == NULL)
            {
                return NULL;
            }
            entry_destroy(tree->entry);
            tree->entry = NULL;
            tree->entry = temp->entry;
            tree->right = temp->right;
            tree->left = temp->left;
            return tree;
        }
        else if (tree->right == NULL)
        {
            struct tree_t *temp = tree->left;

            if (temp == NULL)
            {
                return NULL;
            }
            entry_destroy(tree->entry);
            tree->entry = NULL;
            tree->entry = temp->entry;
            tree->right = temp->right;
            tree->left = temp->left;
            return tree;
        }
        else
        {
            // node with two children:
            struct tree_t *temp = minValueNode(tree->right);
            // Copy the inorder successor's content to this node
            tree->entry = temp->entry;
            // Delete the inorder successor
            tree->right = tree_delete_aux(tree->right, temp->entry->key);
        }
    }
    return tree;
}

/* Função que devolve o número de elementos contidos na árvore.
 */
int tree_size(struct tree_t *tree)
{
    if (tree == NULL)
        return 0;
    if (tree->entry == NULL)
        return 0;
    if (tree->entry->key == NULL)
        return 0;
    return 1 + tree_size(tree->left) + tree_size(tree->right);
}

int max(int value1, int value2)
{
    if (value1 < value2)
        return value2;
    return value1;
}

/* Função que devolve a altura da árvore.
 */
int tree_height(struct tree_t *tree)
{
    if (tree == NULL)
        return 0;
    if (tree->entry == NULL)
        return 0;
    if (tree->entry->key == NULL)
        return 0;
    return 1 + max(tree_height(tree->left), tree_height(tree->right));
}

//        4
//    1       6
// 0     3  5    7

// 4 1 0 3 6 5 7 NULL
// 0 1 2 3 4 5 6 7
int add_to_array(struct tree_t *tree, char **array, int index)
{
    if (tree == NULL || tree->entry->key == NULL)
        return index - 1;

    char *copy = malloc(strlen(tree->entry->key) + 1);
    memcpy(copy, tree->entry->key, strlen(tree->entry->key) + 1);
    array[index] = copy;
    index = add_to_array(tree->left, array, index + 1);
    index = add_to_array(tree->right, array, index + 1);
    return index;
}
/* Função que devolve um array de char* com a cópia de todas as keys da
 * árvore, colocando o último elemento do array com o valor NULL e
 * reservando toda a memória necessária.
 */
char **tree_get_keys(struct tree_t *tree)
{

    if (tree->entry->key == NULL)
    {
        char **array = malloc(sizeof(NULL));
        array[0] = NULL;
        return array;
    }
    char **array = malloc(sizeof(char *) * tree_size(tree) + sizeof(NULL));
    int i = add_to_array(tree, array, 0);
    array[i + 1] = NULL;
    return array;
}

/* Função que liberta toda a memória alocada por tree_get_keys().
 */
void tree_free_keys(char **keys)
{

    if (keys == NULL)
    {
        return;
    }
    else if (keys[0] == NULL)
    {
        free(keys);
        return;
    }
    else
    {
        int i = 0;
        while (keys[i] != NULL)
        {
            free(keys[i]);
            i++;
        }
    }
}
