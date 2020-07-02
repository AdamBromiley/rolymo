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


static long double dotProduct(long double complex z);
static long double complex mandelbrot(unsigned long *n, long double complex c, unsigned long max);
static long double complex julia(unsigned long *n, long double complex z, long double complex c, unsigned long max);


void * generateFractal(void *threadInfo)
{
    struct Thread *thread = threadInfo;
    unsigned int threadCount = thread->ctx->count;

    struct PlotCTX *parameters = thread->block->ctx->array->parameters;
    enum PlotType plot = parameters->type;
    struct ColourScheme *colour = &(parameters->colour);
    enum BitDepth colourDepth = colour->depth;

    size_t rows = thread->block->rows;
    size_t columns = parameters->width;

    char *pixel;
    char *array = (char *) thread->block->ctx->array->array;
    size_t arrayMemberSize = (colour->depth != BIT_DEPTH_1) ? colour->depth / 8 : sizeof(char);

    long double xMin = creall(parameters->minimum);
    long double yMax = cimagl(parameters->maximum);
    
    long double pixelWidth = (creall(parameters->maximum) - creall(parameters->minimum)) / (parameters->width - 1);
    long double pixelHeight = (cimagl(parameters->maximum) - cimagl(parameters->minimum)) / parameters->height;

    size_t blockOffset = thread->block->id * rows;
    long double rowOffset = yMax - blockOffset * pixelHeight;

    long double complex constant = parameters->c;

    unsigned long max = parameters->iterations;

    logMessage(INFO, "Thread %u: Generating plot", thread->tid);

    /* Offset by thread ID to ensure each thread gets a unique row */
    for (size_t y = thread->tid; y < rows; y += threadCount)
    {
        int bitOffset;

        /* Reset to start of row */
        long double complex c = xMin + (rowOffset - y * pixelHeight) * I;

        /* Repoint to first pixel of the row */
        if (colourDepth != BIT_DEPTH_1)
        {
            pixel = array + y * columns * arrayMemberSize;
        }
        else
        {
            bitOffset = 0;
            pixel = array + y * (columns / 8) * arrayMemberSize;
        }

        /* Iterate over the row */
        for (size_t x = 0; x < columns; ++x, c += pixelWidth)
        {
            long double complex z;
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

            if (colourDepth != BIT_DEPTH_1)
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


static long double dotProduct(long double complex z)
{
    return creall(z) * creall(z) + cimagl(z) * cimagl(z);
}


/* Perform Mandelbrot set function */
static long double complex mandelbrot(unsigned long *n, long double complex c, unsigned long max)
{
    long double complex z = 0.0L + 0.0L * I;
    long double cdot = dotProduct(c);

    if (256.0L * cdot * cdot - 96.0L * cdot + 32.0L * creall(c) - 3.0L >= 0.0L
        && 16.0L * (cdot + 2.0L * creall(c) + 1.0L) - 1.0L >= 0.0L)
    {
        for (*n = 0; cabsl(z) < ESCAPE_RADIUS && *n < max; ++(*n))
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
static long double complex julia(unsigned long *n, long double complex z, long double complex c, unsigned long max)
{
    for (*n = 0; cabsl(z) < ESCAPE_RADIUS && *n < max; ++(*n))
        z = z * z + c;

    return z;
}