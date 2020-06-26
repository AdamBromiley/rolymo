#include "array.h"

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include "libgroot/include/log.h"

#include "parameters.h"


unsigned int THREAD_COUNT_MIN = 1;
unsigned int THREAD_COUNT_MAX = 128;


/* Create array metadata structure */
struct ArrayCTX * createArrayCTX(struct PlotCTX *parameters)
{
    struct ArrayCTX *ctx;

    ctx = malloc(sizeof(*ctx));

    if (!ctx)
        return NULL;

    ctx->array = NULL;

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
    ctx->count = 0;
    do
    {
        if (ctx->count != 0)
            logMessage(DEBUG, "Memory allocation attempt failed. Retrying...");

        ++(ctx->count);

        ctx->rows = height / ctx->count;
        ctx->remainder = height % ctx->count;
        blockSize = ctx->rows * rowSize;

        logMessage(DEBUG, "Splitting array into %u blocks (%zu bytes each)", ctx->count, blockSize);

        if (blockSize > freeMemory * FREE_MEMORY_ALLOCATION / 100)
            continue;

        *arrayPtr = malloc(blockSize);
    }
    while (*arrayPtr == NULL && ctx->count < MALLOC_ESCAPE_ITERATIONS);

    if (!(*arrayPtr))
    {
        logMessage(ERROR, "Memory allocation failed");
        /* If too many malloc() calls have failed */
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
struct Thread * createThreads(unsigned int n, struct Block *block)
{
    unsigned int i;

    struct ThreadCTX *ctx;
    struct Thread *threads;

    logMessage(DEBUG, "Creating thread array");

    if (n < THREAD_COUNT_MIN || n > THREAD_COUNT_MAX)
    {
        logMessage(DEBUG, "Thread count out of range - it must be between %u and %u",
            THREAD_COUNT_MIN, THREAD_COUNT_MAX);
        return NULL;
    }

    ctx = malloc(sizeof(*ctx));

    if (!ctx)
    {
        logMessage(ERROR, "Memory allocation failed");
        return NULL;
    }

    /* Set thread count */
    ctx->count = n;

    threads = malloc(n * sizeof(*threads));
    
    if (!threads)
    {
        logMessage(ERROR, "Memory allocation failed");
        return NULL;
    }
    
    for (i = 0; i < n; ++i)
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


/* Free thread list and nested ThreadCTX struct */
void freeThreads(struct Thread *threads)
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