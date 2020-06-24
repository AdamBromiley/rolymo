#ifndef PARAMETERS_H
#define PARAMETERS_H


#include <stddef.h>
#include <stdio.h>

#include "colour.h"


#define ITERATIONS_DEFAULT 100

#define OUTPUT_STRING_LENGTH_MAX 64
#define PLOT_STRING_LENGTH_MAX 32


enum PlotType
{
    PLOT_NONE,
    PLOT_MANDELBROT,
    PLOT_JULIA
};

enum OutputType
{
    OUTPUT_PPM,
    OUTPUT_TERMINAL
};

struct PlotCTX
{
    enum PlotType type;
    complex minimum, maximum, c;
    unsigned long int iterations;
    enum OutputType output;
    FILE *file;
    size_t width, height;
    struct ColourScheme colour;
};


extern const complex COMPLEX_MIN;
extern const complex COMPLEX_MAX;
extern const complex C_MIN;
extern const complex C_MAX;

extern const unsigned long int ITERATIONS_MIN;
extern const unsigned long int ITERATIONS_MAX;

extern const size_t WIDTH_MIN;
extern const size_t WIDTH_MAX;
extern const size_t HEIGHT_MIN;
extern const size_t HEIGHT_MAX;

extern const struct PlotCTX MANDELBROT_PARAMETERS_DEFAULT;
extern const struct PlotCTX JULIA_PARAMETERS_DEFAULT;


int initialiseParameters(struct PlotCTX *parameters, enum PlotType type);
void initialiseTerminalOutputParameters(struct PlotCTX *parameters);

void getOutputString(char *dest, struct PlotCTX *parameters, size_t n);
void getPlotString(char *dest, enum PlotType plot, size_t n);


#endif