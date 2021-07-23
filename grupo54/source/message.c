#include "message-private.h"
#include "sdmessage.pb-c.h"
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
#define TIMEOUT 10
struct message_t;

int write_all(int sock, void *buf, int len)
{
    if (sock <= 0)
    {
        perror("Socket invalido\n");
        return -1;
    }
    int bufsize = len;
    while (len > 0)
    {
        int res = write(sock, buf, len);
        if (res < 0)
        {

            if (errno == EINTR)
            {
                continue;
            }
            perror("Erro escrita");
            return res;
        }
        buf += res;
        len -= res;
    }
    return bufsize;
}

//return message len
int read_all(int sock, void *buf, int bufsize)
{
    if (sock <= 0)
    {
        perror("Socket invalido\n");
        return -1;
    }
    int att = 0;
    int len = bufsize;
    while (bufsize > 0)
    {
        if (att == TIMEOUT)
        {
            return 0;
        }
        int nbytes;
        if ((nbytes = read(sock, buf, bufsize)) < 0)
        {
            if (errno == EINTR)
            {
                att++;
                continue;
            }
            perror("Erro leitura");
            return nbytes;
        }
        else if (nbytes == 0)
            //EOF
            break;

        buf += nbytes;
        bufsize -= nbytes;
    }
    return len;
}

MessageT *receive_all(int sockfd)
{

    if (sockfd <= 0)
        return NULL;

    int msg_size = 0;

    int size = read_all(sockfd, &msg_size, sizeof(int));

    //receive buf size
    if (size != sizeof(int))
    {

        perror("Erro a receber o tamanho da mensagem");
        return NULL;
    }

    msg_size = ntohl(msg_size);
    void *buf = malloc(msg_size);
    size = read_all(sockfd, buf, msg_size);
    //receive buf

    if (size != msg_size)
    {

        close(sockfd);
        return NULL;
    }

    MessageT *msg = message_t__unpack(NULL, msg_size, buf);

    if (msg == NULL)
    {
        message_t__free_unpacked(msg, NULL);
        free(buf);
        return NULL;
    }

    free(buf);
    return msg;
}

int send_all(int sockfd, MessageT *msg)
{

    if (sockfd <= 0 || msg == NULL)
    {
        return -1;
    }

    int msg_size = message_t__get_packed_size(msg);
    if (msg_size <= 0)
    {
        perror("Tamanho da mensagem invalido (get packed size)");
        return -1;
    }

    void *buf = malloc(msg_size);
    int buf_size = message_t__pack(msg, buf);
    int buf_size_network = htonl(buf_size);
    if (buf_size_network == -1)
    {
        message_t__free_unpacked(msg, NULL);
        perror("Erro ao serializar o buf_size");
        return -1;
    }

    if (msg->data_size > 0)
    {
        free(msg->data);
    }

    int size = write_all(sockfd, &(buf_size_network), sizeof(buf_size_network));

    //send buf size
    if (size != sizeof(buf_size_network))
    {
        message_t__free_unpacked(msg, NULL);
        perror("Erro ao enviar tamanho da mensagem\n");
        close(sockfd);
        return -1;
    }

    free(msg->keys);
    size = write_all(sockfd, buf, buf_size);
    //send buf
    if (size != buf_size)
    {
        message_t__free_unpacked(msg, NULL);
        perror("Erro ao enviar dados\n");
        close(sockfd);
        return -1;
    }

    free(buf);
    return buf_size;
}

void printkeysServer(char **keys)
{
    if (keys == NULL)
    {
        printf("\n");
        return;
    }

    if (keys[0] == NULL)
    {
        printf("Arvore Vazia\n");
        return;
    }

    int i = 0;
    while (keys[i] != NULL)
    {
        printf("%s ", keys[i]);
        i++;
    }
    printf("null %s \n", keys[i]);
}

void printkeysClient(char **keys)
{
    if (keys == NULL)
    {
        printf("\n");
        return;
    }
    if (strcmp(keys[0], "NULL") == 0)
    {
        printf("Arvore Vazia\n");
        return;
    }

    int i = 0;
    while (strcmp(keys[i], "NULL") != 0)
    {

        printf("%s ", keys[i]);
        i++;
    }
}
void free_keys_msg(char **keys)
{
    if (keys == NULL || strcmp(keys[0], "NULL") == 0)
    {
        free(keys);
        return;
    }
    int i = 0;
    while (strcmp(keys[i], "NULL") != 0)
    {

        free(keys[i]);
        i++;
    }
    free(keys);
}