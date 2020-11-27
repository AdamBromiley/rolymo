#ifndef PROCESS_OPTIONS_H
#define PROCESS_OPTIONS_H


#include "ext_precision.h"
#include "parameters.h"
#include "program_ctx.h"
#include "connection_handler.h"


extern const uint16_t PORT_DEFAULT;


int validateOptions(int argc, char **argv);

int processProgramOptions(ProgramCTX *ctx, NetworkCTX **network, int argc, char **argv);
PlotCTX * processPlotOptions(int argc, char **argv);


#endif