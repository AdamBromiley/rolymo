#include "function.h"

#include <complex.h>
#include <limits.h>
#include <pthread.h>
#include <stddef.h>

#include "array.h"
#include "colour.h"
#include "log.h"
#include "mandelbrot_parameters.h"
#include "parameters.h"


void * mandelbrot(void *threadInfo)
{
    struct Thread *thread = threadInfo;
    unsigned int threadCount = thread->block->ctx->array->threadCount;

    struct PlotCTX *parameters = thread->block->ctx->array->parameters;
    struct ColourScheme *colour = &(parameters->colour);

    size_t x, y;
    size_t rows = thread->block->rows;
    size_t columns = parameters->width;

    size_t arrayMemberSize = (colour->depth != BIT_DEPTH_1) ? colour->depth / 8 : sizeof(char);
    void *pixel = thread->block->ctx->array->array;
    double pixelWidth = (parameters->maximum.re - parameters->minimum.re) / (parameters->width - 1);
    double pixelHeight = (parameters->maximum.im - parameters->minimum.im) / parameters->height;

    int bitOffset = 1;

    complex z, c;
    unsigned long int n;
    unsigned long int maxIterations = parameters->iterations;

    logMessage(INFO, "Thread %u: Generating plot", thread->threadID);

    /* Set to top left of plot block (minimum real, maximum block imaginary) */
    c = parameters->minimum.re +
        (parameters->maximum.im - (thread->block->blockID * rows + thread->threadID) * pixelHeight) * I;

    /* Offset by threadID to ensure each thread gets a unique row */
    for (y = thread->threadID; y < rows; y += threadCount, c -= pixelHeight * threadCount * I)
    {
        /* Repoint to first pixel of the row */
        pixel = (char *) thread->block->ctx->array->array + y * columns * arrayMemberSize;

        /* Iterate over the row */
        for (x = 0; x < columns; ++x, c += pixelWidth)
        {
            z = 0.0 + 0.0 * I;
    
            /* Perform mandelbrot function */
            for (n = 0; n < maxIterations && cabs(z) < ESCAPE_RADIUS; ++n)
                z = cpow(z, 2.0) + c;

            mapColour(pixel, n, z, bitOffset, maxIterations, colour);

            if (colour->depth != BIT_DEPTH_1)
            {
                pixel = (char *) pixel + arrayMemberSize;
            }
            else if (++bitOffset == CHAR_BIT)
            {
                pixel = (char *) pixel + arrayMemberSize;
                bitOffset = 0;
            }
        }

        /* Reset real value to start of row */
        c -= columns * pixelWidth;
    }

    logMessage(INFO, "Thread %u: Plot generated - exiting", thread->threadID);
    
    pthread_exit(0);
}