#include "parameters.h"

#include <complex.h>
#include <float.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <mpc.h>

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

/* Range of permissible constant values (extended-precision) */
const long double complex C_MIN_EXT = -2.0L - 2.0L * I;
const long double complex C_MAX_EXT = 2.0L + 2.0L * I;

/* Range of permissible magnification values */
const double MAGNIFICATION_MIN = -(DBL_MAX);
const double MAGNIFICATION_MAX = DBL_MAX;

/* Range of permissible magnification values (extended-precision) */
const double MAGNIFICATION_MIN_EXT = -(LDBL_MAX);
const double MAGNIFICATION_MAX_EXT = LDBL_MAX;

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

/* Default parameters for Mandelbrot set plot (arbitrary-precision) */
const PlotCTX MANDELBROT_PARAMETERS_DEFAULT_ARB =
{
    .type = PLOT_MANDELBROT,
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

/* Default parameters for Julia set plot (arbitrary-precision) */
const PlotCTX JULIA_PARAMETERS_DEFAULT_ARB =
{
    .type = PLOT_JULIA,
    .iterations = ITERATIONS_DEFAULT,
    .output = OUTPUT_PPM,
    .file = NULL,
    .width = 800,
    .height = 800
};


/* Create plot parameters object and set default plot settings */
PlotCTX * createPlotCTX(PlotType type)
{
    PlotCTX *p = malloc(sizeof(PlotCTX));

    if (!p)
        return NULL;

    switch (type)
    {
        case PLOT_MANDELBROT:
            switch (precision)
            {
                case STD_PRECISION:
                    *p = MANDELBROT_PARAMETERS_DEFAULT;
                    break;
                case EXT_PRECISION:
                    *p = MANDELBROT_PARAMETERS_DEFAULT_EXT;
                    break;
                case ARB_PRECISION:
                    *p = MANDELBROT_PARAMETERS_DEFAULT_ARB;

                    mpc_init2(p->minimum.mpc, ARB_PRECISION_BITS);
                    mpc_set_d_d(p->minimum.mpc, creal(MANDELBROT_PARAMETERS_DEFAULT.minimum.c),
                        cimag(MANDELBROT_PARAMETERS_DEFAULT.minimum.c), ARB_CMPLX_ROUNDING);

                    mpc_init2(p->maximum.mpc, ARB_PRECISION_BITS);
                    mpc_set_d_d(p->maximum.mpc, creal(MANDELBROT_PARAMETERS_DEFAULT.maximum.c),
                        cimag(MANDELBROT_PARAMETERS_DEFAULT.maximum.c), ARB_CMPLX_ROUNDING);

                    mpc_init2(p->c.mpc, ARB_PRECISION_BITS);

                    break;
                default:
                    free(p);
                    return NULL;
            }

            break;
        case PLOT_JULIA:
            switch (precision)
            {
                case STD_PRECISION:
                    *p = JULIA_PARAMETERS_DEFAULT;
                    break;
                case EXT_PRECISION:
                    *p = JULIA_PARAMETERS_DEFAULT_EXT;
                    break;
                case ARB_PRECISION:
                    *p = JULIA_PARAMETERS_DEFAULT_ARB;

                    mpc_init2(p->minimum.mpc, ARB_PRECISION_BITS);
                    mpc_set_d_d(p->minimum.mpc, creal(JULIA_PARAMETERS_DEFAULT.minimum.c),
                        cimag(JULIA_PARAMETERS_DEFAULT.minimum.c), ARB_CMPLX_ROUNDING);

                    mpc_init2(p->maximum.mpc, ARB_PRECISION_BITS);
                    mpc_set_d_d(p->maximum.mpc, creal(JULIA_PARAMETERS_DEFAULT.maximum.c),
                        cimag(JULIA_PARAMETERS_DEFAULT.maximum.c), ARB_CMPLX_ROUNDING);

                    mpc_init2(p->c.mpc, ARB_PRECISION_BITS);
                    mpc_set_d_d(p->c.mpc, creal(JULIA_PARAMETERS_DEFAULT.c.c),
                        cimag(JULIA_PARAMETERS_DEFAULT.c.c), ARB_CMPLX_ROUNDING);

                    break;
                default:
                    free(p);
                    return NULL;
            }

            break;
        default:
            free(p);
            return NULL;
    }

    initialiseColourScheme(&(p->colour), COLOUR_SCHEME_TYPE_DEFAULT);

    return p;
}


void freePlotCTX(PlotCTX *p)
{
    if (precision == ARB_PRECISION)
    {
        mpc_clear(p->minimum.mpc);
        mpc_clear(p->maximum.mpc);
        mpc_clear(p->c.mpc);
    }

    if (p->file)
        fclose(p->file);

    p->file = NULL;

    free(p);

    return;
}


void initialiseTerminalOutputParameters(PlotCTX *p)
{
    /* Sensible terminal output dimension values */
    const size_t TERMINAL_WIDTH = 80;
    const size_t TERMINAL_HEIGHT = 46;

    p->file = stdout;
    p->width = TERMINAL_WIDTH;
    p->height = TERMINAL_HEIGHT;
    
    initialiseColourScheme(&(p->colour), COLOUR_SCHEME_TYPE_ASCII);

    return;
}


void getOutputString(char *dest, PlotCTX *p, size_t n)
{
    const char *type;

    switch (p->output)
    {
        case OUTPUT_PPM:
            switch (p->colour.depth)
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