#include "image.h"

#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include "groot/include/log.h"

#include "array.h"
#include "colour.h"
#include "function.h"
#include "parameters.h"


#define IMAGE_HEADER_LENGTH_MAX 128


static void blockToImage(const struct Block *block);


/* Create image file and write header */
int initialiseImage(struct PlotCTX *parameters, const char *filepath)
{
    char header[IMAGE_HEADER_LENGTH_MAX];

    logMessage(DEBUG, "Opening image file \'%s\'", filepath);

    parameters->file = fopen(filepath, "wb");

    if (!parameters->file)
    {
        logMessage(ERROR, "File \'%s\' could not be opened", filepath);
        return 1;
    }

    logMessage(DEBUG, "Image file successfully opened");
    logMessage(DEBUG, "Writing header to image");

    /* Write PNM file header */
    switch (parameters->colour.depth)
    {
        case BIT_DEPTH_1:
            /* PBM file */
            snprintf(header, sizeof(header), "P4 %zu %zu ", parameters->width, parameters->height);
            break;
        case BIT_DEPTH_8:
            /* PGM file */
            snprintf(header, sizeof(header), "P5 %zu %zu 255 ", parameters->width, parameters->height);
            break;
        case BIT_DEPTH_24:
            /* PPM file */
            snprintf(header, sizeof(header), "P6 %zu %zu 255 ", parameters->width, parameters->height);
            break;
        default:
            logMessage(ERROR, "Could not determine bit depth");
            return 1;
    }

    fprintf(parameters->file, "%s", header);

    logMessage(DEBUG, "Header \'%s\' successfully wrote to image", header);

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

    /* 
     * Create a list of processing threads.
     * The most optimised solution is one thread per processing core.
     */
    threads = createThreads((unsigned int) sysconf(_SC_NPROCESSORS_ONLN), block);

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
    for (block->id = 0; block->id <= block->ctx->count; ++(block->id))
    {
        if (block->id == block->ctx->count)
        {
            /* If there are no remainder rows */
            if (!(block->ctx->remainder))
                break;

            /* If there's remaining rows */
            block->rows = block->ctx->remainder;
        }
        else
        {
            block->rows = block->ctx->rows;
        }

        logMessage(INFO, "Working on block %u (%zu rows)", block->id, block->rows);

        /* Create threads to significantly decrease execution time */
        for (i = 0; i < threads->ctx->count; ++i)
        {
            thread = &(threads[i]);
            logMessage(INFO, "Spawning thread %u", thread->tid);
    
            if (pthread_create(&(thread->pid), NULL, generateFractal, thread) != 0)
            {
                logMessage(ERROR, "Thread could not be created");
                freeArrayCTX(array);
                freeBlock(block);
                freeThreads(threads);
                return 1;
            }
        }

        logMessage(INFO, "All threads successfully created");
        
        /* Wait for threads to exit */
        for (i = 0; i < threads->ctx->count; ++i)
        {
            thread = &(threads[i]);
            pthread_join(thread->pid, NULL);
            logMessage(INFO, "Thread %u exited", thread->tid);
        }

        logMessage(INFO, "All threads successfully destroyed");

        /* Write block to image file */
        blockToImage(block);
    }

    logMessage(DEBUG, "Freeing memory");

    freeArrayCTX(array);
    freeBlock(block);
    freeThreads(threads);

    return 0;
}


/* Close image file */
int closeImage(struct PlotCTX *parameters)
{
    logMessage(DEBUG, "Closing image file");

    if (fclose(parameters->file))
    {
        logMessage(WARNING, "Image file could not be closed");
        parameters->file = NULL;
        return 1;
    }

    logMessage(DEBUG, "Image file closed");

    parameters->file = NULL;
    return 0;
}


/* Write block to image file */
static void blockToImage(const struct Block *block)
{
    void *array = block->ctx->array->array;

    size_t arrayLength = block->rows * block->ctx->array->parameters->width;
    double pixelSize = block->ctx->array->parameters->colour.depth / 8.0;
    size_t arraySize = arrayLength * pixelSize;

    FILE *image = block->ctx->array->parameters->file;

    logMessage(INFO, "Writing %zu pixels (%zu bytes; pixel size = %d bits) to image file",
        arrayLength, arraySize, block->ctx->array->parameters->colour.depth);

    fwrite(array, sizeof(char), arraySize, image);

    logMessage(INFO, "Block successfully wrote to file");

    return;
}