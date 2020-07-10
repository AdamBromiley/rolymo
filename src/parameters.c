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


/* Default terminal output dimensions */
const size_t JULIA_TERMINAL_WIDTH_DEFAULT = 80;
const size_t JULIA_TERMINAL_HEIGHT_DEFAULT = 46;
const size_t MANDELBROT_TERMINAL_WIDTH_DEFAULT = 80;
const size_t MANDELBROT_TERMINAL_HEIGHT_DEFAULT = 46;

/* Default colour schemes */
const ColourSchemeType COLOUR_SCHEME_DEFAULT = COLOUR_SCHEME_TYPE_RAINBOW;
const ColourSchemeType TERMINAL_COLOUR_SCHEME_DEFAULT = COLOUR_SCHEME_TYPE_ASCII;

/* Default parameters for Julia set plot */
const PlotCTX JULIA_PARAMETERS_DEFAULT =
{
    .type = PLOT_JULIA,
    .minimum.c = -2.0 - 2.0 * I,
    .maximum.c = 2.0 + 2.0 * I,
    .iterations = 100,
    .output = OUTPUT_PNM,
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
    .iterations = 100,
    .output = OUTPUT_PNM,
    .file = NULL,
    .width = 800,
    .height = 800
};

/* Default parameters for Julia set plot (arbitrary-precision) */
const PlotCTX JULIA_PARAMETERS_DEFAULT_MP =
{
    .type = PLOT_JULIA,
    .iterations = 100,
    .output = OUTPUT_PNM,
    .file = NULL,
    .width = 800,
    .height = 800
};

/* Default parameters for Mandelbrot set plot */
const PlotCTX MANDELBROT_PARAMETERS_DEFAULT =
{
    .type = PLOT_MANDELBROT,
    .minimum.c = -2.0 - 1.25 * I,
    .maximum.c = 0.75 + 1.25 * I,
    .iterations = 100,
    .output = OUTPUT_PNM,
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
    .iterations = 100,
    .output = OUTPUT_PNM,
    .file = NULL,
    .width = 550,
    .height = 500
};

/* Default parameters for Mandelbrot set plot (arbitrary-precision) */
const PlotCTX MANDELBROT_PARAMETERS_DEFAULT_MP =
{
    .type = PLOT_MANDELBROT,
    .iterations = 100,
    .output = OUTPUT_PNM,
    .file = NULL,
    .width = 550,
    .height = 500
};


static int initialiseMP(PlotCTX *p);
static void freeMP(PlotCTX *p);

static int initialiseImageOutputParameters(PlotCTX *p);
static int initialiseTerminalOutputParameters(PlotCTX *p);


/* Create plot parameters object and set default plot settings */
PlotCTX * createPlotCTX(PlotType plot, OutputType output)
{
    int ret;

    PlotCTX *p = malloc(sizeof(PlotCTX));

    if (!p)
        return NULL;

    p->type = plot;

    switch (output)
    {
        case OUTPUT_PNM:
            ret = initialiseImageOutputParameters(p);
            break;
        case OUTPUT_TERMINAL:
            ret = initialiseTerminalOutputParameters(p);
            break;
        default:
            free(p);
            return NULL;
    }

    if (ret)
    {
        free(p);
        return NULL;
    }

    return p;
}


/* Free the PlotCTX from memory */
void freePlotCTX(PlotCTX *p)
{
    if (precision == MUL_PRECISION)
        freeMP(p);

    if (p->file)
    {
        fclose(p->file);
        p->file = NULL;
    }

    free(p);

    return;
}


/* Get output type */
void getOutputString(char *dest, PlotCTX *p, size_t n)
{
    const char *type;

    switch (p->output)
    {
        case OUTPUT_PNM:
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


/* Convert plot type to string */
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


/* Initialise MP parameters to extended-precision defaults */
static int initialiseMP(PlotCTX *p)
{
    long double complex minimum;
    long double complex maximum;

    /* Initialise MP parameters */
    mpc_init2(p->minimum.mpc, mpSignificandSize);
    mpc_init2(p->maximum.mpc, mpSignificandSize);
    mpc_init2(p->c.mpc, mpSignificandSize);

    switch (p->type)
    {
        case PLOT_JULIA:
            minimum = JULIA_PARAMETERS_DEFAULT_EXT.minimum.lc;
            maximum = JULIA_PARAMETERS_DEFAULT_EXT.maximum.lc;
            break;
        case PLOT_MANDELBROT:
            minimum = MANDELBROT_PARAMETERS_DEFAULT_EXT.minimum.lc;
            maximum = MANDELBROT_PARAMETERS_DEFAULT_EXT.maximum.lc;
            break;
        default:
            return 1;
    }

    mpc_set_ldc(p->minimum.mpc, minimum, MP_COMPLEX_RND);
    mpc_set_ldc(p->maximum.mpc, maximum, MP_COMPLEX_RND);

    return 0;
}


/* Free MP parameters */
static void freeMP(PlotCTX *p)
{
    mpc_clear(p->minimum.mpc);
    mpc_clear(p->maximum.mpc);
    mpc_clear(p->c.mpc);

    return;
}


static int initialiseImageOutputParameters(PlotCTX *p)
{
    switch (p->type)
    {
        case PLOT_JULIA:
            switch (precision)
            {
                case STD_PRECISION:
                    *p = JULIA_PARAMETERS_DEFAULT;
                    break;
                case EXT_PRECISION:
                    *p = JULIA_PARAMETERS_DEFAULT_EXT;
                    break;
                case MUL_PRECISION:
                    *p = JULIA_PARAMETERS_DEFAULT_MP;
                    initialiseMP(p);
                    break;
                default:
                    return 1;
            }

            break;
        case PLOT_MANDELBROT:
            switch (precision)
            {
                case STD_PRECISION:
                    *p = MANDELBROT_PARAMETERS_DEFAULT;
                    break;
                case EXT_PRECISION:
                    *p = MANDELBROT_PARAMETERS_DEFAULT_EXT;
                    break;
                case MUL_PRECISION:
                    *p = MANDELBROT_PARAMETERS_DEFAULT_MP;
                    initialiseMP(p);
                    break;
                default:
                    return 1;
            }

            break;
        default:
            return 1;
    }

    p->output = OUTPUT_PNM;
    
    if (initialiseColourScheme(&(p->colour), COLOUR_SCHEME_DEFAULT))
    {
        freeMP(p);
        return 1;
    }

    return 0;
}


static int initialiseTerminalOutputParameters(PlotCTX *p)
{
    switch(p->type)
    {
        case PLOT_JULIA:
            switch (precision)
            {
                case STD_PRECISION:
                    *p = JULIA_PARAMETERS_DEFAULT;
                    break;
                case EXT_PRECISION:
                    *p = JULIA_PARAMETERS_DEFAULT_EXT;
                    break;
                case MUL_PRECISION:
                    *p = JULIA_PARAMETERS_DEFAULT_MP;
                    initialiseMP(p);
                    break;
                default:
                    return 1;
            }

            p->width = JULIA_TERMINAL_WIDTH_DEFAULT;
            p->height = JULIA_TERMINAL_HEIGHT_DEFAULT;
            
            break;
        case PLOT_MANDELBROT:
            switch (precision)
            {
                case STD_PRECISION:
                    *p = MANDELBROT_PARAMETERS_DEFAULT;
                    break;
                case EXT_PRECISION:
                    *p = MANDELBROT_PARAMETERS_DEFAULT_EXT;
                    break;
                case MUL_PRECISION:
                    *p = MANDELBROT_PARAMETERS_DEFAULT_MP;
                    initialiseMP(p);
                    break;
                default:
                    return 1;
            }

            p->width = MANDELBROT_TERMINAL_WIDTH_DEFAULT;
            p->height = MANDELBROT_TERMINAL_HEIGHT_DEFAULT;

            break;
        default:
            return 1;
    }

    p->output = OUTPUT_TERMINAL;
    p->file = stdout;
    
    if (initialiseColourScheme(&(p->colour), TERMINAL_COLOUR_SCHEME_DEFAULT))
    {
        freeMP(p);
        return 1;
    }

    return 0;
}