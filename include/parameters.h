#ifndef PARAMETERS_H
#define PARAMETERS_H


#include <complex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "colour.h"
#include "ext_precision.h"


#define PLOT_FILEPATH_LEN_MAX 4096
#define PLOT_FILEPATH_DEFAULT "var/mandelbrot.pnm"


typedef enum PlotType
{
    PLOT_NONE,
    PLOT_JULIA,
    PLOT_MANDELBROT
} PlotType;

typedef enum OutputType
{
    OUTPUT_NONE,
    OUTPUT_PNM,
    OUTPUT_TERMINAL
} OutputType;

typedef struct PlotCTX
{
    PrecisionMode precision;
    PlotType type;
    ExtComplex minimum, maximum, c;
    unsigned long iterations;
    OutputType output;
    char plotFilepath[PLOT_FILEPATH_LEN_MAX];
    FILE *file;
    size_t width, height;
    ColourScheme colour;
} PlotCTX;


extern const size_t JULIA_TERMINAL_WIDTH_DEFAULT;
extern const size_t JULIA_TERMINAL_HEIGHT_DEFAULT;
extern const size_t MANDELBROT_TERMINAL_WIDTH_DEFAULT;
extern const size_t MANDELBROT_TERMINAL_HEIGHT_DEFAULT;

extern const ColourSchemeType COLOUR_SCHEME_DEFAULT;

extern const PlotCTX JULIA_PARAMETERS_DEFAULT;
extern const PlotCTX JULIA_PARAMETERS_DEFAULT_EXT;

#ifdef MP_PREC
extern const PlotCTX JULIA_PARAMETERS_DEFAULT_MP;
#endif

extern const PlotCTX MANDELBROT_PARAMETERS_DEFAULT;
extern const PlotCTX MANDELBROT_PARAMETERS_DEFAULT_EXT;

#ifdef MP_PREC
extern const PlotCTX MANDELBROT_PARAMETERS_DEFAULT_MP;
#endif


PlotCTX * createPlotCTX(PrecisionMode precision);
int initialisePlotCTX(PlotCTX *p, PlotType plot, OutputType output);
void freePlotCTX(PlotCTX *p);

int getOutputString(char *dest, const PlotCTX *p, size_t n);
int getPlotString(char *dest, PlotType plot, size_t n);


#endif