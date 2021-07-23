/*
GRUPO 54
Sara Queimado 52806
Diogo Fernandes 52802
Eduardo Salvadinha 52768
*/

#include "data.h"
#include "entry.h"
#include "client_stub.h"
#include "client_stub_private.h"
#include "network_client.h"
#include "sdmessage.pb-c.h"
#include "message-private.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

/* Função para estabelecer uma associação entre o cliente e o servidor,
 * em que address_port é uma string no formato <hostname>:<port>.
 * Retorna NULL em caso de erro.
 */
struct rtree_t *rtree_connect(const char *address_port)
{
  char *p;
  p = malloc(strlen(address_port) + 1);
  strcpy(p, address_port);
  // printf("ADDRESS PORT %s\n", address_port);
  const char s[2] = ":";
  char *hostname = strtok(p, s);
  char *port = strtok(NULL, s);
  // printf("STRINGS: %s:%s\n", hostname, port);

  int sockfd;
  struct sockaddr_in server;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("Erro ao criar a socket");
    return NULL;
  }
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_family = AF_INET;
  server.sin_port = htons(atoi(port));
  int a;
  if ((a = inet_pton(AF_INET, hostname, &server.sin_addr)) < 1)
  {
    perror("Erro ao converter IP");
    close(sockfd);
    return NULL;
  }
  free(p);
  struct rtree_t *tree = malloc(sizeof(struct rtree_t));
  tree->server = server;
  tree->sockfd = sockfd;
  if (network_connect(tree) < 0)
  {
    perror("Erro ao conectar-se ao servidor");
    close(sockfd);
    return NULL;
  }
  return tree;
}

/* Termina a associação entre o cliente e o servidor, fechando a
 * ligação com o servidor e libertando toda a memória local.
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int rtree_disconnect(struct rtree_t *rtree)
{

  if (rtree == NULL)
  {
    return -1;
  }
  else
  {
    int res = network_close(rtree);
    free(rtree);
    return res;
  }
}

/* Função para adicionar um elemento na árvore.
 * Se a key já existe, vai substituir essa entrada pelos novos dados.
 * Devolve 0 (ok, em adição/substituição) ou -1 (problemas).
 */
int rtree_put(struct rtree_t *rtree, struct entry_t *entry)
{

  MessageT *msg = malloc(sizeof(MessageT));
  message_t__init(msg);

  msg->opcode = MESSAGE_T__OPCODE__OP_PUT;
  msg->c_type = MESSAGE_T__C_TYPE__CT_ENTRY;

  msg->key_size = strlen(entry->key) + 1;
  msg->key = malloc(msg->key_size);
  strcpy(msg->key, entry->key);

  msg->data_size = entry->value->datasize;
  msg->data = malloc(msg->data_size);
  memcpy(msg->data, entry->value->data, msg->data_size);
  //Pedido resposta
  struct message_t *msg_resposta;
  struct message_t *msg_pedido = malloc(sizeof(struct message_t));
  msg_pedido->msg_t = msg;

  msg_resposta = network_send_receive(rtree, msg_pedido);

  if (msg_resposta->msg_t->opcode == MESSAGE_T__OPCODE__OP_ERROR)
  {
    message_t__free_unpacked(msg_pedido->msg_t, NULL);
    free(msg_pedido);
    return -1;
  }
  else
  {
    int result = msg_resposta->msg_t->result;
    message_t__free_unpacked(msg_pedido->msg_t, NULL);
    free(msg_pedido);
    free(msg->key);
    free(msg);
    return result;
  }
}

/* Função para obter um elemento da árvore.
 * Em caso de erro, devolve NULL.
 */
struct data_t *rtree_get(struct rtree_t *rtree, char *key)
{
  MessageT *msg = malloc(sizeof(MessageT));
  message_t__init(msg);
  msg->opcode = MESSAGE_T__OPCODE__OP_GET;
  msg->c_type = MESSAGE_T__C_TYPE__CT_KEY;

  msg->key_size = strlen(key) + 1;
  msg->key = malloc(msg->key_size);
  memcpy(msg->key, key, msg->key_size);

  //Pedido resposta
  struct message_t *msg_resposta;
  struct message_t *msg_pedido = malloc(sizeof(struct message_t));
  msg_pedido->msg_t = msg;
  msg_resposta = network_send_receive(rtree, msg_pedido);

  if (msg_resposta->msg_t->opcode == MESSAGE_T__OPCODE__OP_ERROR)
  {
    message_t__free_unpacked(msg_pedido->msg_t, NULL);
    free(msg->key);
    free(msg);
    free(msg_pedido);
    return NULL;
  }
  else
  {

    if (msg_resposta->msg_t->data_size == 0)
    {
      free(msg->key);
      free(msg);
      free(msg_pedido);
      return data_create2(0, NULL);
    }

    void *data = malloc(msg_resposta->msg_t->data_size);
    memcpy(data, msg_resposta->msg_t->data, msg_resposta->msg_t->data_size);
    struct data_t *datat = data_create2(msg_resposta->msg_t->data_size, data);
    message_t__free_unpacked(msg_pedido->msg_t, NULL);
    free(msg->key);
    free(msg);
    free(msg_pedido);
    return datat;
  }
}

/* Função para remover um elemento da árvore. Vai libertar
 * toda a memoria alocada na respetiva operação rtree_put().
 * Devolve: 0 (ok), -1 (key not found ou problemas).
 */
int rtree_del(struct rtree_t *rtree, char *key)
{
  MessageT *msg = malloc(sizeof(MessageT));
  message_t__init(msg);
  msg->opcode = MESSAGE_T__OPCODE__OP_DEL;
  msg->c_type = MESSAGE_T__C_TYPE__CT_KEY;
  //error entry undeclared
  // int data_size = strlen(entry);
  // msg->data_size = data_size;
  // char *aux = malloc(data_size + 1);
  // strcpy(aux, key);
  // msg->data = aux;

  msg->key_size = strlen(key) + 1;
  msg->key = malloc(msg->key_size);
  memcpy(msg->key, key, msg->key_size);

  struct message_t *msg_resposta;
  struct message_t *msg_pedido = malloc(sizeof(struct message_t));
  msg_pedido->msg_t = msg;
  msg_resposta = network_send_receive(rtree, msg_pedido);

  if (msg_resposta->msg_t->opcode == MESSAGE_T__OPCODE__OP_ERROR)
  {
    message_t__free_unpacked(msg_pedido->msg_t, NULL);
    free(msg->key);
    free(msg);
    free(msg_pedido);
    return -1;
  }
  else
  {
    int result = msg_resposta->msg_t->result;
    message_t__free_unpacked(msg_pedido->msg_t, NULL);
    free(msg->key);
    free(msg);
    free(msg_pedido);
    return result;
  }
}

/* Devolve o número de elementos contidos na árvore.
 */
int rtree_size(struct rtree_t *rtree)
{
  MessageT *msg = malloc(sizeof(MessageT));
  message_t__init(msg);
  msg->opcode = MESSAGE_T__OPCODE__OP_SIZE;
  msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

  struct message_t *msg_resposta;
  struct message_t *msg_pedido = malloc(sizeof(struct message_t));
  msg_pedido->msg_t = msg;
  msg_resposta = network_send_receive(rtree, msg_pedido);

  int result = msg_resposta->msg_t->result;

  if (msg_resposta->msg_t->opcode == MESSAGE_T__OPCODE__OP_ERROR)
  {
    message_t__free_unpacked(msg_pedido->msg_t, NULL);
    free(msg);
    free(msg_pedido);
    return -1;
  }
  else
  {
    message_t__free_unpacked(msg_pedido->msg_t, NULL);
    free(msg);
    free(msg_pedido);
    return result;
  }
}

/* Função que devolve a altura da árvore.
 */
int rtree_height(struct rtree_t *tree)
{
  MessageT *msg = malloc(sizeof(MessageT));
  message_t__init(msg);
  msg->opcode = MESSAGE_T__OPCODE__OP_HEIGHT;
  msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

  struct message_t *msg_resposta;
  struct message_t *msg_pedido = malloc(sizeof(struct message_t));
  msg_pedido->msg_t = msg;
  msg_resposta = network_send_receive(tree, msg_pedido);

  int result = msg_resposta->msg_t->result;

  if (msg_resposta->msg_t->opcode == MESSAGE_T__OPCODE__OP_ERROR)
  {
    message_t__free_unpacked(msg_pedido->msg_t, NULL);
    free(msg);
    free(msg_pedido);
    return -1;
  }
  else
  {
    message_t__free_unpacked(msg_pedido->msg_t, NULL);
    free(msg);
    free(msg_pedido);
    return result;
  }
}

int copy_aux(char **dest, char **src, int size)
{
  int i;
  for (i = 0; i < size; i++)
  {

    if (strcmp(src[i], "NULL") != 0)
    {

      dest[i] = malloc(strlen(src[i]) + 1);
      strcpy(dest[i], src[i]);
    }
    else
    {

      dest[i] = "NULL";
    }
  }
  return 0;
}
/* Devolve um array de char* com a cópia de todas as keys da árvore,
 * colocando um último elemento a NULL.
 */
char **rtree_get_keys(struct rtree_t *rtree)
{
  MessageT *msg = malloc(sizeof(MessageT));
  message_t__init(msg);
  msg->opcode = MESSAGE_T__OPCODE__OP_GETKEYS;
  msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;

  struct message_t *msg_resposta;
  struct message_t *msg_pedido = malloc(sizeof(struct message_t));
  msg_pedido->msg_t = msg;
  msg_resposta = network_send_receive(rtree, msg_pedido);

  int size = msg_resposta->msg_t->n_keys;

  if (msg_resposta->msg_t->keys == NULL)
  {

    message_t__free_unpacked(msg, NULL);
    free(msg);
    free(msg_pedido);
    return NULL;
  }

  if (msg_resposta->msg_t->opcode == MESSAGE_T__OPCODE__OP_ERROR)
  {
    message_t__free_unpacked(msg_pedido->msg_t, NULL);
    free(msg);
    free(msg_pedido);
    return NULL;
  }
  else
  {
    char **keys = malloc(size * sizeof(char *) + sizeof(NULL));
    if (copy_aux(keys, msg_resposta->msg_t->keys, size) == -1)
    {
      message_t__free_unpacked(msg, NULL);
      free(msg);
      free(msg_pedido);
      rtree_free_keys(keys);
      return NULL;
    }
    message_t__free_unpacked(msg_pedido->msg_t, NULL);
    free(msg);
    free(msg_pedido);
    return keys;
  }
}

/* Liberta a memória alocada por rtree_get_keys().
 */
void rtree_free_keys(char **keys)
{
  free_keys_msg(keys);
}

int rtree_verify(struct rtree_t *rtree, int op_n)
{
  MessageT *msg = malloc(sizeof(MessageT));
  message_t__init(msg);
  msg->opcode = MESSAGE_T__OPCODE__OP_VERIFY;
  msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;

  struct message_t *msg_resposta;
  struct message_t *msg_pedido = malloc(sizeof(struct message_t));
  msg_pedido->msg_t = msg;
  msg->result = op_n;
  msg_resposta = network_send_receive(rtree, msg_pedido);

  int result = msg_resposta->msg_t->result;

  if (msg_resposta->msg_t->opcode == MESSAGE_T__OPCODE__OP_ERROR)
  {
    message_t__free_unpacked(msg_pedido->msg_t, NULL);
    free(msg);
    return -1;
  }
  else
  {
    message_t__free_unpacked(msg_pedido->msg_t, NULL);
    free(msg);
    free(msg_pedido);
    return result;
  }
}
