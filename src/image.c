#include "image.h"

#include <limits.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>

#include "array.h"
#include "colour.h"
#include "function.h"
#include "log.h"
#include "parameters.h"


static int blockToImage(const struct Block *block);


/* Create image file and write header */
int initialiseImage(struct PlotCTX *parameters, const char *filePath)
{
    parameters->file = fopen(filePath, "w");

    if (!parameters->file)
        return 1;

    /* Write PNM file header */
    switch (parameters->colour.depth)
    {
        case BIT_DEPTH_1:
            /* PBM file */
            fprintf(parameters->file, "P4 %zu %zu ", parameters->width, parameters->height);
            break;
        case BIT_DEPTH_8:
            /* PGM file */
            fprintf(parameters->file, "P5 %zu %zu 255 ", parameters->width, parameters->height);
            break;
        case BIT_DEPTH_24:
            /* PPM file */
            fprintf(parameters->file, "P6 %zu %zu 255 ", parameters->width, parameters->height);
            break;
        default:
            return 1;
    }

    return 0;
}


/* Initialise plot array, run function, then write to file */
int imageOutput(struct PlotCTX *parameters)
{
    unsigned int i;

    /* Array blocks */
    struct ArrayCTX *array;
    struct Block *block;

    /* Processing threads */
    struct Thread *threads;
    struct Thread *thread;

    /* Create array metadata struct */
    array = createArrayCTX(parameters);

    if (!array)
        return 1;

    /* Allocate memory to the array in manageable blocks */
    block = mallocArray(array);

    if (!block)
    {
        freeArrayCTX(array);
        return 1;
    }

    /* Create list of processing threads */
    threads = createThreads(array, block);

    if (!threads)
    {
        freeArrayCTX(array);
        freeBlock(block);
        return 1;
    }

    /*
     * Because image dimensions can lead to billions of pixels, the plot array
     * may not be able to be stored in one whole memory chunk. Therefore, as per
     * the preceding functions, a block size is determined. A block is a section
     * of N rows of the image array that allow threads will perform on at once.
     * Once all threads have finished, the block gets written to the image file
     * and the cycle continues. The array may not divide evenly into blocks, so
     * the reminader rows are calculated prior and stored in the block context
     * structure
     */
    for (block->blockID = 0; block->blockID <= block->ctx->blockCount; ++(block->blockID))
    {
        if (block->blockID == block->ctx->blockCount)
        {
            /* If there are no remainder rows */
            if (!(block->ctx->remainderRows))
                break;

            /* If there's remaining rows */
            block->rows = block->ctx->remainderRows;
        }
        else
        {
            block->rows = block->ctx->rows;
        }

        /* Create threads to significantly decrease execution time */
        for (i = 0; i < array->threadCount; ++i)
        {
            thread = &(threads[i]);
            if (pthread_create(&(thread->pthreadID), NULL, mandelbrot, thread) != 0)
            {
                freeArrayCTX(array);
                freeBlock(block);
                freeThreads(threads);
                return 1;
            }
        }
        
        /* Wait for threads to exit */
        for (i = 0; i < array->threadCount; ++i)
        {
            thread = &(threads[i]);
            pthread_join(thread->pthreadID, NULL);
        }

        /* Convert iterations to colour values and write to image file */
        if (blockToImage(block))
        {
            freeArrayCTX(array);
            freeBlock(block);
            freeThreads(threads);
            return 1;
        }
    }

    freeArrayCTX(array);
    freeBlock(block);
    freeThreads(threads);

    return 0;
}


/* Close image file */
int closeImage(struct PlotCTX *parameters)
{
    if (fclose(parameters->file))
    {
        parameters->file = NULL;
        return 1;
    }

    parameters->file = NULL;
    return 0;
}


/* Convert iterations to colour values and write to image file */
static int blockToImage(const struct Block *block)
{
    size_t x, y;

    /* Caching the struct values optimises the array iteration */

    size_t columns = block->ctx->array->parameters->width;
    size_t rows = block->rows;

    unsigned long int *array = block->ctx->array->array;

    unsigned long int iterations;
    unsigned long int maxIterations = block->ctx->array->parameters->iterations;
    enum EscapeStatus status;

    unsigned char bitOffset = 0;
    struct ColourRGB rgb;
    struct ColourScheme colour = block->ctx->array->parameters->colour;

    FILE *image = block->ctx->array->parameters->file;

    for (y = 0; y < rows; ++y)
    {
        for (x = 0; x < columns; ++x)
        {
            iterations = *(array + y + x);
            status = (iterations < maxIterations) ? ESCAPED : UNESCAPED;

            /* Write colour value to the file */
            switch (colour.depth)
            {
                case BIT_DEPTH_1:
                    /* Only write every byte */
                    ++bitOffset;
                    if (bitOffset == CHAR_BIT)
                    {
                        fwrite(&(rgb.r), sizeof(rgb.r), 1, image);
                        bitOffset = 0;
                    }
                    mapColour(&rgb, &colour, bitOffset, status);
                    break;
                case BIT_DEPTH_8:
                    mapColour(&rgb, &colour, iterations, status);
                    fwrite(&(rgb.r), sizeof(rgb.r), 1, image);
                    break;
                case BIT_DEPTH_24:
                    mapColour(&rgb, &colour, iterations, status);
                    fwrite(&rgb, sizeof(rgb), 1, image);
                    break;
                default:
                    return 1;
            }
        }
    }

    return 0;
}