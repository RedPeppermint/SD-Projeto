//GRUPO 54
//Sara Queimado 52806
//Diogo Fernandes 52802
//Eduardo Salvadinha 52768

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "entry.h"

typedef struct entry_t ENTRY;

struct entry_t *entry_create(char *key, struct data_t *data)
{
    ENTRY *entry = malloc(sizeof(struct entry_t));
    entry->key = key;
    entry->value = data;
    return entry;
}

void entry_initialize(struct entry_t *entry)
{
    entry->key = NULL;
    entry->value = NULL;
}

void entry_destroy(struct entry_t *entry)
{
    if (entry != NULL)
    {
        if (entry->value != NULL)
        {
            data_destroy(entry->value);
        }
        if (entry->key != NULL)
        {
            free(entry->key);
        }
        free(entry);
    }
}

struct entry_t *entry_dup(struct entry_t *entry)
{
    if (entry != NULL && entry->key != NULL && entry->value != NULL)
    {
        ENTRY *dup = malloc(sizeof(struct entry_t));
        dup->key = malloc(strlen(entry->key) + 1);
        strcpy(dup->key, entry->key);
        dup->value = data_dup(entry->value);
        return dup;
    }
    else
    {
        return NULL;
    }
}

void entry_replace(struct entry_t *entry, char *new_key, struct data_t *new_value)
{
    free(entry->key);
    data_destroy(entry->value);
    free(entry);
    entry = entry_create(new_key, new_value);
}

int entry_compare(struct entry_t *entry1, struct entry_t *entry2)
{
    // int size = 0;
    // if (sizeof(entry1->key) < sizeof(entry2->key))
    // {
    //     size = sizeof(entry2->key);
    // }
    // else
    // {
    //     size = sizeof(entry1->key);
    // }

    int value = strcmp(entry1->key, entry2->key);
    // int value = strcmp(entry1->key, entry2->key);
    if (value < 0)
        return -1;
    else if (value > 0)
        return 1;
    else
        return 0;
}