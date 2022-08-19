#ifndef NETWORK_CTX_H
#define NETWORK_CTX_H


#include <netinet/in.h>
#include <poll.h>

#include "connection.h"


#define GENERAL_NETWORK_BUFFER_SIZE 4096


typedef enum LANStatus
{
    LAN_NONE,
    LAN_MASTER,
    LAN_WORKER
} LANStatus;

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
struct pollfd createPollfd(void);


#endif