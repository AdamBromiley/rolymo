#include "function.h"

#include <complex.h>
#include <limits.h>
#include <pthread.h>
#include <stddef.h>

#include "libgroot/include/log.h"

#include "array.h"
#include "colour.h"
#include "mandelbrot_parameters.h"
#include "parameters.h"


static double dotProduct(complex z);
static complex mandelbrot(unsigned long *n, complex c, unsigned long max);
static complex julia(unsigned long *n, complex z, complex c, unsigned long max);


void * generateFractal(void *threadInfo)
{
    struct Thread *thread = threadInfo;
    unsigned int threadCount = thread->ctx->count;

    struct PlotCTX *parameters = thread->block->ctx->array->parameters;
    enum PlotType plot = parameters->type;
    struct ColourScheme *colour = &(parameters->colour);

    size_t rows = thread->block->rows;
    size_t columns = parameters->width;

    char *array = (char *) thread->block->ctx->array->array;
    char *pixel;
    size_t arrayMemberSize = (colour->depth != BIT_DEPTH_1) ? colour->depth / 8 : sizeof(char);
    
    double pixelWidth = (creal(parameters->maximum) - creal(parameters->minimum)) / (parameters->width - 1);
    double pixelHeight = (cimag(parameters->maximum) - cimag(parameters->minimum)) / parameters->height;

    complex constant = parameters->c;
    complex cReStep = pixelWidth;
    unsigned long max = parameters->iterations;

    /* Set to top left of plot block (minimum real, maximum block imaginary) */

    logMessage(INFO, "Thread %u: Generating plot", thread->tid);

    /* Offset by thread ID to ensure each thread gets a unique row */
    for (size_t y = thread->tid; y < rows; y += threadCount)
    {
        int bitOffset = 0;

        complex c = creal(parameters->minimum)
                + (cimag(parameters->maximum) - (thread->block->id * rows + y) * pixelHeight) * I;

        /* Repoint to first pixel of the row */
        if (colour->depth != BIT_DEPTH_1)
            pixel = array + y * columns * arrayMemberSize;
        else
            pixel = array + y * (columns / 8) * arrayMemberSize;

        /* Iterate over the row */
        for (size_t x = 0; x < columns; ++x, c += cReStep)
        {
            complex z;
            unsigned long n;

            switch (plot)
            {
                case PLOT_JULIA:
                    z = julia(&n, c, constant, max);
                    break;
                case PLOT_MANDELBROT:
                    z = mandelbrot(&n, c, max);
                    break;
                default:
                    pthread_exit(0);
            }

            mapColour(pixel, n, z, bitOffset, max, colour);

            if (colour->depth != BIT_DEPTH_1)
            {
                pixel += arrayMemberSize;
            }
            else if (++bitOffset == CHAR_BIT)
            {
                pixel += arrayMemberSize;
                bitOffset = 0;
            }
        }
    }

    logMessage(INFO, "Thread %u: Plot generated - exiting", thread->tid);
    
    pthread_exit(0);
}


static double dotProduct(complex z)
{
    return creal(z) * creal(z) + cimag(z) * cimag(z);
}


/* Perform Mandelbrot set function */
static complex mandelbrot(unsigned long *n, complex c, unsigned long max)
{
    complex z = 0.0 + 0.0 * I;
    double cdot = dotProduct(c);

    if (256.0 * cdot * cdot - 96.0 * cdot + 32.0 * creal(c) - 3.0 >= 0.0
        && 16.0 * (cdot + 2.0 * creal(c) + 1.0) - 1.0 >= 0.0)
    {
        for (*n = 0; cabs(z) < ESCAPE_RADIUS && *n < max; ++(*n))
            z = z * z + c;
    }
    else
    {
        /* Ignore main and secondary bulb */
        *n = max;
    }

    return z;
}


/* Perform Julia set function */
static complex julia(unsigned long *n, complex z, complex c, unsigned long max)
{
    for (*n = 0; cabs(z) < ESCAPE_RADIUS && *n < max; ++(*n))
        z = z * z + c;

    return z;
}