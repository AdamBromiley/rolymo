#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H


#include <stddef.h>

#include <sys/types.h>

#include "array.h"
#include "network_ctx.h"
#include "parameters.h"


ssize_t writeSocket(const void *src, int s, size_t n);
int blockingRead(NetworkCTX *network, int i, size_t n);
int getRowNumber(Block *block, NetworkCTX *network, const PlotCTX *p);
int nonblockingRead(NetworkCTX *network, int i);
ssize_t readSocket(void *dest, int s, size_t n);

int readParameters(NetworkCTX *network, PlotCTX **p);
int sendParameters(NetworkCTX *network, int i, const PlotCTX *p);

int sendRowData(const NetworkCTX *network, Block *block);


#endif