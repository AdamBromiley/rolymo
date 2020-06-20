#include "array.h"

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include "log.h"
#include "parameters.h"


/* Create array metadata structure */
struct ArrayCTX * createArrayCTX(struct PlotCTX *parameters)
{
    struct ArrayCTX *ctx;

    ctx = malloc(sizeof(*ctx));

    if (!ctx)
        return NULL;

    ctx->array = NULL;

    /* Most optimised solution is using one thread per processing core */
    ctx->threadCount = (unsigned int) sysconf(_SC_NPROCESSORS_ONLN);

    if (ctx->threadCount < 1)
        return NULL;

    ctx->parameters = parameters;

    return ctx;
}


/* To prevent memory overcommitment, the array must be divided into blocks */
struct Block * mallocArray(struct ArrayCTX *array)
{
    /* Percentage of free physical memory that can be allocated by the program */
    const unsigned int FREE_MEMORY_ALLOCATION = 80;
    /* Number of malloc() attempts before failure */
    const unsigned int MALLOC_ESCAPE_ITERATIONS = 16;

    struct BlockCTX *ctx;
    void **arrayPtr = &(array->array);
    struct Block *block;
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

    if (availablePages < 0 || pageSize < 0)
    {
        logMessage(ERROR, "Could not get amount of free memory");
        return NULL;
    }

    freeMemory = (size_t) pageSize * (size_t) availablePages;

    logMessage(DEBUG, "%zu bytes are free. Memory allocation will be limited to %zu bytes",
        freeMemory, freeMemory * FREE_MEMORY_ALLOCATION / 100);
    logMessage(DEBUG, "Creating image array");

    height = array->parameters->height;
    width = array->parameters->width;
    rowSize = width * array->parameters->colour.depth / 8;

    logMessage(DEBUG, "Image array is %zu bytes", height * rowSize);

    /* Try to malloc the array, with each iteration decreasing the array size */
    ctx->blockCount = 0;
    do
    {
        if (ctx->blockCount != 0)
            logMessage(DEBUG, "Memory allocation attempt failed. Retrying...");

        ++(ctx->blockCount);

        ctx->rows = height / ctx->blockCount;
        ctx->remainderRows = height % ctx->blockCount;
        blockSize = ctx->rows * rowSize;

        logMessage(DEBUG, "Splitting array into %u blocks (%zu bytes each)", ctx->blockCount, blockSize);

        if (blockSize > freeMemory * FREE_MEMORY_ALLOCATION / 100)
            continue;

        *arrayPtr = malloc(blockSize);
    }
    while (*arrayPtr == NULL && ctx->blockCount < MALLOC_ESCAPE_ITERATIONS);

    if (!(*arrayPtr))
    {
        logMessage(ERROR, "Memory allocation failed");
        /* If too many malloc() calls have failed */
        free(ctx);
        return NULL;
    }

    ctx->array = array;

    logMessage(DEBUG, "Image array split into %u blocks (%zu bytes - block: %zu rows, remainder block: %zu rows)",
        blockSize, ctx->blockCount, ctx->rows, ctx->remainderRows);

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

    block->blockID = 0;
    block->rows = ctx->rows;
    block->ctx = ctx;

    logMessage(DEBUG, "Block structure created");

    return block;
}


/* Generate a list of threads */
struct Thread * createThreads(const struct ArrayCTX *ctx, struct Block *block)
{
    unsigned int i;

    struct Thread *threads;

    logMessage(DEBUG, "Creating thread array");

    if (!ctx)
    {
        logMessage(DEBUG, "Array context structure not found");
        return NULL;
    }

    threads = malloc(ctx->threadCount * sizeof(*threads));
    
    if (!threads)
    {
        logMessage(ERROR, "Memory allocation failed");
        return NULL;
    }
    
    for (i = 0; i < ctx->threadCount; ++i)
    {
        /* Consecutive IDs allow for threads to work on different array rows */
        threads[i].threadID = i;
        threads[i].block = block;
    }

    logMessage(DEBUG, "Thread array generated");

    return threads;
}


/* Free ArrayCTX struct */
void freeArrayCTX(struct ArrayCTX *ctx)
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
void freeBlock(struct Block *block)
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


/* Free thread list */
void freeThreads(struct Thread *threads)
{
    if (threads)
    {
        free(threads);
        logMessage(DEBUG, "Thread array freed");
    }

    return;
}