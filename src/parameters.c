#include <complex.h>
#include <float.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parameters.h"

#include "colour.h"
#include "ext_precision.h"

#ifdef MP_PREC
#include <mpc.h>
#endif


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
    .precision = STD_PRECISION,
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
    .precision = EXT_PRECISION,
    .type = PLOT_JULIA,
    .minimum.lc = -2.0L - 2.0L * I,
    .maximum.lc = 2.0L + 2.0L * I,
    .iterations = 100,
    .output = OUTPUT_PNM,
    .file = NULL,
    .width = 800,
    .height = 800
};

#ifdef MP_PREC
/* Default parameters for Julia set plot (multiple-precision) */
const PlotCTX JULIA_PARAMETERS_DEFAULT_MP =
{
    .precision = MUL_PRECISION,
    .type = PLOT_JULIA,
    .iterations = 100,
    .output = OUTPUT_PNM,
    .file = NULL,
    .width = 800,
    .height = 800
};
#endif

/* Default parameters for Mandelbrot set plot */
const PlotCTX MANDELBROT_PARAMETERS_DEFAULT =
{
    .precision = STD_PRECISION,
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
    .precision = EXT_PRECISION,
    .type = PLOT_MANDELBROT,
    .minimum.lc = -2.0L - 1.25L * I,
    .maximum.lc = 0.75L + 1.25L * I,
    .iterations = 100,
    .output = OUTPUT_PNM,
    .file = NULL,
    .width = 550,
    .height = 500
};

#ifdef MP_PREC
/* Default parameters for Mandelbrot set plot (multiple-precision) */
const PlotCTX MANDELBROT_PARAMETERS_DEFAULT_MP =
{
    .precision = MUL_PRECISION,
    .type = PLOT_MANDELBROT,
    .iterations = 100,
    .output = OUTPUT_PNM,
    .file = NULL,
    .width = 550,
    .height = 500
};


static void createMP(PlotCTX *p);
static int initialiseMP(PlotCTX *p);
static void freeMP(PlotCTX *p);
#endif

static int initialiseImageOutputParameters(PlotCTX *p);
static int initialiseTerminalOutputParameters(PlotCTX *p);


/* Create plot parameters object */
PlotCTX * createPlotCTX(PrecisionMode precision)
{
    PlotCTX *p = malloc(sizeof(PlotCTX));

    if (!p)
        return NULL;

    p->precision = precision;

    #ifdef MP_PREC
    if (p->precision == MUL_PRECISION)
        createMP(p);
    #endif

    return p;
}


/* Set default plot settings into PlotCTX object */
int initialisePlotCTX(PlotCTX *p, PlotType plot, OutputType output)
{
    int ret;

    if (!p)
        return 1;

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
            return 1;
    }

    if (ret)
        return 1;

    return 0;
}


/* Free the PlotCTX from memory */
void freePlotCTX(PlotCTX *p)
{
    if (p)
    {
        #ifdef MP_PREC
        if (p->precision == MUL_PRECISION)
            freeMP(p);
        #endif

        if (p->file)
        {
            fclose(p->file);
            p->file = NULL;
        }
    }

    free(p);
}


#ifdef MP_PREC
/* Allocate memory for MP parameters */
static void createMP(PlotCTX *p)
{
    if (p && p->precision == MUL_PRECISION)
    {
        mpc_init2(p->minimum.mpc, mpSignificandSize);
        mpc_init2(p->maximum.mpc, mpSignificandSize);
        mpc_init2(p->c.mpc, mpSignificandSize);
    }
}


/* Free MP parameters */
static void freeMP(PlotCTX *p)
{
    if (p && p->precision == MUL_PRECISION)
    {
        mpc_clear(p->minimum.mpc);
        mpc_clear(p->maximum.mpc);
        mpc_clear(p->c.mpc);
    }
}
#endif


/* Get output type */
int getOutputString(char *dest, const PlotCTX *p, size_t n)
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
                    type = "Portable Any Map (.pnm)";
                    break;
            }

            break;
        case OUTPUT_TERMINAL:
            type = "Terminal output";
            break;
        default:
            return 1;
    }

    strncpy(dest, type, n);
    dest[n - 1] = '\0';

    return 0;
}


/* Convert plot type to string */
int getPlotString(char *dest, PlotType plot, size_t n)
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
            return 1;
    }

    strncpy(dest, type, n);
    dest[n - 1] = '\0';

    return 0;
}


static int initialiseImageOutputParameters(PlotCTX *p)
{
    switch (p->type)
    {
        case PLOT_JULIA:
            switch (p->precision)
            {
                case STD_PRECISION:
                    *p = JULIA_PARAMETERS_DEFAULT;
                    break;
                case EXT_PRECISION:
                    *p = JULIA_PARAMETERS_DEFAULT_EXT;
                    break;
                
                #ifdef MP_PREC
                case MUL_PRECISION:
                    p->iterations = JULIA_PARAMETERS_DEFAULT_MP.iterations;
                    p->width = JULIA_PARAMETERS_DEFAULT_MP.width;
                    p->height = JULIA_PARAMETERS_DEFAULT_MP.height;
                    initialiseMP(p);
                    break;
                #endif

                default:
                    return 1;
            }

            break;
        case PLOT_MANDELBROT:
            switch (p->precision)
            {
                case STD_PRECISION:
                    *p = MANDELBROT_PARAMETERS_DEFAULT;
                    break;
                case EXT_PRECISION:
                    *p = MANDELBROT_PARAMETERS_DEFAULT_EXT;
                    break;
                
                #ifdef MP_PREC
                case MUL_PRECISION:
                    p->iterations = MANDELBROT_PARAMETERS_DEFAULT_MP.iterations;
                    p->width = MANDELBROT_PARAMETERS_DEFAULT_MP.width;
                    p->height = MANDELBROT_PARAMETERS_DEFAULT_MP.height;
                    initialiseMP(p);
                    break;
                #endif

                default:
                    return 1;
            }

            break;
        default:
            return 1;
    }

    p->output = OUTPUT_PNM;

    strncpy(p->plotFilepath, PLOT_FILEPATH_DEFAULT, sizeof(p->plotFilepath));
    p->plotFilepath[sizeof(p->plotFilepath) - 1] = '\0';
    
    if (initialiseColourScheme(&(p->colour), COLOUR_SCHEME_DEFAULT))
        return 1;

    return 0;
}


static int initialiseTerminalOutputParameters(PlotCTX *p)
{
    switch(p->type)
    {
        case PLOT_JULIA:
            switch (p->precision)
            {
                case STD_PRECISION:
                    *p = JULIA_PARAMETERS_DEFAULT;
                    break;
                case EXT_PRECISION:
                    *p = JULIA_PARAMETERS_DEFAULT_EXT;
                    break;

                #ifdef MP_PREC
                case MUL_PRECISION:
                    p->iterations = JULIA_PARAMETERS_DEFAULT_MP.iterations;
                    initialiseMP(p);
                    break;
                #endif

                default:
                    return 1;
            }

            p->width = JULIA_TERMINAL_WIDTH_DEFAULT;
            p->height = JULIA_TERMINAL_HEIGHT_DEFAULT;
            
            break;
        case PLOT_MANDELBROT:
            switch (p->precision)
            {
                case STD_PRECISION:
                    *p = MANDELBROT_PARAMETERS_DEFAULT;
                    break;
                case EXT_PRECISION:
                    *p = MANDELBROT_PARAMETERS_DEFAULT_EXT;
                    break;
                
                #ifdef MP_PREC
                case MUL_PRECISION:
                    p->iterations = MANDELBROT_PARAMETERS_DEFAULT_MP.iterations;
                    initialiseMP(p);
                    break;
                #endif

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
        return 1;

    return 0;
}


#ifdef MP_PREC
/* Initialise MP parameters to extended-precision defaults */
static int initialiseMP(PlotCTX *p)
{
    long double complex minimum;
    long double complex maximum;

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
#endif