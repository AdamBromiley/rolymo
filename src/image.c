#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <unistd.h>

#include "libgroot/include/log.h"
#include "percy/include/parser.h"

#include "image.h"

#include "array.h"
#include "connection_handler.h"
#include "ext_precision.h"
#include "function.h"
#include "parameters.h"
#include "program_ctx.h"
#include "request_handler.h"


#define IMAGE_HEADER_LEN_MAX 128


/* Minimum/maximum memory limit values */
const size_t MEMORY_MIN = 1000;
const size_t MEMORY_MAX = SIZE_MAX;

/* Minimum/maximum allowable thread count */
const unsigned int THREAD_COUNT_MIN = 1;
const unsigned int THREAD_COUNT_MAX = 512;


static void blockToImage(const Block *block);


/* Create image file and write header */
int initialiseImage(PlotCTX *p)
{
    logMessage(DEBUG, "Opening image file \'%s\'", p->plotFilepath);

    p->file = fopen(p->plotFilepath, "wb");

    if (!p->file)
    {
        logMessage(ERROR, "File \'%s\' could not be opened", p->plotFilepath);
        return 1;
    }

    logMessage(DEBUG, "Image file successfully opened");

    if (p->output == OUTPUT_PNM)
    {
        char header[IMAGE_HEADER_LEN_MAX];

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
    }

    return 0;
}


/* Initialise plot array, run function, then write to file */
int imageOutput(PlotCTX *p, ProgramCTX *ctx)
{
    /* Processing threads */
    Thread *threads;

    /* Image block object */
    Block *block;

    /* Pointer to fractal generation function */
    void * (*genFractal)(void *);

    switch (p->precision)
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

    block = createBlock();

    if (!block)
        return 1;

    /* Set values in the Block object and allocate memory for the image array in
     * manageable chunks (the "blocks")
     */
    if (initialiseBlock(block, p, ctx->mem))
    {
        freeBlock(block);
        return 1;
    }

    /* Create a list of processing threads. The most optimised solution is one
     * thread per processing core.
     */
    threads = createThreads(block, ctx->threads);

    if (!threads)
    {
        freeBlock(block);
        return 1;
    }

    /* Because image dimensions can lead to billions of pixels, the plot array
     * may not be able to be stored in one whole memory chunk. Therefore, as per
     * the preceding functions, a block size is determined. A block is a section
     * of N rows of the image array that allow threads will perform on at once.
     * Once all threads have finished, the block gets written to the image file
     * and the cycle continues. The array may not divide evenly into blocks, so
     * the reminader rows are calculated prior and stored in the block context
     * structure
     */
    for (block->id = 0; block->id <= block->bCount; ++(block->id))
    {
        if (block->id == block->bCount)
        {
            /* If there are no remainder rows */
            if (!(block->remainderRows))
                break;

            /* If there are remaining rows */
            block->remainder = true;
        }
        else
        {
            block->remainder = false;
        }

        logMessage(INFO, "Working on block %u (%zu rows)",
                   block->id,
                   (block->remainder) ? block->remainderRows : block->rows);

        /* Create threads to significantly decrease execution time */
        for (unsigned int i = 0; i < threads->tCount; ++i)
        {
            Thread *t = &(threads[i]);

            logMessage(INFO, "Spawning thread %u", t->tid);
    
            if (pthread_create(&(t->pid), NULL, genFractal, t))
            {
                logMessage(ERROR, "Thread could not be created");
                freeBlock(block);
                freeThreads(threads);
                return 1;
            }
        }

        logMessage(INFO, "All threads successfully created");
        
        /* Wait for threads to exit */
        for (unsigned int i = 0; i < threads->tCount; ++i)
        {
            Thread *t = &(threads[i]);

            if (pthread_join(t->pid, NULL))
            {
                logMessage(ERROR, "Thread %u could not be harvested", t->tid);
                freeBlock(block);
                freeThreads(threads);
                return 1;
            }
                
            logMessage(INFO, "Thread %u joined", t->tid);
        }

        logMessage(INFO, "All threads successfully destroyed");

        blockToImage(block);
    }

    logMessage(DEBUG, "Freeing memory");

    freeBlock(block);
    freeThreads(threads);

    return 0;
}


/* Initialise plot array, run function, then write to file */
int imageOutputMaster(PlotCTX *p, NetworkCTX *network, ProgramCTX *ctx)
{
    /* Image block object */
    Block *block = createBlock();

    if (!block)
        return 1;

    /* Set values in the Block object and allocate memory for the image array in
     * manageable chunks (the "blocks")
     */
    if (initialiseBlock(block, p, ctx->mem))
    {
        freeBlock(block);
        return 1;
    }

    /* Because image dimensions can lead to billions of pixels, the plot array
     * may not be able to be stored in one whole memory chunk. Therefore, as per
     * the preceding functions, a block size is determined. A block is a section
     * of N rows of the image array that allow threads will perform on at once.
     * Once all threads have finished, the block gets written to the image file
     * and the cycle continues. The array may not divide evenly into blocks, so
     * the reminader rows are calculated prior and stored in the block context
     * structure
     */
    for (block->id = 0; block->id <= block->bCount; ++(block->id))
    {
        if (block->id == block->bCount)
        {
            /* If there are no remainder rows */
            if (!(block->remainderRows))
                break;

            /* If there are remaining rows */
            block->remainder = true;
        }
        else
        {
            block->remainder = false;
        }

        logMessage(INFO, "Working on block %u (%zu rows)",
                   block->id,
                   (block->remainder) ? block->remainderRows : block->rows);

        if (listener(network, block))
        {
            freeBlock(block);
            return 1;
        }

        blockToImage(block);
    }

    freeBlock(block);

    logMessage(INFO, "Closing connections with workers");

    for (int i = 0; i < network->n; ++i)
    {
        int s = network->workers[i].s;

        if (s < 0)
            continue;

        close(s);
        network->workers[i].s = -1;
        freeClientReceiveBuffer(&(network->workers[i]));
    }

    return 0;
}


/* Initialise plot array, run function, then write to file */
int imageRowOutput(PlotCTX *p, NetworkCTX *network, ProgramCTX *ctx)
{
    /* Processing threads */
    Thread *threads;

    /* Image block object */
    Block *block;

    /* Pointer to fractal row generation function */
    void * (*genFractalRow)(void *);

    switch (p->precision)
    {
        case STD_PRECISION:
            genFractalRow = generateFractalRow;
            break;
        case EXT_PRECISION:
            genFractalRow = generateFractalRowExt;
            break;
        
        #ifdef MP_PREC
        case MUL_PRECISION:
            genFractalRow = generateFractalRowMP;
            break;
        #endif
        
        default:
            return 1;
    }

    block = createBlock();

    if (!block)
        return 1;

    /* Set values in the Block object and allocate memory for the image array as
     * a single row of the image
     */
    if (initialiseBlockAsRow(block, p))
    {
        freeBlock(block);
        return 1;
    }

    /* Create a list of processing threads. The most optimised solution is one
     * thread per processing core.
     */
    threads = createThreads(block, ctx->threads);

    if (!threads)
    {
        freeBlock(block);
        return 1;
    }

    while (1)
    {
        int ret = requestRowNumber(&(block->id), network->s, p);

        if (ret == -3)
        {
            /* Parsing error */
            continue;
        }
        else if (ret == -2)
        {
            /* Safe shutdown */
            break;
        }
        else if (ret)
        {
            close(network->s);
            freeBlock(block);
            freeThreads(threads);
            return 1;  
        }

        logMessage(INFO, "Working on row %zu", block->id);

        /* Create threads to significantly decrease execution time */
        for (unsigned int i = 0; i < threads->tCount; ++i)
        {
            Thread *t = &(threads[i]);
            logMessage(DEBUG, "Spawning thread %u", t->tid);
    
            if (pthread_create(&(t->pid), NULL, genFractalRow, t))
            {
                logMessage(ERROR, "Thread could not be created");
                close(network->s);
                freeBlock(block);
                freeThreads(threads);
                return 1;
            }
        }

        logMessage(DEBUG, "All threads successfully created");
        
        /* Wait for threads to exit */
        for (unsigned int i = 0; i < threads->tCount; ++i)
        {
            Thread *t = &(threads[i]);

            if (pthread_join(t->pid, NULL))
            {
                logMessage(ERROR, "Thread %u could not be harvested", t->tid);
                close(network->s);
                freeBlock(block);
                freeThreads(threads);
                return 1;
            }
                
            logMessage(DEBUG, "Thread %u joined", t->tid);
        }

        logMessage(DEBUG, "All threads successfully destroyed");

        ret = sendRowData(network->s, block->array, block->rowSize);

        if (ret == -2)
        {
            /* Safe shutdown */
            break;
        }
        else if (ret)
        {
            close(network->s);
            freeBlock(block);
            freeThreads(threads);
            return 1;
        }
    }

    logMessage(DEBUG, "Freeing memory");

    close(network->s);
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
    FILE *f = block->parameters->file;
    size_t n = (block->remainder) ? block->remainderBlockSize : block->blockSize;

    logMessage(INFO, "Writing %zu bytes to image file", n);

    if (block->parameters->colour.depth != BIT_DEPTH_ASCII)
    {
        fwrite(block->array, sizeof(char), n, f);
    }
    else
    {
        size_t rows = (block->remainder) ? block->remainderRows : block->rows;
        char *array = block->array;

        for (size_t i = 0; i < rows; ++i, array += block->rowSize)
        {
            const char ASCII_EOL = '\n';

            fwrite(array, sizeof(char), block->rowSize, f);
            fputc(ASCII_EOL, f);
        }
    }

    logMessage(INFO, "Block successfully wrote to file");
}