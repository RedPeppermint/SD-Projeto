/*
GRUPO 54
Sara Queimado 52806
Diogo Fernandes 52802
Eduardo Salvadinha 52768
*/

#ifndef _MESSAGE_PRIVATE_H
#define _MESSAGE_PRIVATE_H

#include "sdmessage.pb-c.h"
#include <unistd.h>
#define MAX_MSG 2048

struct message_t
{
    struct _MessageT *msg_t;
};

int ignsigpipe();

int write_all(int sock, void *buf, int len);

int read_all(int sock, void *buf, int bufsize);

MessageT *receive_all(int sockfd);

int send_all(int sockfd, MessageT *message);

void printkeysClient(char **keys);

void printkeysServer(char **keys);

void free_keys_msg(char **keys);
#endif
