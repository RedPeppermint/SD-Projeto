//GRUPO 54
//Sara Queimado 52806
//Diogo Fernandes 52802
//Eduardo Salvadinha 52768

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"

typedef struct data_t DATA;

struct data_t *data_create(int size)
{
    if (size <= 0)
    {
        return NULL;
    }
    DATA *create = malloc(sizeof(struct data_t));
    if (create == NULL)
        return NULL;
    create->data = malloc(size);
    create->datasize = size;
    return create;
}

struct data_t *data_create2(int size, void *data)
{
    if (size <= 0 || data == NULL)
    {
        return NULL;
    }
    DATA *create = malloc(sizeof(struct data_t));
    if (create == NULL)
        return NULL;
    create->datasize = size;
    create->data = data;
    return create;
}

void data_destroy(struct data_t *data)
{
    if (data != NULL)
    {
        if (data->datasize > 0)
        {

            free(data->data);
        }
        free(data);
    }
}

struct data_t *data_dup(struct data_t *data)
{
    if (data == NULL)
        return NULL;
    if (data->datasize <= 0)
        return NULL;
    if (data->data == NULL)
        return NULL;

    DATA *copy = data_create(data->datasize);
    memcpy(copy->data, data->data, data->datasize);
    return copy;
}

void data_replace(struct data_t *data, int new_size, void *new_data)
{
    if (data != NULL)
    {
        data->datasize = new_size;
        free(data->data);
        data->data = new_data;
    }
}