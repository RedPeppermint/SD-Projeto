/*
GRUPO 54
Sara Queimado 52806
Diogo Fernandes 52802
Eduardo Salvadinha 52768
*/
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <poll.h>
#include "tree_skel.h"
#include "message-private.h"
#include "network_server.h"
#define n_clients 20

int sockfd;
struct sockaddr_in client;
socklen_t size_client;
struct pollfd connections[n_clients];
int nfds = 1;
struct message_t *msg;

void print_sockets()
{
    for (size_t i = 0; i < n_clients; i++)
    {
        printf("SOCK %ld - %d\n", i, connections[i].fd);
    }
}

/* Função para preparar uma socket de receção de pedidos de ligação
 * num determinado porto.
 * Retornar descritor do socket (OK) ou -1 (erro).
 */
int network_server_init(short port)
{
    // signal(SIGPIPE, SIG_IGN);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;
    if (sockfd < 0)
    {
        perror("Erro ao criar socket no servidor");
        return -1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Erro ao fazer bind");
        close(sockfd);
        return -1;
    }

    int option = 1;
    if (setsockopt(sockfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char *)&option, sizeof(option)) < 0)
    {
        perror("Erro no setsockopt");
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, 0) < 0)
    {
        perror("Erro ao executar listen");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

/* Esta função deve:
 * - Ler os bytes da rede, a partir do client_socket indicado;
 * - De-serializar estes bytes e construir a mensagem com o pedido,
 *   reservando a memória necessária para a estrutura message_t.
 */
struct message_t *network_receive(int client_socket)
{

    struct message_t *msg = malloc(sizeof(struct message_t));
    if (msg == NULL)
    {
        return NULL;
    }
    msg->msg_t = receive_all(client_socket);
    if (msg->msg_t == NULL)
    {
        free(msg);
        perror("MESSAGE NULL\n");
        return NULL;
    }
    return msg;
}

void deleteMsg(struct message_t *msg)
{
    if (msg != NULL)
    {
        message_t__free_unpacked(msg->msg_t, NULL);
        free(msg);
    }
}

/* Esta função deve:
 * - Serializar a mensagem de resposta contida em msg;
 * - Libertar a memória ocupada por esta mensagem;
 * - Enviar a mensagem serializada, através do client_socket.
 */
int network_send(int client_socket, struct message_t *msg)
{
    int sent = send_all(client_socket, msg->msg_t);
    return sent;
}

/* A função network_server_close() liberta os recursos alocados por
 * network_server_init().
 */
int network_server_close()
{
    int c = close(sockfd);
    int i;
    for (i = 0; i < n_clients; i++)
    {
        int sock = connections[i].fd;
        if (sock != -1)
        {
            c |= close(sock);
            connections[i].fd = -1;
        }
    }
    nfds = 0;
    return c;
}

/* Esta função deve:
 * - Aceitar uma conexão de um cliente;
 * - Receber uma mensagem usando a função network_receive;
 * - Entregar a mensagem de-serializada ao skeleton para ser processada;
 * - Esperar a resposta do skeleton;
 * - Enviar a resposta ao cliente usando a função network_send.
 */
int network_main_loop(int listening_socket)
{
    size_client = sizeof(client);

    int i;
    for (i = 0; i < n_clients; i++)
    {
        connections[i].fd = -1;
    }
    connections[0].fd = listening_socket;
    connections[0].events = POLLIN;
    int kfds;
    while ((kfds = poll(connections, n_clients, 1)) >= 0)
    {
        if (kfds > 0)
        {
            if ((connections[0].revents & POLLIN) && (nfds < n_clients))
                if ((connections[nfds].fd = accept(connections[0].fd, (struct sockaddr *)&client, &size_client)) > 0)
                {
                    connections[nfds].events = POLLIN;
                    nfds++;
                    printf("Novo cliente detectado | Numero de Clientes: %d\n", nfds - 1);
                }
            int i;
            for (i = 1; i < n_clients; i++)
            {
                if (connections[i].revents == POLLIN && connections[i].fd != -1)
                {

                    msg = network_receive(connections[i].fd);

                    if (msg == NULL || msg->msg_t == NULL || msg->msg_t->opcode == 0)
                    {
                        deleteMsg(msg);
                        msg = NULL;
                        if (nfds == 1)
                            continue;
                        printf("Cliente terminou a ligacao | Numero de Clientes: %d\n", nfds - 2);
                        close(connections[i].fd);
                        connections[i].fd = -1;
                        nfds--;
                        continue;
                    }

                    printf("-----------------\n");
                    printf("Receiving\n");

                    printf("-----------------\n");
                    printf("MESSAGE:\n");
                    printf("OPCODE %d\n", msg->msg_t->opcode);
                    printf("CTYPE %d\n", msg->msg_t->c_type);
                    printf("KEY %s\n", msg->msg_t->key);
                    printf("KEYSIZE %d\n", msg->msg_t->key_size);
                    printf("DATA %s\n", msg->msg_t->data);
                    printf("DATASIZE %d\n", msg->msg_t->data_size);
                    printf("RESULT %d\n", msg->msg_t->result);
                    printf("KEYS ");
                    printkeysServer(msg->msg_t->keys);
                    printf("-----------------\n");

                    printf("Invoking\n");
                    if (invoke(msg) == -1)
                    {
                        deleteMsg(msg);

                        perror("Erro invoke");
                        close(connections[i].fd);
                        connections[i].fd = -1;
                        nfds--;
                        continue;
                    }

                    printf("-----------------\n");
                    printf("OPCODE %d\n", msg->msg_t->opcode);
                    printf("CTYPE %d\n", msg->msg_t->c_type);
                    printf("KEY %s\n", msg->msg_t->key);
                    printf("KEYSIZE %d\n", msg->msg_t->key_size);
                    printf("DATA %s\n", msg->msg_t->data);
                    printf("DATASIZE %d\n", msg->msg_t->data_size);
                    printf("RESULT %d\n", msg->msg_t->result);
                    printf("KEYS ");
                    printkeysClient(msg->msg_t->keys);
                    printf("\n-----------------\n");

                    printf("Sending\n\n");
                    printf("-----------------\n");
                    int sent = network_send(connections[i].fd, msg);
                    if (sent == -1)
                    {
                        perror("Erro network_send");
                        close(connections[i].fd);
                        connections[i].fd = -1;
                        nfds--;
                        continue;
                    }
                    free(msg->msg_t);
                    free(msg);
                    //msg = NULL;
                }
                else if (connections[i].revents == POLLHUP)
                {
                    deleteMsg(msg);
                    msg = NULL;
                    printf("POLLLHUP Cliente terminou a ligacao | Numero de Clientes: %d\n", nfds - 2);
                    close(connections[i].fd);
                    connections[i].fd = -1;
                    nfds--;
                }
            }
        }
    }
    deleteMsg(msg);
    printf("Closing server\n");
    return 0;
}
