#ifndef PARAMETERS_H
#define PARAMETERS_H


#include <complex.h>
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
    long double complex minimum, maximum, c;
    unsigned long iterations;
    enum OutputType output;
    FILE *file;
    size_t width, height;
    struct ColourScheme colour;
};


extern const long double complex LCOMPLEX_MIN;
extern const long double complex LCOMPLEX_MAX;
extern const long double complex LC_MIN;
extern const long double complex LC_MAX;

extern const long double MAGNIFICATION_MIN;
extern const long double MAGNIFICATION_MAX;

extern const unsigned long ITERATIONS_MIN;
extern const unsigned long ITERATIONS_MAX;

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