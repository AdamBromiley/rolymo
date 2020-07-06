#ifndef PARAMETERS_H
#define PARAMETERS_H


#include <complex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "colour.h"
#include "ext_precision.h"


#define ITERATIONS_DEFAULT 100

#define OUTPUT_STR_LEN_MAX 64
#define PLOT_STR_LEN_MAX 32


typedef enum PlotType
{
    PLOT_NONE,
    PLOT_MANDELBROT,
    PLOT_JULIA
} PlotType;

typedef enum OutputType
{
    OUTPUT_PPM,
    OUTPUT_TERMINAL
} OutputType;

typedef struct PlotCTX
{
    PlotType type;
    ExtComplex minimum, maximum, c;
    unsigned long iterations;
    enum OutputType output;
    FILE *file;
    size_t width, height;
    struct ColourScheme colour;
} PlotCTX;


extern const complex COMPLEX_MIN;
extern const complex COMPLEX_MAX;
extern const long double complex COMPLEX_MIN_EXT;
extern const long double complex COMPLEX_MAX_EXT;

extern const complex C_MIN;
extern const complex C_MAX;
extern const long double complex C_MIN_EXT;
extern const long double complex C_MAX_EXT;

extern const double MAGNIFICATION_MIN;
extern const double MAGNIFICATION_MAX;
extern const double MAGNIFICATION_MIN_EXT;
extern const double MAGNIFICATION_MAX_EXT;

extern const unsigned long ITERATIONS_MIN;
extern const unsigned long ITERATIONS_MAX;

extern const size_t WIDTH_MIN;
extern const size_t WIDTH_MAX;
extern const size_t HEIGHT_MIN;
extern const size_t HEIGHT_MAX;

extern const PlotCTX MANDELBROT_PARAMETERS_DEFAULT;
extern const PlotCTX MANDELBROT_PARAMETERS_DEFAULT_EXT;
extern const PlotCTX MANDELBROT_PARAMETERS_DEFAULT_ARB;

extern const PlotCTX JULIA_PARAMETERS_DEFAULT;
extern const PlotCTX JULIA_PARAMETERS_DEFAULT_EXT;
extern const PlotCTX JULIA_PARAMETERS_DEFAULT_ARB;


PlotCTX * createPlotCTX(PlotType type);
void freePlotCTX(PlotCTX *parameters);
void initialiseTerminalOutputParameters(PlotCTX *parameters);

void getOutputString(char *dest, PlotCTX *parameters, size_t n);
void getPlotString(char *dest, PlotType plot, size_t n);


#endif