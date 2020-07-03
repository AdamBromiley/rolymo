#include "parameters.h"

#include <complex.h>
#include <float.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "colour.h"
#include "ext_precision.h"


/* Range of permissible complex numbers */
const complex COMPLEX_MIN = -(DBL_MAX) - (DBL_MAX) * I;
const complex COMPLEX_MAX = (DBL_MAX) + (DBL_MAX) * I;

/* Range of permissible complex numbers (extended-precision) */
const long double complex COMPLEX_MIN_EXT = -(LDBL_MAX) - (LDBL_MAX) * I;
const long double complex COMPLEX_MAX_EXT = (LDBL_MAX) + (LDBL_MAX) * I;

/* Range of permissible constant values */
const complex C_MIN = -2.0 - 2.0 * I;
const complex C_MAX = 2.0 + 2.0 * I;
const long double complex C_MIN_EXT = -2.0L - 2.0L * I;
const long double complex C_MAX_EXT = 2.0L + 2.0L * I;

/* Range of permissible magnification values */
const double MAGNIFICATION_MIN = -(DBL_MAX);
const double MAGNIFICATION_MAX = DBL_MAX;

/* Range of permissible iteration counts */
const unsigned long ITERATIONS_MIN = 0UL;
const unsigned long ITERATIONS_MAX = ULONG_MAX;

/* Range of permissible dimensions */
const size_t WIDTH_MIN = 0;
const size_t WIDTH_MAX = SIZE_MAX;
const size_t HEIGHT_MIN = 0;
const size_t HEIGHT_MAX = SIZE_MAX;

/* Default parameters for Mandelbrot set plot */
const PlotCTX MANDELBROT_PARAMETERS_DEFAULT =
{
    .type = PLOT_MANDELBROT,
    .minimum.c = -2.0 - 1.25 * I,
    .maximum.c = 0.75 + 1.25 * I,
    .iterations = ITERATIONS_DEFAULT,
    .output = OUTPUT_PPM,
    .file = NULL,
    .width = 550,
    .height = 500
};

/* Default parameters for Mandelbrot set plot (extended-precision) */
const PlotCTX MANDELBROT_PARAMETERS_DEFAULT_EXT =
{
    .type = PLOT_MANDELBROT,
    .minimum.lc = -2.0L - 1.25L * I,
    .maximum.lc = 0.75L + 1.25L * I,
    .iterations = ITERATIONS_DEFAULT,
    .output = OUTPUT_PPM,
    .file = NULL,
    .width = 550,
    .height = 500
};

/* Default parameters for Julia set plot */
const PlotCTX JULIA_PARAMETERS_DEFAULT =
{
    .type = PLOT_JULIA,
    .minimum.c = -2.0 - 2.0 * I,
    .maximum.c = 2.0 + 2.0 * I,
    .iterations = ITERATIONS_DEFAULT,
    .output = OUTPUT_PPM,
    .file = NULL,
    .width = 800,
    .height = 800
};

/* Default parameters for Julia set plot (extended-precision) */
const PlotCTX JULIA_PARAMETERS_DEFAULT_EXT =
{
    .type = PLOT_JULIA,
    .minimum.lc = -2.0L - 2.0L * I,
    .maximum.lc = 2.0L + 2.0L * I,
    .iterations = ITERATIONS_DEFAULT,
    .output = OUTPUT_PPM,
    .file = NULL,
    .width = 800,
    .height = 800
};


/* Set default plot settings for Mandelbrot image output */
int initialiseParameters(PlotCTX *parameters, PlotType type)
{
    switch (type)
    {
        case PLOT_MANDELBROT:
            *parameters = (!extPrecision) ? MANDELBROT_PARAMETERS_DEFAULT
                                          : MANDELBROT_PARAMETERS_DEFAULT_EXT;
            break;
        case PLOT_JULIA:
            *parameters = (!extPrecision) ? JULIA_PARAMETERS_DEFAULT
                                          : JULIA_PARAMETERS_DEFAULT_EXT;
            break;
        default:
            return 1;
    }

    initialiseColourScheme(&(parameters->colour), COLOUR_SCHEME_TYPE_DEFAULT);

    return 0;
}


void initialiseTerminalOutputParameters(PlotCTX *parameters)
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


void getOutputString(char *dest, PlotCTX *parameters, size_t n)
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


void getPlotString(char *dest, PlotType plot, size_t n)
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