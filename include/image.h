#ifndef IMAGE_H
#define IMAGE_H


#include <stddef.h>

#include "network_ctx.h"
#include "parameters.h"
#include "program_ctx.h"


extern const size_t MEMORY_MIN;
extern const size_t MEMORY_MAX;

extern const unsigned int THREAD_COUNT_MIN;
extern const unsigned int THREAD_COUNT_MAX;


int initialiseImage(PlotCTX *p);
int imageOutput(PlotCTX *p, ProgramCTX *ctx);
int imageOutputMaster(PlotCTX *p, NetworkCTX *network, ProgramCTX *ctx);
int imageRowOutput(PlotCTX *p, NetworkCTX *network, ProgramCTX *ctx);
int closeImage(PlotCTX *p);


#endif