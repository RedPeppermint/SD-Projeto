/*
GRUPO 54
Sara Queimado 52806
Diogo Fernandes 52802
Eduardo Salvadinha 52768
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include "client_stub.h"
#include "sdmessage.pb-c.h"
#include "client_stub_private.h"
#include "message-private.h"

/* Esta função deve:
 * - Obter o endereço do servidor (struct sockaddr_in) a base da
 *   informação guardada na estrutura rtree;
 * - Estabelecer a ligação com o servidor;
 * - Guardar toda a informação necessária (e.g., descritor do socket)
 *   na estrutura rtree;
 * - Retornar 0 (OK) ou -1 (erro).
 */
int network_connect(struct rtree_t *rtree)
{
  if (rtree == NULL || rtree->sockfd <= 0)
    return -1;
  if (connect(rtree->sockfd, (struct sockaddr *)&(rtree->server), sizeof(rtree->server)) == -1)
  {
    close(rtree->sockfd);
    return -1;
  }

  return 0;
}

/* Esta função deve:
 * - Obter o descritor da ligação (socket) da estrutura rtree_t;
 * - Serializar a mensagem contida em msg;
 * - Enviar a mensagem serializada para o servidor;
 * - Esperar a resposta do servidor;
 * - De-serializar a mensagem de resposta;
 * - Retornar a mensagem de-serializada ou NULL em caso de erro.
 */
struct message_t *network_send_receive(struct rtree_t *rtree, struct message_t *msg)
{
  int sockfd = rtree->sockfd;

  int checkbytes = send_all(sockfd, msg->msg_t);
  if (checkbytes == -1)
  {
    return NULL;
  }

  printf("A espera de resposta...\n");
  MessageT *resp_msg = receive_all(sockfd);

  if (resp_msg == NULL)
  {
    close(rtree->sockfd);
    return NULL;
  }

  msg->msg_t = resp_msg;
  return msg;
}

/* A função network_close() fecha a ligação estabelecida por
 * network_connect().
 */
int network_close(struct rtree_t *rtree)
{
  if (rtree == NULL)
    return -1;
  int sockfd = rtree->sockfd;

  shutdown(sockfd, SHUT_RDWR);
  int closed = close(rtree->sockfd);
  return closed;
}
