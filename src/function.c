#include "function.h"

#include <complex.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>

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

    c = parameters->minimum.re +
        (parameters->maximum.im - (thread->block->blockID * rows) * pixelHeight) * I;
    
    /* Offset by threadID to ensure each thread gets a unique row */
    for (y = 0 + thread->threadID; y < rows; y += blockCount)
    {
        /* Centers the plot vertically - noticeable with the terminal output */
        /*if (parameters->output == OUTPUT_TERMINAL && y == 0)
            continue;*/

        c -= (columns - 1) * pixelWidth;
        c -= y * pixelHeight * I;
        
        for (x = 0; x < columns; ++x, c += pixelWidth)
        {
            z = 0.0 + 0.0 * I;
            for (n = 0; n < parameters->iterations && cabs(z) < ESCAPE_RADIUS; ++n)
                z = cpow(z, 2.0) + c;
            
            array[y][x] = n;
        }
    }
    
    /*
    if (parameters->output == OUTPUT_TERMINAL)
        return NULL;
    */
    
    pthread_exit(0);
}