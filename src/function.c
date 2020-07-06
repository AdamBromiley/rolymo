#include "function.h"

#include <complex.h>
#include <limits.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

#include <mpfr.h>
#include <mpc.h>

#include "libgroot/include/log.h"

#include "array.h"
#include "colour.h"
#include "mandelbrot_parameters.h"
#include "parameters.h"


static double dotProduct(complex z);
static long double dotProductExt(long double complex z);

static complex mandelbrot(unsigned long *n, complex c, unsigned long max);
static long double complex mandelbrotExt(unsigned long *n, long double complex c, unsigned long max);
static void mandelbrotArb(unsigned long *n, mpc_t z, mpfr_t norm, mpc_t c, unsigned long max);

static complex julia(unsigned long *n, complex z, complex c, unsigned long max);
static long double complex juliaExt(unsigned long *n, long double complex z, long double complex c, unsigned long max);
static void juliaArb(unsigned long *n, mpc_t z, mpfr_t norm, mpc_t c, unsigned long max);


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


void * generateFractalArb(void *threadInfo)
{
    Thread *t = threadInfo;

    unsigned int tCount = t->ctx->count;

    /* Plot parameters */
    PlotCTX *p = t->block->ctx->array->parameters;

    /* Julia set constant */
    mpc_t constant;
    mpc_init2(constant, ARB_PRECISION_BITS);
    mpc_set(constant, p->c.mpc, ARB_CMPLX_ROUNDING);

    /* Maximum iteration count */
    unsigned long nMax = p->iterations;

    PlotType type = p->type;
    ColourScheme *colour = &(p->colour);
    BitDepth colourDepth = colour->depth;

    /* Values at top-left of plot */
    mpfr_t reMin, imMax;
    mpfr_init2(reMin, ARB_PRECISION_BITS);
    mpfr_init2(imMax, ARB_PRECISION_BITS);

    mpfr_set(reMin, mpc_realref(p->minimum.mpc), ARB_REAL_ROUNDING);
    mpfr_set(imMax, mpc_imagref(p->maximum.mpc), ARB_IMAG_ROUNDING);

    /* Image array */
    char *px;
    char *array = (char *) t->block->ctx->array->array;
    size_t rows = t->block->rows;
    size_t columns = p->width;
    size_t nmemb = (colour->depth != BIT_DEPTH_1) ? colour->depth / CHAR_BIT : sizeof(char);
    size_t rowSize = columns * nmemb;

    /* Width value */
    mpfr_t width, pxWidth;
    mpfr_init2(width, ARB_PRECISION_BITS);
    mpfr_init2(pxWidth, ARB_PRECISION_BITS);

    /* TODO: subtract 1? */
    mpfr_set_uj(width, (uintmax_t) p->width, ARB_REAL_ROUNDING);
    mpfr_sub(pxWidth, mpc_realref(p->maximum.mpc), mpc_realref(p->minimum.mpc), ARB_REAL_ROUNDING);
    mpfr_div(pxWidth, pxWidth, width, ARB_REAL_ROUNDING);

    mpfr_clear(width);

    /* Height value */
    mpfr_t height, pxHeight;
    mpfr_init2(height, ARB_PRECISION_BITS);
    mpfr_init2(pxHeight, ARB_PRECISION_BITS);

    mpfr_set_uj(height, (uintmax_t) p->height, ARB_IMAG_ROUNDING);
    mpfr_sub(pxHeight, mpc_imagref(p->maximum.mpc), mpc_imagref(p->minimum.mpc), ARB_IMAG_ROUNDING);
    mpfr_div(pxHeight, pxHeight, height, ARB_IMAG_ROUNDING);

    mpfr_clear(height);

    /* Offset of block from start ('top-left') of image array */
    mpfr_t blockOffset, rowOffset;

    mpfr_init2(blockOffset, ARB_PRECISION_BITS);
    mpfr_init2(rowOffset, ARB_PRECISION_BITS);

    mpfr_set_uj(blockOffset, (uintmax_t) (t->block->id * rows + t->tid), ARB_IMAG_ROUNDING);
    mpfr_mul(blockOffset, blockOffset, pxHeight, ARB_IMAG_ROUNDING);
    mpfr_sub(rowOffset, imMax, blockOffset, ARB_IMAG_ROUNDING);

    mpfr_clear(blockOffset);

    /* Calculation variables */
    mpc_t z, c;
    mpc_init2(z, ARB_PRECISION_BITS);
    mpc_init2(c, ARB_PRECISION_BITS);

    mpfr_t norm;
    mpfr_init2(norm, ARB_PRECISION_BITS);

    mpfr_mul_ui(pxHeight, pxHeight, (unsigned long) tCount, ARB_IMAG_ROUNDING);

    logMessage(INFO, "Thread %u: Generating plot", t->tid);

    /* Offset by thread ID to ensure each thread gets a unique row */
    for (size_t y = t->tid; y < rows; y += tCount)
    {
        /* Number of bits into current byte (if bit depth < CHAR_BIT) */
        int bitOffset;

        /* Set complex value to start of the row */
        mpc_set_fr_fr(c, reMin, rowOffset, ARB_CMPLX_ROUNDING);

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
        for (size_t x = 0; x < columns; ++x, mpc_add_fr(c, c, pxWidth, ARB_REAL_ROUNDING))
        {
            unsigned long n;

            /* Run fractal function on c */
            switch (type)
            {
                case PLOT_JULIA:
                    juliaArb(&n, c, norm, constant, nMax);
                    break;
                case PLOT_MANDELBROT:
                    mandelbrotArb(&n, z, norm, c, nMax);
                    break;
                default:
                    mpfr_clears(reMin, imMax, pxWidth, pxHeight, rowOffset, norm, NULL);
                    mpc_clear(constant);
                    mpc_clear(z);
                    mpc_clear(c);
                    pthread_exit(NULL);
            }

            /* Map iteration count to RGB colour value */
            mapColourArb(px, n, norm, bitOffset, nMax, colour);

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

        mpfr_sub(rowOffset, rowOffset, pxHeight, ARB_IMAG_ROUNDING);
    }

    mpfr_clears(reMin, imMax, pxWidth, pxHeight, rowOffset, norm, NULL);
    mpc_clear(constant);
    mpc_clear(z);
    mpc_clear(c);

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


/* Perform Mandelbrot set function (arbitrary-precision) */
static void mandelbrotArb(unsigned long *n, mpc_t z, mpfr_t norm, mpc_t c, unsigned long max)
{
    mpc_set_d_d(z, 0.0, 0.0, ARB_CMPLX_ROUNDING);
    mpc_norm(norm, z, ARB_REAL_ROUNDING);

    for (*n = 0; mpfr_cmp_d(norm, ESCAPE_RADIUS_ARB * ESCAPE_RADIUS_ARB) < 0 && *n < max; ++(*n))
    {
        mpc_sqr(z, z, ARB_CMPLX_ROUNDING);
        mpc_add(z, z, c, ARB_CMPLX_ROUNDING);
        mpc_norm(norm, z, ARB_REAL_ROUNDING);
    }

    return;
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


/* Perform Julia set function (arbitrary-precision) */
static void juliaArb(unsigned long *n, mpc_t z, mpfr_t norm, mpc_t c, unsigned long max)
{
    mpc_norm(norm, z, ARB_REAL_ROUNDING);
    
    for (*n = 0; mpfr_cmp_d(norm, ESCAPE_RADIUS * ESCAPE_RADIUS) < 0 && *n < max; ++(*n))
    {
        mpc_sqr(z, z, ARB_CMPLX_ROUNDING);
        mpc_add(z, z, c, ARB_CMPLX_ROUNDING);
        mpc_norm(norm, z, ARB_REAL_ROUNDING);
    }

    return;
}