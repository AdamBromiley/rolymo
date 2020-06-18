#include "function.h"

#include <complex.h>
#include <pthread.h>
#include <stddef.h>

#include "array.h"
#include "parameters.h"


void * mandelbrot(void *threadInfo)
{
    struct Thread *thread = threadInfo;

    struct PlotCTX *parameters = thread->block->ctx->array->parameters;

    unsigned long int **array = thread->block->ctx->array->array;
    size_t x, y;
    size_t rows = thread->block->rows;
    size_t columns = parameters->width;

    unsigned int blockCount = thread->block->ctx->blockCount;

    double pixelWidth = (parameters->maximum.re - parameters->minimum.re) / parameters->width;
    double pixelHeight = (parameters->maximum.im - parameters->minimum.im) / parameters->height;

    complex z, c;
    unsigned long int n;

    /* Set to top left of plot block (minimum real, maximum block imaginary) */
    c = parameters->minimum.re +
        (parameters->maximum.im - (thread->block->blockID * rows) * pixelHeight) * I;
    
    /* Offset by threadID to ensure each thread gets a unique row */
    for (y = 0 + thread->threadID; y < rows; y += blockCount)
    {
        /* Centers the plot vertically - noticeable with the terminal output */
        /*** NOT IMPLEMENTED YET ***
        if (parameters->output == OUTPUT_TERMINAL && y == 0)
            continue;
        *** NOT IMPLEMENTED YET ***/

        /* Iterate over the row */
        for (x = 0; x < columns; ++x, c += pixelWidth)
        {
            z = 0.0 + 0.0 * I;

            /* Perform mandelbrot function */
            for (n = 0; n < parameters->iterations && cabs(z) < ESCAPE_RADIUS; ++n)
                z = cpow(z, 2.0) + c;
            
            /* Set iteration count in array */
            array[y][x] = n;
        }

        /* Reset real value to start of row (left of plot) */
        c -= (columns - 1) * pixelWidth;

        /* Set imaginary value to that of the next row */
        c -= y * pixelHeight * I;
    }
    
    pthread_exit(0);
}