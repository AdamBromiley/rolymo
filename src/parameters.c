#include "parameters.h"

#include <float.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "colour.h"


/* Range of permissible complex numbers */
const struct ComplexNumber COMPLEX_MIN = {-(DBL_MAX), -(DBL_MAX)};
const struct ComplexNumber COMPLEX_MAX = {DBL_MAX, DBL_MAX};
const struct ComplexNumber C_MIN = {-2.0, -2.0};
const struct ComplexNumber C_MAX = {2.0, 2.0};

/* Escape radius of plot (mathematically defined as 2) */
const double ESCAPE_RADIUS = 2.0;

/* Range of permissible iteration counts */
const unsigned long int ITERATIONS_MIN = 0;
const unsigned long int ITERATIONS_MAX = ULONG_MAX;

/* Range of permissible dimensions */
const size_t WIDTH_MIN = 0;
const size_t WIDTH_MAX = SIZE_MAX;
const size_t HEIGHT_MIN = 0;
const size_t HEIGHT_MAX = SIZE_MAX;

/* Default parameters for Mandelbrot set plot */
const struct PlotCTX MANDELBROT_PARAMETERS_DEFAULT =
{
    .type = PLOT_MANDELBROT,
    .minimum = {-2.0, -1.25},
    .maximum = {0.75, 1.25},
    .iterations = ITERATIONS_DEFAULT,
    .output = OUTPUT_PPM,
    .file = NULL,
    .width = 550,
    .height = 500
};

/* Default parameters for Julia set plot */
const struct PlotCTX JULIA_PARAMETERS_DEFAULT =
{
    .type = PLOT_JULIA,
    .minimum = {-2.0, -2.0},
    .maximum = {2.0, 2.0},
    .iterations = ITERATIONS_DEFAULT,
    .output = OUTPUT_PPM,
    .file = NULL,
    .width = 800,
    .height = 800
};


/* Set default plot settings for Mandelbrot image output */
int initialiseParameters(struct PlotCTX *parameters, enum PlotType type)
{
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
    
    initialiseColourScheme(&(parameters->colour), COLOUR_SCHEME_TYPE_ASCII);

    return;
}


void getOutputString(char *dest, struct PlotCTX *parameters, size_t n)
{
    const char *type;

    switch (parameters->output)
    {
        case OUTPUT_PPM:
            switch (parameters->colour.depth)
            {
                case BIT_DEPTH_1:
                    type = "Portable Bit Map (.pbm)";
                    break;
                case BIT_DEPTH_8:
                    type = "Portable Gray Map (.pgm)";
                    break;
                case BIT_DEPTH_24:
                    type = "Portable Pixel Map (.ppm)";
                    break;
                default:
                    type = "Unknown Portable Any Map (.pnm) format";
                    break;
            }

            break;
        case OUTPUT_TERMINAL:
            type = "Terminal output";
            break;
        default:
            type = "Unknown output type";
            break;
    }

    strncpy(dest, type, n);
    dest[n - 1] = '\0';

    return;
}


void getPlotString(char *dest, struct PlotCTX *parameters, size_t n)
{
    const char *type;

    switch (parameters->type)
    {
        case PLOT_MANDELBROT:
            type = "Mandelbrot set";
            break;
        case PLOT_JULIA:
            type = "Julia set";
            break;
        default:
            type = "Unknown plot type";
            break;
    }

    strncpy(dest, type, n);
    dest[n - 1] = '\0';

    return;
}