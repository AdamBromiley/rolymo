#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include <netinet/in.h>

#include "array.h"


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
int createClientReceiveBuffer(Client *client, size_t n);
void freeClientReceiveBuffer(Client *client);
void freeNetworkCTX(NetworkCTX *ctx);

int initialiseNetworkConnection(NetworkCTX *network, PlotCTX **p);

int initialiseAsMaster(NetworkCTX *network);
int initialiseAsWorker(NetworkCTX *network, PlotCTX **p);

int acceptConnection(NetworkCTX *network);

int listener(NetworkCTX *network, const Block *block);


#endif