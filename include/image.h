#ifndef IMAGE_H
#define IMAGE_H


#include "parameters.h"


int initialiseImage(struct PlotCTX *parameters, const char *filePath);
int imageOutput(const struct PlotCTX *parameters);
int closeImage(struct PlotCTX *parameters);


#endif