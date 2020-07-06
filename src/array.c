#include "array.h"

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include "libgroot/include/log.h"

#include "parameters.h"


/* Create array metadata structure */
ArrayCTX * createArrayCTX(PlotCTX *parameters)
{
    ArrayCTX *ctx;

    ctx = malloc(sizeof(*ctx));

    if (!ctx)
        return NULL;

    ctx->array = NULL;
    ctx->parameters = parameters;

    return ctx;
}


/* To prevent memory overcommitment, the array must be divided into blocks */
Block * mallocArray(ArrayCTX *array, size_t bytes)
{
    /* Percentage of free physical memory that can be allocated by the program */
    const unsigned int FREE_MEMORY_ALLOCATION = 80;
    /* Maximum number of blocks the array should be divided into */
    const unsigned int BLOCK_COUNT_MAX = 64;

    Block *block;
    BlockCTX *ctx;

    void **arrayPtr = &(array->array);
    size_t height, width, rowSize, blockSize;

    size_t freeMemory;
    long availablePages, pageSize;

    logMessage(DEBUG, "Generating image array context");

    if (!(array->parameters))
    {
        logMessage(ERROR, "Pointer to plotting parameters structure not found");
        return NULL;
    }

    ctx = malloc(sizeof(*ctx));

    if (!ctx)
    {
        logMessage(DEBUG, "Memory allocation failed");
        return NULL;
    }

    logMessage(DEBUG, "Context generated");
    logMessage(DEBUG, "Getting amount of free memory");

    availablePages = sysconf(_SC_AVPHYS_PAGES);
    pageSize = sysconf(_SC_PAGE_SIZE);

    if (availablePages < 1 || pageSize < 1)
    {
        logMessage(ERROR, "Could not get amount of free memory");
        return NULL;
    }

    freeMemory = (size_t) pageSize * (size_t) availablePages;

    logMessage(DEBUG, "%zu bytes of physical memory are free", freeMemory);

    /* If caller has specified max memory usage */
    if (bytes > 0)
    {
        if (bytes > freeMemory)
        {
            logMessage(WARNING, "Memory maximum of %zu bytes is greater than the amount of free physical memory"
                " (%zu bytes). It is recommended to only allow allocation of physical memory for efficiency",
                bytes, freeMemory);
        }

        freeMemory = bytes;
        logMessage(DEBUG, "Memory allocation will be limited to %zu bytes", freeMemory);
    }
    else
    {
        logMessage(DEBUG, "Memory allocation will be limited to %u%% (%zu bytes)",
            FREE_MEMORY_ALLOCATION, freeMemory * FREE_MEMORY_ALLOCATION / 100);
    }

    logMessage(DEBUG, "Creating image array");

    height = array->parameters->height;
    width = array->parameters->width;
    rowSize = width * array->parameters->colour.depth / 8;

    logMessage(DEBUG, "Image array is %zu bytes", height * rowSize);

    /* Try to malloc the array, with each iteration decreasing the array size */
    *arrayPtr = NULL;

    for (ctx->count = 1; ctx->count <= BLOCK_COUNT_MAX; ctx->count *= 2)
    {
        if (ctx->count > 1)
            logMessage(DEBUG, "Memory allocation attempt failed. Retrying...");    

        ctx->rows = height / ctx->count;
        ctx->remainder = height % ctx->count;
        blockSize = ctx->rows * rowSize;

        logMessage(DEBUG, "Splitting array into %u blocks (%zu bytes each)", ctx->count, blockSize);

        if (blockSize > freeMemory * FREE_MEMORY_ALLOCATION / 100)
            continue;

        *arrayPtr = malloc(blockSize);

        if (*arrayPtr)
            break;
    }

    if (!(*arrayPtr) || blockSize == 0)
    {
        /* If too many malloc() calls have failed */
        logMessage(ERROR, "Memory allocation failed");
        free(ctx);
        return NULL;
    }

    ctx->array = array;

    logMessage(DEBUG, "Image array split into %u blocks (%zu bytes - block: %zu rows, remainder block: %zu rows)",
        blockSize, ctx->count, ctx->rows, ctx->remainder);
    logMessage(DEBUG, "Creating image block structure");

    /* Create block structure */
    block = malloc(sizeof(*block));

    if (!block)
    {
        logMessage(ERROR, "Memory allocation failed");
        free(*arrayPtr);
        free(ctx);
        return NULL;
    }

    block->id = 0;
    block->rows = ctx->rows;
    block->ctx = ctx;

    logMessage(DEBUG, "Block structure created");

    return block;
}


/* Generate a list of threads */
Thread * createThreads(Block *block, unsigned int n)
{
    Thread *threads;
    ThreadCTX *ctx;

    logMessage(DEBUG, "Creating thread array");

    ctx = malloc(sizeof(*ctx));

    if (!ctx)
    {
        logMessage(ERROR, "Memory allocation failed");
        return NULL;
    }

    if (n < 1)
    {
        long result = sysconf(_SC_NPROCESSORS_ONLN);

        if (result < 1)
        {
            result = 1;
            logMessage(WARNING, "Could not get number of online processors - limiting to %ld thread(s)",
                result);
        }
        else if (result > UINT_MAX)
        {
            result = UINT_MAX;
        }

        n = (unsigned int) result;
    }
    
    /* Set thread count */
    ctx->count = n;

    threads = malloc(n * sizeof(*threads));
    
    if (!threads)
    {
        logMessage(ERROR, "Memory allocation failed");
        return NULL;
    }
    
    for (unsigned int i = 0; i < n; ++i)
    {
        /* Consecutive IDs allow threads to work on different array rows */
        threads[i].tid = i;
        threads[i].block = block;
        threads[i].ctx = ctx;
    }

    logMessage(DEBUG, "Thread array generated");

    return threads;
}


/* Free ArrayCTX struct */
void freeArrayCTX(ArrayCTX *ctx)
{
    if (ctx)
    {
        if (ctx->array)
        {
            free(ctx->array);
            logMessage(DEBUG, "Image array freed");
        }
        
        free(ctx);
        logMessage(DEBUG, "Array context freed");
    }

    return;
}


/* Free Block and nested BlockCTX structs */
void freeBlock(Block *block)
{
    if (block)
    {
        if (block->ctx)
        {
            free(block->ctx);
            logMessage(DEBUG, "Block context freed");
        }

        free(block);
        logMessage(DEBUG, "Block structure freed");
    }

    return;
}


/* Free thread list and nested ThreadCTX struct */
void freeThreads(Thread *threads)
{
    if (threads)
    {
        if (threads->ctx)
        {
            free(threads->ctx);
            logMessage(DEBUG, "Thread context freed");
        }

        free(threads);
        logMessage(DEBUG, "Thread array freed");
    }

    return;
}