#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H


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


typedef struct LANCTX
{
    LANStatus mode;
    struct sockaddr_in addr;
    int s;
    int n;
    int slaves[1];
} LANCTX;


int initialiseNetworkConnection(LANCTX *lan, PlotCTX *p);

int initialiseMaster(LANCTX *lan);
int initialiseSlave(PlotCTX *p, LANCTX *lan);
int initialiseSlaves(int *slaves, int n, PlotCTX *p);

int acceptConnectionReq(int master);
int acceptConnections(int *slaves, int s, int n, time_t timeout);

int listener(int *slaves, int n, Block *block);


#endif