#include <parameters.h>


const struct ComplexNumber COMPLEX_MIN = {DBL_MIN, DBL_MIN};
const struct ComplexNumber COMPLEX_MAX = {DBL_MAX, DBL_MAX};

const double ESCAPE_RADIUS = 2.0;

const unsigned int ITERATIONS_MIN = 0;
const unsigned int ITERATIONS_MAX = UINT_MAX;

const size_t WIDTH_MIN = 0;
const size_t WIDTH_MAX = SIZE_MAX;
const size_t HEIGHT_MIN = 0;
const size_t HEIGHT_MAX = SIZE_MAX;

const size_t TERMINAL_WIDTH = 80;
const size_t TERMINAL_HEIGHT = 46;


/* Set default plot settings for Mandelbrot image output */
int initialiseParameters(struct PlotCTX *parameters, enum PlotType type)
{
    /* Minimum and maximum numbers to plot */
    struct ComplexNumber mandelbrotMinDefault = {-(ESCAPE_RADIUS), -1.25};
    struct ComplexNumber mandelbrotMaxDefault = {0.75, 1.25};

    struct ComplexNumber juliaMinDefault = {-(ESCAPE_RADIUS), ESCAPE_RADIUS};
    struct ComplexNumber juliaMaxDefault = {ESCAPE_RADIUS, ESCAPE_RADIUS};

    /* Maximum iteration count */
    const unsigned int ITERATIONS_DEFAULT = 100;

    /* Pixels per unit */
    const double DEFAULT_SCALE_FACTOR = 200.0;

    /* Dimensions */
    size_t mandelbrotWidthDefault = (mandelbrotMaxDefault.re - mandelbrotMinDefault.re) * DEFAULT_SCALE_FACTOR;
    size_t mandelbrotHeightDefault = (mandelbrotMaxDefault.im - mandelbrotMinDefault.im) * DEFAULT_SCALE_FACTOR;

    size_t juliaWidthDefault = (juliaMaxDefault.re - juliaMinDefault.re) * DEFAULT_SCALE_FACTOR;
    size_t juliaHeightDefault = (juliaMaxDefault.im - juliaMinDefault.im) * DEFAULT_SCALE_FACTOR;

    /* Default colour scheme */
    const enum ColourScheme colour = SCHEME_DEFAULT;

    switch (type)
    {
        case PLOT_MANDELBROT:
            parameters->minimum     = mandelbrotMinDefault;
            parameters->maximum     = mandelbrotMaxDefault;
            parameters->iterations  = ITERATIONS_DEFAULT;
            parameters->output      = OUTPUT_PPM;
            parameters->width       = mandelbrotWidthDefault;
            parameters->height      = mandelbrotHeightDefault;
            parameters->colour      = SCHEME_DEFAULT;
            break;
        case PLOT_JULIA:
            parameters->minimum     = juliaMinDefault;
            parameters->maximum     = juliaMaxDefault;
            parameters->iterations  = ITERATIONS_DEFAULT;
            parameters->output      = OUTPUT_PPM;
            parameters->width       = juliaWidthDefault;
            parameters->height      = juliaHeightDefault;
            parameters->colour      = SCHEME_DEFAULT;
            break;
        default:
            return 1;
    }

    return 0;
}