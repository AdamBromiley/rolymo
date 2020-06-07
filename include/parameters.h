#ifndef PARAMETERS_H
#define PARAMETERS_H


#include <limits.h>
#include <stdint.h>
#include <stddef.h>
#include <complex.h>
#include <float.h>

#include <colour.h>


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
    /* Minimum/maximum values */
    struct ComplexNumber minimum, maximum;

    /* Maximum iterations before being considered to have escaped */
    unsigned int iterations;

    /* Output type: terminal or PPM file */
    enum OutputType output;

    /* Dimensions of output in pixels or columns/lines */
    size_t width, height;

    /* Colour scheme of the PPM image */
    enum ColourScheme colour;
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

extern const size_t TERMINAL_WIDTH;
extern const size_t TERMINAL_HEIGHT;


int initialiseParameters(struct PlotCTX *parameters, enum PlotType type);


#endif