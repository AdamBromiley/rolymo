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
static long double dotProductExt(long double complex z);

static complex mandelbrot(unsigned long *n, complex c, unsigned long max);
static long double complex mandelbrotExt(unsigned long *n, long double complex c, unsigned long max);

static complex julia(unsigned long *n, complex z, complex c, unsigned long max);
static long double complex juliaExt(unsigned long *n, long double complex z, long double complex c, unsigned long max);


void * generateFractal(void *threadInfo)
{
    Thread *t = threadInfo;

    unsigned int tCount = t->ctx->count;

    /* Plot parameters */
    PlotCTX *p = t->block->ctx->array->parameters;

    /* Julia set constant */
    complex constant = p->c.c;

    /* Maximum iteration count */
    unsigned long nMax = p->iterations;

    PlotType type = p->type;
    ColourScheme *colour = &(p->colour);
    BitDepth colourDepth = colour->depth;

    /* Values at top-left of plot */
    double reMin = creal(p->minimum.c);
    double imMax = cimag(p->maximum.c);

    /* Pixel dimensions */
    double pxWidth = (creal(p->maximum.c) - creal(p->minimum.c)) / (p->width - 1);
    double pxHeight = (cimag(p->maximum.c) - cimag(p->minimum.c)) / p->height;

    /* Image array */
    char *px;
    char *array = (char *) t->block->ctx->array->array;
    size_t rows = t->block->rows;
    size_t columns = p->width;
    size_t nmemb = (colour->depth != BIT_DEPTH_1) ? colour->depth / CHAR_BIT : sizeof(char);
    size_t rowSize = columns * nmemb;

    /* Offset of block from start ('top-left') of image array */
    size_t blockOffset = t->block->id * rows;
    double rowOffset = imMax - blockOffset * pxHeight;

    logMessage(INFO, "Thread %u: Generating plot", t->tid);

    /* Offset by thread ID to ensure each thread gets a unique row */
    for (size_t y = t->tid; y < rows; y += tCount)
    {
        /* Number of bits into current byte (if bit depth < CHAR_BIT) */
        int bitOffset;

        /* Set complex value to start of the row */
        complex c = reMin + (rowOffset - y * pxHeight) * I;

        /* Set pixel pointer to start of the row */
        if (colourDepth >= CHAR_BIT)
        {
            px = array + y * rowSize;
        }
        else
        {
            bitOffset = 0;
            px = array + y * rowSize / CHAR_BIT;
        }

        /* Iterate over the row */
        for (size_t x = 0; x < columns; ++x, c += pxWidth)
        {
            complex z;
            unsigned long n;

            /* Run fractal function on c */
            switch (type)
            {
                case PLOT_JULIA:
                    z = julia(&n, c, constant, nMax);
                    break;
                case PLOT_MANDELBROT:
                    z = mandelbrot(&n, c, nMax);
                    break;
                default:
                    pthread_exit(NULL);
            }

            /* Map iteration count to RGB colour value */
            mapColour(px, n, z, bitOffset, nMax, colour);

            /* Increment pixel pointer */
            if (colourDepth >= CHAR_BIT)
            {
                px += nmemb;
            }
            else if (++bitOffset == CHAR_BIT)
            {
                px += nmemb;
                bitOffset = 0;
            }
        }
    }

    logMessage(INFO, "Thread %u: Plot generated - exiting", t->tid);
    
    pthread_exit(NULL);
}


void * generateFractalExt(void *threadInfo)
{
    Thread *t = threadInfo;

    unsigned int tCount = t->ctx->count;

    /* Plot parameters */
    PlotCTX *p = t->block->ctx->array->parameters;

    /* Julia set constant */
    long double complex constant = p->c.lc;

    /* Maximum iteration count */
    unsigned long nMax = p->iterations;

    PlotType type = p->type;
    ColourScheme *colour = &(p->colour);
    BitDepth colourDepth = colour->depth;

    /* Values at top-left of plot */
    long double reMin = creall(p->minimum.lc);
    long double imMax = cimagl(p->maximum.lc);
    
    /* Pixel dimensions */
    long double pxWidth = (creall(p->maximum.lc) - creall(p->minimum.lc)) / (p->width - 1);
    long double pxHeight = (cimagl(p->maximum.lc) - cimagl(p->minimum.lc)) / p->height;

    /* Image array */
    char *px;
    char *array = (char *) t->block->ctx->array->array;
    size_t rows = t->block->rows;
    size_t columns = p->width;
    size_t nmemb = (colour->depth != BIT_DEPTH_1) ? colour->depth / CHAR_BIT : sizeof(char);
    size_t rowSize = columns * nmemb;

    /* Offset of block from start ('top-left') of image array */
    size_t blockOffset = t->block->id * rows;
    long double rowOffset = imMax - blockOffset * pxHeight;

    logMessage(INFO, "Thread %u: Generating plot", t->tid);

    /* Offset by thread ID to ensure each thread gets a unique row */
    for (size_t y = t->tid; y < rows; y += tCount)
    {
        /* Number of bits into current byte (if bit depth < CHAR_BIT) */
        int bitOffset;

        /* Set complex value to start of the row */
        long double complex c = reMin + (rowOffset - y * pxHeight) * I;

        /* Set pixel pointer to start of the row */
        if (colourDepth >= CHAR_BIT)
        {
            px = array + y * rowSize;
        }
        else
        {
            bitOffset = 0;
            px = array + y * rowSize / CHAR_BIT;
        }

        /* Iterate over the row */
        for (size_t x = 0; x < columns; ++x, c += pxWidth)
        {
            long double complex z;
            unsigned long n;

            /* Run fractal function on c */
            switch (type)
            {
                case PLOT_JULIA:
                    z = juliaExt(&n, c, constant, nMax);
                    break;
                case PLOT_MANDELBROT:
                    z = mandelbrotExt(&n, c, nMax);
                    break;
                default:
                    pthread_exit(NULL);
            }

            /* Map iteration count to RGB colour value */
            mapColourExt(px, n, z, bitOffset, nMax, colour);

            /* Increment pixel pointer */
            if (colourDepth >= CHAR_BIT)
            {
                px += nmemb;
            }
            else if (++bitOffset == CHAR_BIT)
            {
                px += nmemb;
                bitOffset = 0;
            }
        }
    }

    logMessage(INFO, "Thread %u: Plot generated - exiting", t->tid);
    
    pthread_exit(NULL);
}


static double dotProduct(complex z)
{
    return creal(z) * creal(z) + cimag(z) * cimag(z);
}


static long double dotProductExt(long double complex z)
{
    return creall(z) * creall(z) + cimagl(z) * cimagl(z);
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


/* Perform Mandelbrot set function (extended-precision) */
static long double complex mandelbrotExt(unsigned long *n, long double complex c, unsigned long max)
{
    long double complex z = 0.0L + 0.0L * I;
    long double cdot = dotProductExt(c);

    if (256.0L * cdot * cdot - 96.0L * cdot + 32.0L * creall(c) - 3.0L >= 0.0L
        && 16.0L * (cdot + 2.0L * creall(c) + 1.0L) - 1.0L >= 0.0L)
    {
        for (*n = 0; cabsl(z) < ESCAPE_RADIUS_EXT && *n < max; ++(*n))
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


/* Perform Julia set function (extended-precision) */
static long double complex juliaExt(unsigned long *n, long double complex z, long double complex c, unsigned long max)
{
    for (*n = 0; cabsl(z) < ESCAPE_RADIUS && *n < max; ++(*n))
        z = z * z + c;

    return z;
}