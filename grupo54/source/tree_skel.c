/*
GRUPO 54
Sara Queimado 52806
Diogo Fernandes 52802
Eduardo Salvadinha 52768
*/

/*
char hostBuffer[256];
            char *IPBuffer;
            struct hostent *host_entry;
            int hostname;

            if ((hostname = gethostname(hostBuffer, sizeof(hostBuffer))) == -1)
            {
                perror("hostname");
                return -1;
            }
            if ((host_entry = gethostbyname(hostBuffer)) == NULL)
            {
                perror("host_entry");
                return -1;
            }

            IPBuffer = inet_ntoa(*((struct in_addr *)host_entry->h_addr_list[0]));
            char *ip_port = IPBuffer;
            strcat(ip_port, ":");
            strcat(ip_port, port);
            */
#include "sdmessage.pb-c.h"
#include "tree.h"
#include "tree-private.h"
#include "server_private.h"
#include "message-private.h"
#include "data.h"
#include "entry.h"
#include "client_stub.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <zookeeper/zookeeper.h>

static char *root = "/kvstore";
static char *primary = "/kvstore/primary";
static char *backup = "/kvstore/backup";
static char *watcher_ctx = "ZooKeeper Data Watcher";

pthread_t new;
struct tree_t *tree;
int last_assigned, op_count, stop;
struct task_t *queue_head;
pthread_mutex_t stop_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t tree_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER; //condiçao para wait
PARAMS params;

static zhandle_t *zoo_handle;
static int is_on;

int which_server;           //1 to if primary, 0 if backup, -1 if there's no backup (NULL)
struct rtree_t *other_tree; //the other server

#define MAX_IP_SIZE 1024

int checkRun()
{
    int ret;
    pthread_mutex_lock(&stop_lock);
    ret = stop == -1;
    pthread_mutex_unlock(&stop_lock);
    //printf("%d", ret);
    return ret;
}

void free_task(struct task_t *task)
{
    if (task != NULL)
    {
        data_destroy(task->data);
        free(task->key);
        free(task);
    }
}

int verify(int op_n)
{
    return (op_n <= op_count) ? 1 : 0;
}

int real_time_invoke(struct message_t *msg)
{

    MessageT__Opcode opcode = msg->msg_t->opcode;
    MessageT__CType ctype = msg->msg_t->c_type;
    MessageT *resp = malloc(sizeof(MessageT));
    message_t__init(resp);
    if (opcode == MESSAGE_T__OPCODE__OP_SIZE)
    {
        if (ctype != MESSAGE_T__C_TYPE__CT_NONE)
        {
            msg->msg_t->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->msg_t->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return 0;
        }

        pthread_mutex_lock(&tree_lock);
        int size = tree_size(tree);
        pthread_mutex_unlock(&tree_lock);

        if (size == -1)
        {
            resp->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            resp->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            msg->msg_t = resp;
            return 0;
        }
        resp->opcode = MESSAGE_T__OPCODE__OP_SIZE + 1;
        resp->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        resp->result = size;
        free(msg->msg_t);
        msg->msg_t = resp;
        return 0;
    }

    else if (opcode == MESSAGE_T__OPCODE__OP_GET)
    {
        if (ctype != MESSAGE_T__C_TYPE__CT_KEY)
        {
            msg->msg_t->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->msg_t->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return 0;
        }
        pthread_mutex_lock(&tree_lock);
        struct data_t *data = tree_get(tree, msg->msg_t->key);
        pthread_mutex_unlock(&tree_lock);

        message_t__free_unpacked(msg->msg_t, NULL);

        resp->opcode = MESSAGE_T__OPCODE__OP_GET + 1;
        resp->c_type = MESSAGE_T__C_TYPE__CT_VALUE;
        if (data == NULL)
        {
            resp->data_size = 0;
            resp->data = NULL;
        }
        else
        {
            resp->data_size = data->datasize;
            resp->data = data->data;
        }
        msg->msg_t = resp;
        return 0;
    }
    else if (opcode == MESSAGE_T__OPCODE__OP_GETKEYS)
    {
        if (ctype != MESSAGE_T__C_TYPE__CT_NONE)
        {
            msg->msg_t->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->msg_t->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return 0;
        }
        pthread_mutex_lock(&tree_lock);
        int tsize = tree_size(tree) + 1;
        char **a = tree_get_keys(tree);
        char **keys;
        pthread_mutex_unlock(&tree_lock);

        if (a[0] == NULL)
        {
            keys = malloc(2 * sizeof(char *));
            keys[0] = "NULL";
            keys[1] = "\0";
        }
        else
        {
            keys = malloc((tsize + 1) * sizeof(char *));
            int i;
            for (i = 0; i < tsize; i++)
            {

                if (a[i] != NULL)
                {

                    keys[i] = malloc(strlen(a[i]) + 1);
                    strcpy(keys[i], a[i]);
                }
                else
                {
                    keys[i] = "NULL";
                }
            }
        }
        tree_free_keys(a);

        resp->opcode = MESSAGE_T__OPCODE__OP_GETKEYS + 1;
        resp->c_type = MESSAGE_T__C_TYPE__CT_KEYS;

        message_t__free_unpacked(msg->msg_t, NULL);
        resp->keys = keys;
        resp->n_keys = tsize;
        msg->msg_t = resp;
        return 0;
    }
    else if (opcode == MESSAGE_T__OPCODE__OP_HEIGHT)
    {
        if (ctype != MESSAGE_T__C_TYPE__CT_NONE)
        {
            msg->msg_t->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->msg_t->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return 0;
        }
        pthread_mutex_lock(&tree_lock);
        int height = tree_height(tree);
        pthread_mutex_unlock(&tree_lock);
        resp->opcode = MESSAGE_T__OPCODE__OP_HEIGHT + 1;
        resp->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        resp->result = height;

        message_t__free_unpacked(msg->msg_t, NULL);
        msg->msg_t = resp;
        return 0;
    }
    else if (opcode == MESSAGE_T__OPCODE__OP_VERIFY)
    {
        if (ctype != MESSAGE_T__C_TYPE__CT_RESULT)
        {
            msg->msg_t->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->msg_t->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return 0;
        }
        int op_n = msg->msg_t->result;
        int vrf = verify(op_n);

        resp->opcode = MESSAGE_T__OPCODE__OP_VERIFY + 1;
        resp->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        resp->result = vrf;
        message_t__free_unpacked(msg->msg_t, NULL);
        msg->msg_t = resp;
        return 0;
    }
    else
    {
        resp->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        resp->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        message_t__free_unpacked(msg->msg_t, NULL);
        msg->msg_t = resp;
        return 0;
    }
}

int async_invoke(int op, char *key, void *data)
{

    if (op == 0)
    {
        char *key_aux = malloc(strlen(key) + 1);
        strcpy(key_aux, key);
        int del = tree_del(tree, key);
        if (del == -1)
        {
            return -1;
        }
        if (which_server == 1)
            rtree_del(other_tree, key_aux);
        return 0;
    }
    else
    {

        struct data_t *datat = data_create2(strlen(data) + 1, data);
        char *new_key = malloc(strlen(key) + 1);
        strcpy(new_key, key);

        struct data_t *data_aux = data_dup(datat);
        int ret;
        if ((ret = tree_put(tree, new_key, datat)) == 0)
        {
            if (which_server == 1)
                rtree_put(other_tree, entry_create(key, data_aux));
        }
        return ret;
    }
}

void *process_task(void *params)
{
    while (stop == 0)
    {

        pthread_mutex_lock(&queue_lock);
        while (queue_head == NULL && stop == 0)
        {
            pthread_cond_wait(&queue_not_empty, &queue_lock);
        }
        if (stop != 0)
        {
            pthread_mutex_unlock(&queue_lock);
            break;
        }
        pthread_mutex_lock(&tree_lock);

        //Buscar primeira task
        async_invoke(queue_head->op, queue_head->key, queue_head->data);

        TASK *aux = queue_head->next;
        free(queue_head->key);
        free(queue_head);
        queue_head = aux;
        op_count++;
        pthread_mutex_unlock(&queue_lock);
        pthread_mutex_unlock(&tree_lock);
    }
    return 0;
}

void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void *context)
{
    if (type == ZOO_SESSION_EVENT)
    {
        if (state == ZOO_CONNECTED_STATE)
        {
            is_on = 1;
        }
        else
        {
            is_on = 0;
        }
    }
}

static void children_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx)
{
    if (state == ZOO_CONNECTED_STATE)
    {
        if (type == ZOO_CHILD_EVENT)
        {
            struct String_vector *children_list = (struct String_vector *)malloc(sizeof(struct String_vector));
            if (ZOK != zoo_wget_children(zoo_handle, root, &children_watcher, watcher_ctx, children_list))
            {
                fprintf(stderr, "Erro a fazer watch dos filhos no watcher\n");
                exit(EXIT_FAILURE);
            }
            int primary_exists = zoo_exists(zoo_handle, primary, 0, NULL) != ZNONODE;
            int backup_exists = zoo_exists(zoo_handle, backup, 0, NULL) != ZNONODE;

            //both exist
            if (primary_exists && backup_exists)
            {
                //Nothing changed
                if (which_server == 1 && other_tree != NULL)
                {
                    free(children_list);
                    return;
                }
                //Is primary and backup joined
                else if (which_server == 1)
                {
                    //TODO review
                    int buf_len = MAX_IP_SIZE;
                    char *buffer = malloc(buf_len);
                    if (ZOK != zoo_get(zoo_handle, backup, 0, buffer, &buf_len, NULL))
                    {
                        fprintf(stderr, "Erro no zoo get do watcher\n");
                        free(children_list);
                        exit(EXIT_FAILURE);
                    }

                    char *adrr = malloc(30 * sizeof(char));
                    strcpy(adrr, "127.0.0.1:");
                    strcat(adrr, buffer);
                    other_tree = rtree_connect(adrr);
                }
                //Is backup, nothing changed
                else
                {
                    free(children_list);
                    return;
                }
            }
            //primary exists, backup not
            else if (primary_exists)
            {
                //was primary, backup left
                if (which_server == 1 && other_tree != NULL)
                { //TODO review
                    other_tree = NULL;
                }
                //nothing changed
                else if (which_server == 1)
                {
                    free(children_list);
                    return;
                }
                else
                {
                    fprintf(stderr, "Erro no watcher: primary but no backup\n");
                    free(children_list);
                    exit(EXIT_FAILURE);
                }
            }
            //backup exists but not primary
            else if (backup_exists)
            {
                //nothing changed
                if (which_server == 0 && other_tree != NULL)
                {
                    free(children_list);
                    return;
                }
                //primary shut off
                else if (which_server == 0)
                {
                    //remove from backup, put in primary
                    int buf_len = MAX_IP_SIZE;
                    char *buffer = malloc(buf_len);

                    if (ZOK != zoo_get(zoo_handle, backup, 0, buffer, &buf_len, NULL))
                    {
                        printf("Erro no zoo get do watcher\n");
                        free(children_list);
                        exit(EXIT_FAILURE);
                    }
                    zoo_delete(zoo_handle, backup, -1);
                    zoo_create(zoo_handle, primary, buffer, buf_len, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0);
                    which_server = 1;
                    other_tree = NULL;
                }
                else
                {
                    fprintf(stderr, "Erro no watcher: existe backup mas nao primario\n");
                    free(children_list);
                    exit(EXIT_FAILURE);
                }
            }
            //neither exists
            else
            {
                fprintf(stderr, "Erro no watcher: nenhum existe\n");
                free(children_list);
                exit(EXIT_FAILURE);
            }
            free(children_list);
            return;
        }
    }
}

void tree_skel_destroy();

/* Inicia o skeleton da árvore.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke(). 
 * Retorna 0 (OK) ou -1 (erro, por exemplo OUT OF MEMORY)
 */
int tree_skel_init(const char *port, const char *zoo_addr)
{
    if (zoo_addr == NULL || port == NULL)
        return -1;

    op_count = 0;
    last_assigned = 0;
    stop = 0;
    if ((tree = tree_create()) == NULL)
    {
        perror("Erro a criar a tree\n");
        return -1;
    }

    if (pthread_create(&new, NULL, process_task, (void *)&params) != 0)
    {
        perror("Erro a criar thread\n");
        return -1;
    }

    // printf("%d\n", zoo_exists(zoo_handle, root, 0, NULL) == ZNONODE);
    if ((zoo_handle = zookeeper_init(zoo_addr, &connection_watcher, 2000, 0, NULL, 0)) == NULL)
    {
        fprintf(stderr, "Erro a conectar ao servidor zookeeper\n");
        exit(EXIT_FAILURE);
    }

    sleep(2);
    if (is_on)
    {
        //nao houver kvstore
        if (zoo_exists(zoo_handle, root, 0, NULL) == ZNONODE)
        {
            fprintf(stderr, "ZNode kvstore nao existe. Criando um no\n");
            //criar root
            if (zoo_create(zoo_handle, root, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0) == ZOK)
            {
                fprintf(stderr, "Criado kvstore\n");
            }
            else
            {
                fprintf(stderr, "Erro a criar kvstore\n");
                exit(EXIT_FAILURE);
            }

            //criar primario
            if (zoo_create(zoo_handle, primary, port, strlen(port) + 1, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0) == ZOK)
            {
                fprintf(stderr, "Criado primary\n");
                which_server = 1;
            }
            else
            {
                fprintf(stderr, "Erro a criar primary\n");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            int is_primary = zoo_exists(zoo_handle, primary, 0, NULL) != ZNONODE;
            int is_backup = zoo_exists(zoo_handle, backup, 0, NULL) != ZNONODE;
            if (is_primary && is_backup)
            {
                fprintf(stderr, "Existe tanto primary como backup\n");
                tree_skel_destroy();
                exit(EXIT_FAILURE);
            }
            else if (!is_primary && !is_backup)
            {
                fprintf(stderr, "Nao existe nem primary nem backup\n");
                if (zoo_create(zoo_handle, primary, port, strlen(port) + 1, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0) == ZOK)
                {
                    fprintf(stderr, "Criado primary\n");
                    which_server = 1;
                }
                else
                {
                    fprintf(stderr, "Erro a criar primary\n");
                    exit(EXIT_FAILURE);
                }
            }
            else if (!is_backup)
            {
                if (zoo_create(zoo_handle, backup, port, strlen(port) + 1, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0) == ZOK)
                {
                    fprintf(stderr, "Criado backup\n");
                    which_server = 0;
                }
                else
                {
                    fprintf(stderr, "Erro a criar backup\n");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                fprintf(stderr, "Existe backup mas nao primary. Nao devia acontecer\n");
                exit(EXIT_FAILURE);
            }
        }

        struct String_vector *children_list = (struct String_vector *)malloc(sizeof(struct String_vector));

        if (ZOK != zoo_wget_children(zoo_handle, root, &children_watcher, watcher_ctx, children_list))
        {
            fprintf(stderr, "Erro a fazer watch dos filhos\n");
            exit(EXIT_FAILURE);
        }

        int buf_len = 100;
        char buffer[256] = "";
        other_tree = NULL;
        // if (which_server == 0)
        // {
        //     other_tree = NULL;
        // }
        // // get other server
        // else if (which_server == 1)
        // {
        //     if (ZOK != zoo_get(zoo_handle, backup, 0, buffer, &buf_len, NULL))
        //     {
        //         fprintf(stderr, "Erro no zoo get\n");
        //         exit(EXIT_FAILURE);
        //     }
        //     else
        //     {
        //         other_tree = rtree_connect(buffer);
        //     }
        // }
        free(children_list);
    }

    return 0;
}

void free_all_tasks()
{
    pthread_mutex_lock(&queue_lock);
    if (queue_head != NULL)
    {
        int i = 0;
        while (queue_head != NULL)
        {
            data_destroy(queue_head->data);
            free(queue_head->key);
            queue_head = queue_head->next;
            i++;
        }
        printf("Numero de tasks fechadas %d\n", i);
    }

    pthread_mutex_unlock(&queue_lock);
}

/* Liberta toda a memória e recursos alocados pela função tree_skel_init.
 */
void tree_skel_destroy()
{
    stop = -1;
    pthread_cond_signal(&queue_not_empty);
    pthread_join(new, NULL);
    free_all_tasks();
    tree_destroy(tree);
    tree = NULL;
    zookeeper_close(zoo_handle);
}

/* Executa uma operação na árvore (indicada pelo opcode contido em msg)
 * e utiliza a mesma estrutura message_t para devolver o resultado.
 * Retorna 0 (OK) ou -1 (erro, por exemplo, árvore nao incializada)
*/
int invoke(struct message_t *msg)
{
    if (tree == NULL)
        return -1;

    MessageT__Opcode opcode = msg->msg_t->opcode;

    if (opcode == MESSAGE_T__OPCODE__OP_DEL || opcode == MESSAGE_T__OPCODE__OP_PUT)
    {
        int op = opcode == MESSAGE_T__OPCODE__OP_DEL ? 0 : 1;

        char *key = malloc(msg->msg_t->key_size);
        strcpy(key, msg->msg_t->key);

        struct task_t *new_task = malloc(sizeof(struct task_t));
        new_task->next = NULL;
        new_task->op_n = last_assigned;
        new_task->op = op;
        if (msg->msg_t->data_size == 0)
        {
            new_task->data = NULL;
        }
        else
        {
            void *data = malloc(msg->msg_t->data_size);
            memcpy(data, msg->msg_t->data, msg->msg_t->data_size);
            new_task->data = data;
        }
        new_task->key = key;
        pthread_mutex_lock(&queue_lock);
        if (queue_head == NULL)
        {
            queue_head = new_task;
            pthread_cond_signal(&queue_not_empty); //queue no longer empty
        }
        else
        {
            struct task_t *aux = queue_head;
            while (aux->next != NULL)
            {
                aux = aux->next;
            }

            aux->next = new_task;
        }

        msg->msg_t->opcode = opcode + 1;
        msg->msg_t->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->msg_t->result = last_assigned;
        last_assigned++;

        pthread_mutex_unlock(&queue_lock);
        return 0;
    }
    else
    {
        return real_time_invoke(msg);
    }
}
