/*
GRUPO 54
Sara Queimado 52806
Diogo Fernandes 52802
Eduardo Salvadinha 52768
*/

#ifndef _CLIENT_STUB_PRIVATE_H
#define _CLIENT_STUB_PRIVATE_H

#include <netinet/in.h>

struct rtree_t
{
    char *address;
    int sockfd;
    struct sockaddr_in server;
    char **keys;
};

#endif