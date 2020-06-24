#ifndef IMAGE_H
#define IMAGE_H


#include "parameters.h"


int initialiseImage(struct PlotCTX *parameters, const char *filepath);
int imageOutput(struct PlotCTX *parameters);
int closeImage(struct PlotCTX *parameters);


#endif