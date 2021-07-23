/*
GRUPO 54
Sara Queimado 52806
Diogo Fernandes 52802
Eduardo Salvadinha 52768
*/

#include "tree-private.h"
#include "tree.h"
#include "message-private.h"
#include "tree_skel.h"
#include "network_server.h"
#include "server_private.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

//O servidor concretiza uma árvore binária de pesquisa que pode ser acedida no porto definido
//na linha de comando. O servidor deverá suportar apenas um cliente de cada vez. Como o
//servidor apenas necessita de interagir com um cliente de cada vez, não será necessário, nesta
//fase, recorrer à criação de processos ligeiros (threads) para atendimento simultâneo de vários
//clientes.
int sockfd;
void intHandler(int dummy)
{
    printf("\nServer shutting down\n");
    network_server_close();
    tree_skel_destroy();
    exit(0);
}

int main(int argc, char const *argv[])
{
    struct sigaction sig = {0};
    sig.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sig, NULL);
    signal(SIGINT, intHandler);

    // Verifica se foi passado algum argumento
    if (argc != 3)
    {
        printf("Uso: ./tree-server <porto_servidor> <ip>:<porta>\n");
        // printf("Exemplo de uso: ./tree-server 12345 \n");
        return -1;
    }
    // Cria socket TCP
    sockfd = network_server_init(atoi(argv[1]));
    // printf("Servidor a espera de dados\n");

    if (sockfd == -1)
    {
        perror("Erro a criar socket");
        close(sockfd);
        return -1;
    }
    if (tree_skel_init(argv[1], argv[2]) == -1)
    {
        perror("Erro ao iniciar tree_skel");
        close(sockfd);
        return -1;
    }
    printf("-----------------\n   Server ON          \n-----------------\n");
    // Bloqueia a espera de pedidos de conexão
    if (network_main_loop(sockfd) == -1)
    {
        perror("Erro no network loop");
        tree_skel_destroy();
        close(sockfd);
        return -1;
    }

    // Fecha socket
    tree_skel_destroy();
    close(sockfd);
    return 0;
}