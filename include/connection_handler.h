#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H


#include <stddef.h>

#include "array.h"
#include "network_ctx.h"


int initialiseNetworkConnection(NetworkCTX *network, PlotCTX **p);

int initialiseAsMaster(NetworkCTX *network);
int initialiseAsWorker(NetworkCTX *network, PlotCTX **p);

int acceptConnection(NetworkCTX *network);

int listener(NetworkCTX *network, const Block *block);


#endif