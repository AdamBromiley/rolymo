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

    /* Write NetPBM file header */
    switch (parameters->colour.depth)
    {
        case BIT_DEPTH_1:
            /* PBM file */
            fprintf(parameters->file, "P4 %d %d ", parameters->width, parameters->height);
        case BIT_DEPTH_8:
            /* PGM file */
            fprintf(parameters->file, "P5 %d %d 255 ", parameters->width, parameters->height);
        case BIT_DEPTH_24:
            /* PPM file */
            fprintf(parameters->file, "P6 %d %d 255 ", parameters->width, parameters->height);
        default:
            return 1;
    }

    return 0;
}


int imageOutput(const struct PlotCTX *parameters)
{
    unsigned int i;

    /* Array blocks */
    struct ArrayCTX *array;
    struct Block *block;

    /* Processing threads */
    struct Thread *threads;
    struct Thread *thread;

    array = createArrayCTX(parameters);

    if (!array)
        return 1;

    block = mallocArray(array);

    if (!block)
    {
        freeArrayCTX(array);
        return 1;
    }

    threads = createThreads(array, block);

    if (!threads)
    {
        freeArrayCTX(array);
        freeBlock(block);
        return 1;
    }

    block->rows = block->ctx->rows;

    for (block->blockID = 0; block->blockID <= block->ctx->blockCount; ++(block->blockID))
    {
        if (block->blockID == block->ctx->blockCount)
        {
            if (!(block->ctx->remainderRows))
                break;

            /* If there's remaining rows */
            block->rows = block->ctx->remainderRows;
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
    size_t width = block->ctx->array->parameters->width;
    size_t height = block->rows;

    unsigned long int **array = block->ctx->array->array;

    unsigned long int iterations;
    unsigned long int maxIterations = block->ctx->array->parameters->iterations;

    unsigned char bitOffset = 0;
    struct ColourScheme colour = block->ctx->array->parameters->colour;

    FILE *image = block->ctx->array->parameters->file;

    enum EscapeStatus status;
    struct ColourRGB rgb;

    for (y = 0; y < height; ++y)
    {
        for (x = 0; x < width; ++x)
        {
            iterations = array[y][x];
            status = (iterations < maxIterations) ? ESCAPED : UNESCAPED;

            switch (colour.depth)
            {
                case BIT_DEPTH_1:
                    ++bitOffset;
                    if (bitOffset == CHAR_BIT)
                    {
                        fwrite(&(rgb.r), sizeof(rgb.r), 1, image);
                        bitOffset = 0;
                    }
                    mapColour(&rgb, &colour, bitOffset, status);
                case BIT_DEPTH_8:
                    mapColour(&rgb, &colour, iterations, status);
                    fwrite(&(rgb.r), sizeof(rgb.r), 1, image);
                case BIT_DEPTH_24:
                    mapColour(&rgb, &colour, iterations, status);
                    fwrite(&rgb, sizeof(rgb), 1, image);
                default:
                    return 1;
            }
        }
    }

    return 0;
}