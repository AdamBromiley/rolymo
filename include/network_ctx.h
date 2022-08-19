#ifndef NETWORK_CTX_H
#define NETWORK_CTX_H


#include <stdbool.h>
#include <stddef.h>

#include <netinet/in.h>
#include <poll.h>


#define GENERAL_NETWORK_BUFFER_SIZE 4096


typedef enum LANStatus
{
    LAN_NONE,
    LAN_MASTER,
    LAN_WORKER
} LANStatus;

typedef struct Connection
{
    struct sockaddr_in addr; /* Address */
    bool rowAllocated;       /* True if the worker has been allocated a row */
    size_t row;              /* Row number allocated to the worker */
    size_t n;                /* Receive buffer allocated size */
    size_t read;             /* Bytes of data present in the buffer */
    char *buffer;            /* Receive buffer */
} Connection;

typedef struct NetworkCTX
{
    LANStatus mode;          /* Whether master, worker, or standalone */
    int max;                 /* Maximum number of connections (inc. master) */
    int n;                   /* Number of current connections (inc. master) */
    Connection *connections; /* Array of workers (0 is self for LAN_MASTER) */
    struct pollfd *fds;      /* Socket file descriptor set for polling */
} NetworkCTX;


NetworkCTX * createNetworkCTX(LANStatus status, int n, struct sockaddr_in *addr);
void freeNetworkCTX(NetworkCTX *ctx);
Connection createConnection(void);
struct pollfd createPollfd(void);
int createClientReceiveBuffer(Connection *client, size_t n);
void clearClientReceiveBuffer(Connection *c);
void freeClientReceiveBuffer(Connection *client);


#endif