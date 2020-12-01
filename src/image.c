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
    /* Pointer to fractal generation function */
    void * (*genFractal)(void *);

    /* Array blocks */
    ArrayCTX *array;
    Block *block;

    /* Processing threads */
    Thread *threads;
    Thread *thread;

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

    /* Create array metadata struct */
    array = createArrayCTX(p);

    if (!array)
        return 1;

    /* Allocate memory to the array in manageable blocks */
    block = mallocArray(array, ctx->mem);

    if (!block)
    {
        freeArrayCTX(array);
        return 1;
    }

    /* Create a list of processing threads. The most optimised solution is one
     * thread per processing core.
     */
    threads = createThreads(block, ctx->threads);

    if (!threads)
    {
        freeArrayCTX(array);
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

        blockToImage(block);
    }

    logMessage(DEBUG, "Freeing memory");

    freeArrayCTX(array);
    freeBlock(block);
    freeThreads(threads);

    return 0;
}


/* Initialise plot array, run function, then write to file */
int imageOutputMaster(PlotCTX *p, NetworkCTX *network, ProgramCTX *ctx)
{
    /* Array blocks */
    ArrayCTX *array;
    Block *block;

    /* Create array metadata struct */
    array = createArrayCTX(p);

    if (!array)
        return 1;

    /* Allocate memory to the array in manageable blocks */
    block = mallocArray(array, ctx->mem);

    if (!block)
    {
        freeArrayCTX(array);
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

        if (listener(network, block))
        {
            freeArrayCTX(array);
            freeBlock(block);
            return 1;
        }

        blockToImage(block);
    }
    
    
    logMessage(INFO, "Closing connections with slaves");

    for (int i = 0; i < network->n; ++i)
    {
        int s = network->slaves[i];

        if (s < 0)
            continue;

        close(s);
        network->slaves[i] = -1;
    }

    logMessage(DEBUG, "Freeing memory");

    freeArrayCTX(array);
    freeBlock(block);

    return 0;
}


/* Initialise plot array, run function, then write to file */
int imageRowOutput(PlotCTX *p, NetworkCTX *network, ProgramCTX *ctx)
{
    /* Pointer to fractal row generation function */
    void * (*genFractalRow)(void *);

    /* Array blocks */
    ArrayCTX *array;

    RowCTX row;

    /* Processing threads */
    SlaveThread *threads;
    SlaveThread *thread;

    size_t rowSize;

    char *writeBuffer;

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

    /* Create array metadata struct */
    array = createArrayCTX(p);

    if (!array)
        return 1;

    rowSize = (p->colour.depth == BIT_DEPTH_ASCII)
              ? p->width * sizeof(char)
              : p->width * p->colour.depth / 8.0;

    array->array = malloc(rowSize);

    if (!array->array)
    {
        freeArrayCTX(array);
        return 1;
    }

    row.array = array;

    /* Create a list of processing threads. The most optimised solution is one
     * thread per processing core.
     */
    threads = createSlaveThreads(&row, ctx->threads);

    if (!threads)
    {
        freeArrayCTX(array);
        return 1;
    }

    /* For row array plus row number at beginning */
    writeBuffer = malloc(rowSize + RESPONSE_IMAGE_DATA_OFFSET);

    if (!writeBuffer)
    {
        freeArrayCTX(array);
        freeSlaveThreads(threads);
        return 1;
    }

    while (1)
    {
        char readBuffer[10];
        char *endptr;

        uintmax_t tempUIntMax = 0;

        ssize_t ret = writeSocket(">", network->s, sizeof(">"));

        if (ret == 0)
        {
            break;
        }
        else if (ret < 0 || ret != 2)
        {
            logMessage(ERROR, "Could not write to socket connection");
            close(network->s);
            free(writeBuffer);
            freeArrayCTX(array);
            freeSlaveThreads(threads);
            return 1;
        }

        memset(readBuffer, '\0', sizeof(readBuffer));
        ret = readSocket(readBuffer, network->s, sizeof(readBuffer));

        if (ret == 0)
        {
            break;
        }
        else if (ret < 0)
        {
            logMessage(ERROR, "Error reading from socket connection");
            free(writeBuffer);
            freeArrayCTX(array);
            freeSlaveThreads(threads);
            return 1;
        }

        readBuffer[sizeof(readBuffer)] = '\0';

        if (stringToUIntMax(&tempUIntMax, readBuffer, 0, p->height - 1, &endptr, BASE_DEC) != PARSE_SUCCESS)
        {
            logMessage(ERROR, "Error reading from socket connection");
            free(writeBuffer);
            freeArrayCTX(array);
            freeSlaveThreads(threads);
            return 1;
        }

        row.row = (size_t) tempUIntMax;

        logMessage(INFO, "Working on row %zu", row.row);

        /* Create threads to significantly decrease execution time */
        for (unsigned int i = 0; i < threads->ctx->count; ++i)
        {
            thread = &(threads[i]);
            logMessage(DEBUG, "Spawning thread %u", thread->tid);
    
            if (pthread_create(&(thread->pid), NULL, genFractalRow, thread))
            {
                logMessage(ERROR, "Thread could not be created");
                close(network->s);
                free(writeBuffer);
                freeArrayCTX(array);
                freeSlaveThreads(threads);
                return 1;
            }
        }

        logMessage(DEBUG, "All threads successfully created");
        
        /* Wait for threads to exit */
        for (unsigned int i = 0; i < threads->ctx->count; ++i)
        {
            thread = &(threads[i]);

            if (pthread_join(thread->pid, NULL))
            {
                logMessage(ERROR, "Thread %u could not be harvested", thread->tid);
                close(network->s);
                free(writeBuffer);
                freeArrayCTX(array);
                freeSlaveThreads(threads);
                return 1;
            }
                
            logMessage(DEBUG, "Thread %u joined", thread->tid);
        }

        logMessage(DEBUG, "All threads successfully destroyed");

        memset(writeBuffer, '\0', rowSize + RESPONSE_IMAGE_DATA_OFFSET);
        sprintf(writeBuffer, "%zu", row.row);
        memcpy(writeBuffer + RESPONSE_IMAGE_DATA_OFFSET, array->array, rowSize);

        ret = writeSocket(writeBuffer, network->s, rowSize + RESPONSE_IMAGE_DATA_OFFSET);

        if (ret == 0)
        {
            break;
        }
        else if (ret < 0 || (size_t) ret != rowSize + RESPONSE_IMAGE_DATA_OFFSET)
        {
            logMessage(ERROR, "Could not write to socket connection");
            close(network->s);
            free(writeBuffer);
            freeArrayCTX(array);
            freeSlaveThreads(threads);
            return 1;
        }

        memset(readBuffer, '\0', sizeof(readBuffer));
        ret = readSocket(readBuffer, network->s, sizeof(readBuffer));
        readBuffer[sizeof(readBuffer) - 1] = '\0';

        if (ret == 0)
        {
            break;
        }
        else if (ret < 0 || strcmp(">", readBuffer))
        {
            logMessage(ERROR, "Error reading from socket connection");
            close(network->s);
            free(writeBuffer);
            freeArrayCTX(array);
            freeSlaveThreads(threads);
            return 1;
        }
    }

    close(network->s);

    logMessage(DEBUG, "Freeing memory");
    free(writeBuffer);
    freeArrayCTX(array);
    freeSlaveThreads(threads);

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
    PlotCTX *p = block->ctx->array->params;

    void *array = block->ctx->array->array;

    size_t arrayLength = block->rows * p->width;
    double pixelSize = (p->colour.depth == BIT_DEPTH_ASCII)
                       ? sizeof(char)
                       : p->colour.depth / 8.0;
    size_t arraySize = arrayLength * pixelSize;

    FILE *image = p->file;

    logMessage(INFO, "Writing %zu pixels (%zu bytes; pixel size = %d bits) to image file",
                     arrayLength, arraySize, p->colour.depth);

    if (p->colour.depth != BIT_DEPTH_ASCII)
    {
        fwrite(array, sizeof(char), arraySize, image);
    }
    else
    {
        for (size_t i = 0; i < block->rows; ++i)
        {
            const char *LINE_TERMINATOR = "\n";

            fwrite(array, sizeof(char), p->width, image);
            fwrite(LINE_TERMINATOR, sizeof(*LINE_TERMINATOR), strlen(LINE_TERMINATOR), image);

            array = (char *) array + p->width;
        }
    }

    logMessage(INFO, "Block successfully wrote to file");
}