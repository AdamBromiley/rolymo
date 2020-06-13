#ifndef PARAMETERS_H
#define PARAMETERS_H


#include <stddef.h>
#include <stdio.h>

#include "colour.h"


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


struct ComplexNumber
{
    double re, im;
};


struct PlotCTX
{
    enum PlotType type;
    struct ComplexNumber minimum, maximum;
    unsigned int iterations;
    enum OutputType output;
    FILE *file;
    size_t width, height;
    struct ColourScheme colour;
};


extern const struct ComplexNumber COMPLEX_MIN;
extern const struct ComplexNumber COMPLEX_MAX;

extern const double ESCAPE_RADIUS;

extern const int ITERATIONS_MIN;
extern const int ITERATIONS_MAX;

extern const size_t WIDTH_MIN;
extern const size_t WIDTH_MAX;
extern const size_t HEIGHT_MIN;
extern const size_t HEIGHT_MAX;


int initialiseParameters(struct PlotCTX *parameters, enum PlotType type);
void initialiseTerminalOutputParameters(struct PlotCTX *parameters);


#endif