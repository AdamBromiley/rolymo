#include "parameters.h"

#include <float.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h.>


const struct ComplexNumber COMPLEX_MIN = {-(DBL_MAX), -(DBL_MAX)};
const struct ComplexNumber COMPLEX_MAX = {DBL_MAX, DBL_MAX};

const double ESCAPE_RADIUS = 2.0;

const unsigned int ITERATIONS_MIN = 0;
const unsigned int ITERATIONS_MAX = UINT_MAX;

const size_t WIDTH_MIN = 0;
const size_t WIDTH_MAX = SIZE_MAX;
const size_t HEIGHT_MIN = 0;
const size_t HEIGHT_MAX = SIZE_MAX;


/* Set default plot settings for Mandelbrot image output */
int initialiseParameters(struct PlotCTX *parameters, enum PlotType type)
{
    const struct PlotCTX MANDELBROT_PARAMETERS_DEFAULT =
    {
        .type = PLOT_MANDELBROT,
        .minimum = {-2.0, -1.25},
        .maximum = {0.75, 1.25},
        .iterations = 100,
        .output = OUTPUT_PPM,
        .file = NULL,
        .width = 550,
        .height = 500
    };

    const struct PlotCTX JULIA_PARAMETERS_DEFAULT =
    {
        .type = PLOT_JULIA,
        .minimum = {-2.0, 2.0},
        .maximum = {2.0, 2.0},
        .iterations = 100,
        .output = OUTPUT_PPM,
        .file = NULL,
        .width = 800,
        .height = 800
    };

    switch (type)
    {
        case PLOT_MANDELBROT:
            *parameters = MANDELBROT_PARAMETERS_DEFAULT;
            break;
        case PLOT_JULIA:
            *parameters = JULIA_PARAMETERS_DEFAULT;
            break;
        default:
            return 1;
    }

    initialiseColourScheme(&(parameters->colour), COLOUR_SCHEME_TYPE_DEFAULT);

    return 0;
}


void initialiseTerminalOutputParameters(struct PlotCTX *parameters)
{
    const size_t TERMINAL_WIDTH = 80;
    const size_t TERMINAL_HEIGHT = 46;

    parameters->file = stdout;
    parameters->width = TERMINAL_WIDTH;
    parameters->height = TERMINAL_HEIGHT;

    return;
}