/*
GRUPO 54
Sara Queimado 52806
Diogo Fernandes 52802
Eduardo Salvadinha 52768
*/

//Main
// put<key><data>
// get<key>
// del<key>
// size
// height
// getkeys
// quit int

#include "client_stub.h"
#include "client_stub_private.h"
#include "message-private.h"
#include "network_client.h"
#include "data.h"
#include "entry.h"
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <zookeeper/zookeeper.h>

struct rtree_t *backup;
struct rtree_t *primary;
static zhandle_t *zoo_handle;
static char *root = "/kvstore";
static char *primary_path = "/kvstore/primary";
static char *backup_path = "/kvstore/backup";
static char *watcher_ctx = "ZooKeeper Data Watcher";

pthread_mutex_t stop_lock = PTHREAD_MUTEX_INITIALIZER;

int is_on;
static int stop = -1;

#define MAX_IP_SIZE 30

void getIps();

void intHandler(int dummy)
{
    printf("\nClient shutting down\n");
    rtree_disconnect(backup);
    exit(0);
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
                fprintf(stderr, "Erro a fazer watch dos filhos\n");
                exit(EXIT_FAILURE);
            }
            int primary_exists = zoo_exists(zoo_handle, primary_path, 0, NULL) != ZNONODE;
            int backup_exists = zoo_exists(zoo_handle, backup_path, 0, NULL) != ZNONODE;

            if (!primary_exists)
            {
                printf("\nPrimary stopped \n");
                pthread_mutex_lock(&stop_lock);
                stop = 1;
                pthread_mutex_unlock(&stop_lock);
                return;
            }

            if (!backup_exists)
            {
                printf("\nBackup stopped \n");
                pthread_mutex_lock(&stop_lock);
                stop = 1;
                pthread_mutex_unlock(&stop_lock);
                return;
            }
            getIps();
            pthread_mutex_lock(&stop_lock);
            stop = 0;
            pthread_mutex_unlock(&stop_lock);
        }
    }
}

void getIps()
{

    if (backup != NULL)
    {
        rtree_disconnect(backup);
    }

    if (primary != NULL)
    {
        rtree_disconnect(primary);
    }

    int bufferlen = 100;
    char *buffer = malloc(10 * sizeof(char));

    zoo_get(zoo_handle, primary_path, &children_watcher, buffer, &bufferlen, NULL);

    char *ip1 = malloc(30 * sizeof(char));
    strcpy(ip1, "127.0.0.1:");
    strcat(ip1, buffer);

    free(buffer);

    char *buffer2 = malloc(10 * sizeof(char));

    zoo_get(zoo_handle, backup_path, &children_watcher, buffer2, &bufferlen, NULL);
    char *ip2 = malloc(30 * sizeof(char));
    strcpy(ip2, "127.0.0.1:");
    strcat(ip2, buffer2);

    free(buffer2);

    if ((backup = rtree_connect(ip2)) == NULL)
    {
        perror("Erro ao criar a backup");
        exit(EXIT_FAILURE);
    }

    if ((primary = rtree_connect(ip1)) == NULL)
    {
        perror("Erro ao criar a primary");
        exit(EXIT_FAILURE);
    }
}

//tree-client <server>:<port> <zookeeper_port>
int main(int argc, char const *argv[])
{
    struct sigaction sig = {0};
    sig.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sig, NULL);
    signal(SIGINT, intHandler);
    struct String_vector *children_list = (struct String_vector *)malloc(sizeof(struct String_vector));
    /* Testar os argumentos de entrada */
    /* inicialização da camada de rede */
    if (argc != 3)
    {
        perror("Usage: ./tree-client <server>:<port> <zookeeper_Port>\n");
        return -1;
    }

    char *zoo_addr[MAX_IP_SIZE];

    strcpy(zoo_addr, "127.0.0.1:");
    strcat(zoo_addr, argv[2]);

    zoo_handle = zookeeper_init(zoo_addr, &connection_watcher, 2000, 0, NULL, 0);
    if (zoo_handle == NULL)
    {
        printf("Error while connecting ZooKeeper\n");
        return -1;
    }

    if (ZOK != zoo_wget_children(zoo_handle, root, &children_watcher, watcher_ctx, children_list))
    {
        fprintf(stderr, "Erro a fazer watch dos filhos\n");
        exit(EXIT_FAILURE);
    }

    printf("Getting ips\n");
    getIps();

    printf("CONNECTADO AO SERVIDOR\n");
    for (;;)
    {

        printf("Inserir comandos>");
        char buffer[32];
        char *b = buffer;
        size_t bufsize = 32;
        int len = getline(&b, &bufsize, stdin);
        pthread_mutex_lock(&stop_lock);
        if (stop == 1)
        {
            pthread_mutex_unlock(&stop_lock);
            printf("Waiting for servers...\n");
            continue;
        }
        pthread_mutex_unlock(&stop_lock);
        //deletes '\n'
        if ((buffer)[len - 1] == '\n')
        {
            (buffer)[len - 1] = '\0';
        }

        char s[2] = " ";
        char *arg1;
        if ((arg1 = strtok(buffer, s)) == NULL)
        {
            printf("No commands found\n");
            continue;
        }

        if (strcmp(arg1, "put") == 0)
        {
            char *arg2;
            char *arg3;
            if ((arg2 = strtok(NULL, s)) == NULL)
            {
                printf("Usage put <key> <data>\n");
                continue;
            }
            if ((arg3 = strtok(NULL, s)) == NULL)
            {
                printf("Usage put <key> <data>\n");
                continue;
            }

            struct data_t *data = data_create2(strlen(arg3) + 1, arg3);
            if (data == NULL)
            {
                printf("Erro ao criar data\n");
                continue;
            }
            struct entry_t *entry = entry_create(arg2, data);
            if (entry == NULL)
            {
                printf("Erro ao criar entry\n");
                continue;
            }
            int result;
            if ((result = rtree_put(primary, entry)) == -1)
            {
                printf("Erro ao fazer put\n");
                continue;
            }
            free(data);
            free(entry);
            printf("PUT Pedido\n");
        }

        else if (strcmp(arg1, "get") == 0)
        {
            char *arg2;
            if ((arg2 = strtok(NULL, s)) == NULL)
            {
                printf("Usage get <key>\n");
                continue;
            }
            struct data_t *data;
            data = rtree_get(backup, arg2);

            printf("GET Efectuado\n");
            if (data == NULL)
            {
                printf("Key not found\n");
            }
            else
            {
                printf("%s\n", (char *)data->data);
                free(data->data);
                free(data);
            }
        }

        else if (strcmp(arg1, "del") == 0)
        {
            char *arg2;
            if ((arg2 = strtok(NULL, s)) == NULL)
            {
                printf("Usage del <key>\n");
                continue;
            }
            int result;
            if ((result = rtree_del(primary, arg2)) == -1)
            {
                printf("Erro ao apagar\n");
                continue;
            }
            printf("DELETE Pedido\n");
        }
        else if (strcmp(arg1, "size") == 0)
        {
            int size;
            if ((size = rtree_size(backup)) == -1)
            {
                printf("Erro no size\n");
                continue;
            }

            printf("Size = %d\n", size);
        }
        else if (strcmp(arg1, "height") == 0)
        {
            int height;
            if ((height = rtree_height(backup)) == -1)
            {
                printf("Erro no height\n");
                continue;
            }
            printf("HEIGHT = %d\n", height);
        }
        else if (strcmp(arg1, "getkeys") == 0)
        {
            char **keys = rtree_get_keys(backup);
            if (keys == NULL)
            {
                printf("Arvore vazia\n");
                continue;
            }
            if (keys != NULL)
            {
                printkeysClient(keys);
                printf("\n");
                rtree_free_keys(keys);
            }
        }
        else if (strcmp(arg1, "verify") == 0)
        {
            char *arg2;
            if ((arg2 = strtok(NULL, s)) == NULL)
            {
                printf("Usage verify <op_n>\n");
                continue;
            }
            int ver;
            if ((ver = rtree_verify(backup, atoi(arg2))) == -1)
            {
                perror("Erro no verify\n");
                continue;
            }
            printf("%s\n", ver != 0 ? "Operacao realizada" : "Operacao ainda nao realizada");
        }
        else if (strcmp(arg1, "quit") == 0)
        {
            printf("Client Shutting Down\n");
            break;
        }
        else
        {
            printf("Command %s not found\n", arg1);
            continue;
        }
    }

    if (rtree_disconnect(backup) == -1)
    {
        perror("Erro ao desconectar a backup");
        return -1;
    }
    return 0;
}
