#include "image.h"

#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include "libgroot/include/log.h"

#include "array.h"
#include "ext_precision.h"
#include "function.h"
#include "parameters.h"


#define IMAGE_HEADER_LEN_MAX 128


/* Minimum/maximum memory limit values */
const size_t MEMORY_MIN = 1000;
const size_t MEMORY_MAX = SIZE_MAX;

/* Minimum/maximum allowable thread count */
const unsigned int THREAD_COUNT_MIN = 1;
const unsigned int THREAD_COUNT_MAX = 512;


static void blockToImage(const Block *block);


/* Create image file and write header */
int initialiseImage(PlotCTX *p, const char *filepath)
{
    char header[IMAGE_HEADER_LEN_MAX];

    logMessage(DEBUG, "Opening image file \'%s\'", filepath);

    p->file = fopen(filepath, "wb");

    if (!p->file)
    {
        logMessage(ERROR, "File \'%s\' could not be opened", filepath);
        return 1;
    }

    logMessage(DEBUG, "Image file successfully opened");
    logMessage(DEBUG, "Writing header to image");

    /* Write PNM file header */
    switch (p->colour.depth)
    {
        case BIT_DEPTH_1:
            /* PBM file */
            snprintf(header, sizeof(header), "P4 %zu %zu ", p->width, p->height);
            break;
        case BIT_DEPTH_8:
            /* PGM file */
            snprintf(header, sizeof(header), "P5 %zu %zu 255 ", p->width, p->height);
            break;
        case BIT_DEPTH_24:
            /* PPM file */
            snprintf(header, sizeof(header), "P6 %zu %zu 255 ", p->width, p->height);
            break;
        default:
            logMessage(ERROR, "Could not determine bit depth");
            return 1;
    }

    fprintf(p->file, "%s", header);

    logMessage(DEBUG, "Header \'%s\' successfully wrote to image", header);

    return 0;
}


/* Initialise plot array, run function, then write to file */
int imageOutput(PlotCTX *p, size_t mem, unsigned int threadCount)
{
    /* Pointer to fractal generation function */
    void * (*genFractal)(void *);

    switch (precision)
    {
        case STD_PRECISION:
            genFractal = generateFractal;
            break;
        case EXT_PRECISION:
            genFractal = generateFractalExt;
            break;
        
        #ifdef MP_PREC
        case MUL_PRECISION:
            genFractal = generateFractalMP;
            break;
        #endif
        
        default:
            return 1;
    }

    /* Array blocks */
    ArrayCTX *array;
    Block *block;

    /* Processing threads */
    Thread *threads;
    Thread *thread;

    /* Create array metadata struct */
    array = createArrayCTX(p);

    if (!array)
        return 1;

    /* Allocate memory to the array in manageable blocks */
    block = mallocArray(array, mem);

    if (!block)
    {
        freeArrayCTX(array);
        return 1;
    }

    /* 
     * Create a list of processing threads.
     * The most optimised solution is one thread per processing core.
     */
    threads = createThreads(block, threadCount);

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
        for (unsigned int i = 0; i < threads->ctx->count; ++i)
        {
            thread = &(threads[i]);
            logMessage(INFO, "Spawning thread %u", thread->tid);
    
            if (pthread_create(&(thread->pid), NULL, genFractal, thread))
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
        for (unsigned int i = 0; i < threads->ctx->count; ++i)
        {
            thread = &(threads[i]);

            if (pthread_join(thread->pid, NULL))
            {
                logMessage(ERROR, "Thread %u could not be harvested", thread->tid);
                freeArrayCTX(array);
                freeBlock(block);
                freeThreads(threads);
                return 1;
            }
                
            logMessage(INFO, "Thread %u joined", thread->tid);
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
int closeImage(PlotCTX *p)
{
    logMessage(DEBUG, "Closing image file");

    if (fclose(p->file))
    {
        logMessage(WARNING, "Image file could not be closed");
        p->file = NULL;
        return 1;
    }

    p->file = NULL;

    logMessage(DEBUG, "Image file closed");

    return 0;
}


/* Write block to image file */
static void blockToImage(const Block *block)
{
    void *array = block->ctx->array->array;

    size_t arrayLength = block->rows * block->ctx->array->params->width;
    double pixelSize = (block->ctx->array->params->colour.depth == BIT_DEPTH_ASCII)
                       ? sizeof(char)
                       : block->ctx->array->params->colour.depth / 8.0;
    size_t arraySize = arrayLength * pixelSize;

    FILE *image = block->ctx->array->params->file;

    logMessage(INFO, "Writing %zu pixels (%zu bytes; pixel size = %d bits) to image file",
        arrayLength, arraySize, block->ctx->array->params->colour.depth);

    if (block->ctx->array->params->colour.depth != BIT_DEPTH_ASCII)
        fwrite(array, sizeof(char), arraySize, image);
    else
    {
        for (size_t i = 0; i < block->rows; ++i)
        {
            fwrite(array, sizeof(char), block->ctx->array->params->width, image);
            putchar('\n');
            array = (char *) array + block->ctx->array->params->width;
        }
    }

    logMessage(INFO, "Block successfully wrote to file");

    return;
}