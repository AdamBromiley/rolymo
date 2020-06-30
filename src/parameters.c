#include "parameters.h"

#include <complex.h>
#include <float.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "colour.h"


/* Range of permissible complex numbers */
const complex COMPLEX_MIN = -(DBL_MAX) - DBL_MAX * I;
const complex COMPLEX_MAX = DBL_MAX + DBL_MAX * I;
const complex C_MIN = -2.0 - 2.0 * I;
const complex C_MAX = 2.0 + 2.0 * I;

/* Range of permissible long double complex numbers */
const long double complex LCOMPLEX_MIN = -(LDBL_MAX) - LDBL_MAX * I;
const long double complex LCOMPLEX_MAX = LDBL_MAX + LDBL_MAX * I;
const long double complex LC_MIN = -2.0L - 2.0L * I;
const long double complex LC_MAX = 2.0L + 2.0L * I;

/* Range of permissible magnification values */
const long double MAGNIFICATION_MIN = -(DBL_MAX);
const long double MAGNIFICATION_MAX = DBL_MAX;

/* Range of permissible iteration counts */
const unsigned long ITERATIONS_MIN = 0;
const unsigned long ITERATIONS_MAX = ULONG_MAX;

/* Range of permissible dimensions */
const size_t WIDTH_MIN = 0;
const size_t WIDTH_MAX = SIZE_MAX;
const size_t HEIGHT_MIN = 0;
const size_t HEIGHT_MAX = SIZE_MAX;

/* Default parameters for Mandelbrot set plot */
const struct PlotCTX MANDELBROT_PARAMETERS_DEFAULT =
{
    .type = PLOT_MANDELBROT,
    .minimum = -2.0 - 1.25 * I,
    .maximum = 0.75 + 1.25 * I,
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
    .minimum = -2.0 - 2.0 * I,
    .maximum = 2.0 + 2.0 * I,
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
    /* Sensible terminal output dimension values */
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


void getPlotString(char *dest, enum PlotType plot, size_t n)
{
    const char *type;

    switch (plot)
    {
        case PLOT_JULIA:
            type = "Julia set";
            break;
        case PLOT_MANDELBROT:
            type = "Mandelbrot set";
            break;
        default:
            type = "Unknown plot type";
            break;
    }

    strncpy(dest, type, n);
    dest[n - 1] = '\0';

    return;
}