#include "function.h"

#include <complex.h>
#include <limits.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

#ifdef MP_PREC
#include <mpfr.h>
#include <mpc.h>
#endif

#include "libgroot/include/log.h"

#include "array.h"
#include "colour.h"
#include "mandelbrot_parameters.h"
#include "parameters.h"


static double dotProduct(complex z);
static long double dotProductExt(long double complex z);

static complex mandelbrot(unsigned long *n, complex c, unsigned long max);
static long double complex mandelbrotExt(unsigned long *n, long double complex c, unsigned long max);

#ifdef MP_PREC
static void mandelbrotMP(unsigned long *n, mpc_t z, mpfr_t norm, mpc_t c, unsigned long max);
#endif

static complex julia(unsigned long *n, complex z, complex c, unsigned long max);
static long double complex juliaExt(unsigned long *n, long double complex z, long double complex c, unsigned long max);

#ifdef MP_PREC
static void juliaMP(unsigned long *n, mpc_t z, mpfr_t norm, mpc_t c, unsigned long max);
#endif


void * generateFractal(void *threadInfo)
{
    Thread *t = threadInfo;

    /*
     * Because the loops may run for billions of iterations, all relevant struct
     * members are cached before use.
     */

    unsigned int tCount = t->ctx->count;

    /* Plot parameters */
    PlotCTX *p = t->block->ctx->array->params;

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
    double pxWidth = (p->width > 1) ? (creal(p->maximum.c) - creal(p->minimum.c)) / (p->width - 1) : 0.0;
    double pxHeight = (p->height > 1) ? (cimag(p->maximum.c) - cimag(p->minimum.c)) / (p->height - 1) : 0.0;

    /* Image array */
    char *px;
    char *array = (char *) t->block->ctx->array->array;
    size_t rows = t->block->rows;
    size_t columns = p->width;
    size_t nmemb = (colourDepth <= CHAR_BIT || colourDepth == BIT_DEPTH_ASCII)
                   ? sizeof(char)
                   : colourDepth / CHAR_BIT;

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
        if (colourDepth >= CHAR_BIT || colourDepth == BIT_DEPTH_ASCII)
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
            if (colourDepth >= CHAR_BIT || colourDepth == BIT_DEPTH_ASCII)
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

    /*
     * Because the loops may run for billions of iterations, all relevant struct
     * members are cached before use.
     */

    unsigned int tCount = t->ctx->count;

    /* Plot parameters */
    PlotCTX *p = t->block->ctx->array->params;

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
    long double pxWidth = (p->width > 1) ? (creall(p->maximum.lc) - creall(p->minimum.lc)) / (p->width - 1) : 0.0L;
    long double pxHeight = (p->height > 1) ? (cimagl(p->maximum.lc) - cimagl(p->minimum.lc)) / (p->height - 1) : 0.0L;

    /* Image array */
    char *px;
    char *array = (char *) t->block->ctx->array->array;
    size_t rows = t->block->rows;
    size_t columns = p->width;
    size_t nmemb = (colourDepth <= CHAR_BIT || colourDepth == BIT_DEPTH_ASCII)
                   ? sizeof(char)
                   : colourDepth / CHAR_BIT;
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
        if (colourDepth >= CHAR_BIT || colourDepth == BIT_DEPTH_ASCII)
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
            if (colourDepth >= CHAR_BIT || colourDepth == BIT_DEPTH_ASCII)
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


#ifdef MP_PREC
void * generateFractalMP(void *threadInfo)
{
    Thread *t = threadInfo;

    /*
     * Because the loops may run for billions of iterations, all relevant struct
     * members are cached before use.
     */

    unsigned int tCount = t->ctx->count;

    /* Plot parameters */
    PlotCTX *p = t->block->ctx->array->params;

    /* Julia set constant */
    mpc_t constant;
    mpc_init2(constant, mpSignificandSize);
    mpc_set(constant, p->c.mpc, MP_COMPLEX_RND);

    /* Maximum iteration count */
    unsigned long nMax = p->iterations;

    PlotType type = p->type;
    ColourScheme *colour = &(p->colour);
    BitDepth colourDepth = colour->depth;

    /* Values at top-left of plot */
    mpfr_t reMin, imMax;
    mpfr_init2(reMin, mpSignificandSize);
    mpfr_init2(imMax, mpSignificandSize);

    mpfr_set(reMin, mpc_realref(p->minimum.mpc), MP_REAL_RND);
    mpfr_set(imMax, mpc_imagref(p->maximum.mpc), MP_IMAG_RND);

    /* Image array */
    char *px;
    char *array = (char *) t->block->ctx->array->array;
    size_t rows = t->block->rows;
    size_t columns = p->width;
    size_t nmemb = (colourDepth <= CHAR_BIT || colourDepth == BIT_DEPTH_ASCII)
                   ? sizeof(char)
                   : colourDepth / CHAR_BIT;
    size_t rowSize = columns * nmemb;

    /* Width value */
    mpfr_t pxWidth;
    mpfr_init2(pxWidth, mpSignificandSize);

    if (p->width > 1)
    {
        mpfr_t width;
        mpfr_init2(width, mpSignificandSize);

        mpfr_set_uj(width, (uintmax_t) (p->width - 1), MP_REAL_RND);
        mpfr_sub(pxWidth, mpc_realref(p->maximum.mpc), mpc_realref(p->minimum.mpc), MP_REAL_RND);
        mpfr_div(pxWidth, pxWidth, width, MP_REAL_RND);

        mpfr_clear(width);
    }
    else
    {
        mpfr_set_d(pxWidth, 0.0, MP_REAL_RND);
    }

    /* Height value */
    mpfr_t pxHeight;
    mpfr_init2(pxHeight, mpSignificandSize);

    if (p->height > 1)
    {
        mpfr_t height;
        mpfr_init2(height, mpSignificandSize);

        mpfr_set_uj(height, (uintmax_t) (p->height - 1), MP_IMAG_RND);
        mpfr_sub(pxHeight, mpc_imagref(p->maximum.mpc), mpc_imagref(p->minimum.mpc), MP_IMAG_RND);
        mpfr_div(pxHeight, pxHeight, height, MP_IMAG_RND);

        /* Rather than calculate row im-value as rowOffset - y * pxHeight, just subtract pxHeight * tCount each time */
        mpfr_mul_ui(pxHeight, pxHeight, (unsigned long) tCount, MP_IMAG_RND);

        mpfr_clear(height);
    }
    else
    {
        mpfr_set_d(pxHeight, 0.0, MP_IMAG_RND);
    }

    /* Offset of block from start ('top-left') of image array */
    mpfr_t blockOffset, rowOffset;
    mpfr_init2(blockOffset, mpSignificandSize);
    mpfr_init2(rowOffset, mpSignificandSize);

    mpfr_set_uj(blockOffset, (uintmax_t) (t->block->id * rows + t->tid), MP_IMAG_RND);
    mpfr_mul(blockOffset, blockOffset, pxHeight, MP_IMAG_RND);
    mpfr_sub(rowOffset, imMax, blockOffset, MP_IMAG_RND);

    mpfr_clear(blockOffset);

    /* Calculation variables */
    mpc_t z, c;
    mpc_init2(z, mpSignificandSize);
    mpc_init2(c, mpSignificandSize);

    mpfr_t norm;
    mpfr_init2(norm, mpSignificandSize);

    logMessage(INFO, "Thread %u: Generating plot", t->tid);

    /* Offset by thread ID to ensure each thread gets a unique row */
    for (size_t y = t->tid; y < rows; y += tCount)
    {
        /* Number of bits into current byte (if bit depth < CHAR_BIT) */
        int bitOffset;

        /* Set complex value to start of the row */
        mpc_set_fr_fr(c, reMin, rowOffset, MP_COMPLEX_RND);

        /* Set pixel pointer to start of the row */
        if (colourDepth >= CHAR_BIT || colourDepth == BIT_DEPTH_ASCII)
        {
            px = array + y * rowSize;
        }
        else
        {
            bitOffset = 0;
            px = array + y * rowSize / CHAR_BIT;
        }

        /* Iterate over the row */
        for (size_t x = 0; x < columns; ++x, mpc_add_fr(c, c, pxWidth, MP_REAL_RND))
        {
            unsigned long n;

            /* Run fractal function on c */
            switch (type)
            {
                case PLOT_JULIA:
                    juliaMP(&n, c, norm, constant, nMax);
                    break;
                case PLOT_MANDELBROT:
                    mandelbrotMP(&n, z, norm, c, nMax);
                    break;
                default:
                    mpfr_clears(reMin, imMax, pxWidth, pxHeight, rowOffset, norm, NULL);
                    mpc_clear(constant);
                    mpc_clear(z);
                    mpc_clear(c);
                    pthread_exit(NULL);
            }

            /* Map iteration count to RGB colour value */
            mapColourMP(px, n, norm, bitOffset, nMax, colour);

            /* Increment pixel pointer */
            if (colourDepth >= CHAR_BIT || colourDepth == BIT_DEPTH_ASCII)
            {
                px += nmemb;
            }
            else if (++bitOffset == CHAR_BIT)
            {
                px += nmemb;
                bitOffset = 0;
            }
        }

        mpfr_sub(rowOffset, rowOffset, pxHeight, MP_IMAG_RND);
    }

    mpfr_clears(reMin, imMax, pxWidth, pxHeight, rowOffset, norm, NULL);
    mpc_clear(constant);
    mpc_clear(z);
    mpc_clear(c);

    logMessage(INFO, "Thread %u: Plot generated - exiting", t->tid);
    
    pthread_exit(NULL);
}
#endif


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


#ifdef MP_PREC
/* Perform Mandelbrot set function (multiple-precision) */
static void mandelbrotMP(unsigned long *n, mpc_t z, mpfr_t norm, mpc_t c, unsigned long max)
{
    mpc_set_d_d(z, 0.0, 0.0, MP_COMPLEX_RND);
    mpc_norm(norm, z, MP_REAL_RND);

    for (*n = 0; mpfr_cmp_d(norm, ESCAPE_RADIUS_MP * ESCAPE_RADIUS_MP) < 0 && *n < max; ++(*n))
    {
        mpc_sqr(z, z, MP_COMPLEX_RND);
        mpc_add(z, z, c, MP_COMPLEX_RND);
        mpc_norm(norm, z, MP_REAL_RND);
    }

    return;
}
#endif


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


#ifdef MP_PREC
/* Perform Julia set function (multiple-precision) */
static void juliaMP(unsigned long *n, mpc_t z, mpfr_t norm, mpc_t c, unsigned long max)
{
    mpc_norm(norm, z, MP_REAL_RND);
    
    for (*n = 0; mpfr_cmp_d(norm, ESCAPE_RADIUS * ESCAPE_RADIUS) < 0 && *n < max; ++(*n))
    {
        mpc_sqr(z, z, MP_COMPLEX_RND);
        mpc_add(z, z, c, MP_COMPLEX_RND);
        mpc_norm(norm, z, MP_REAL_RND);
    }

    return;
}
#endif