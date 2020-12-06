#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H


#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include <netinet/in.h>

#include "array.h"


typedef enum LANStatus
{
    LAN_NONE,
    LAN_MASTER,
    LAN_SLAVE
} LANStatus;


typedef struct NetworkCTX
{
    LANStatus mode;
    struct sockaddr_in addr;
    int s;
    int n;
    int *slaves;
} NetworkCTX;


NetworkCTX * createNetworkCTX(int n);
void freeNetworkCTX(NetworkCTX *ctx);

int initialiseNetworkConnection(NetworkCTX *network, PlotCTX **p);

int initialiseMaster(NetworkCTX *network);
int initialiseSlave(NetworkCTX *network, PlotCTX **p);
int initialiseSlaves(NetworkCTX *network, PlotCTX *p);

int acceptConnectionReq(NetworkCTX *network);
int acceptConnections(NetworkCTX *network, time_t timeout);

int listener(NetworkCTX *network, const Block *block);


#endif