#ifndef _SERVER_PRIVATE_H
#define _SERVER_PRIVATE_H

#include "message-private.h"
#include "data.h"

typedef struct task_t
{
    int op_n; //o número da operação
    // struct message_t *msg;
    struct task_t *next;
    int op;     //a operação a executar. op=0 se for um delete, op=1 se for um put
    char *key;  //a chave a remover ou adicionar
    void *data; // os dados a adicionar em caso de put, ou NULL em caso de delete
    //adicionar campo(s) necessário(s) para implementar fila do tipo produtor/consumidor
} TASK;

typedef struct thread_parameters
{
    struct message_t msg;
} PARAMS;

#endif