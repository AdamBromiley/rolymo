#ifndef NETWORK_CTX_H
#define NETWORK_CTX_H


#include <stdbool.h>
#include <stddef.h>

#include <netinet/in.h>


typedef enum LANStatus
{
    LAN_NONE,
    LAN_MASTER,
    LAN_WORKER
} LANStatus;

typedef struct Client
{
    int s;                   /* Local socket connected to client */
    struct sockaddr_in addr; /* Address structure for client */
    bool rowAllocated;       /* True if worker has been allocated row */
    size_t row;              /* Row number allocated to the worker */
    size_t n;                /* Receive buffer allocated size */
    size_t read;             /* Bytes of data present in the buffer */
    char *buffer;            /* Receive buffer */
} Client;

typedef struct NetworkCTX
{
    LANStatus mode;          /* Whether master, worker, or standalone */
    struct sockaddr_in addr; /* IP address connected to/listening on */
    int s;                   /* Connected socket */
    int n;                   /* Number of workers */
    Client *workers;         /* Array of sockets connected to workers */
} NetworkCTX;


NetworkCTX * createNetworkCTX(int n);
void freeNetworkCTX(NetworkCTX *ctx);
int createClientReceiveBuffer(Client *client, size_t n);
void freeClientReceiveBuffer(Client *client);


#endif