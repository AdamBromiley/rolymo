#ifndef IMAGE_H
#define IMAGE_H


#include <stddef.h>

#include "connection_handler.h"
#include "parameters.h"


extern const size_t MEMORY_MIN;
extern const size_t MEMORY_MAX;

extern const unsigned int THREAD_COUNT_MIN;
extern const unsigned int THREAD_COUNT_MAX;


int initialiseImage(PlotCTX *p, const char *filepath);
int imageOutput(PlotCTX *p, size_t memory, unsigned int threadCount);
int imageOutputMaster(PlotCTX *p, NetworkCTX *network, size_t mem);
int imageRowOutput(PlotCTX *p, NetworkCTX *network, unsigned int threadCount);
int closeImage(PlotCTX *p);


#endif