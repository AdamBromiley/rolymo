#ifndef IMAGE_H
#define IMAGE_H


#include <stddef.h>

#include "parameters.h"


extern const size_t MEMORY_MIN;
extern const size_t MEMORY_MAX;

extern const unsigned int THREAD_COUNT_MIN;
extern const unsigned int THREAD_COUNT_MAX;


int initialiseImage(PlotCTX *p, const char *filepath);
int imageOutput(PlotCTX *p, size_t memory, unsigned int threadCount);
int closeImage(PlotCTX *p);


#endif