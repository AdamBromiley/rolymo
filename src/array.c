#include "array.h"

#include <stddef.h>
#include <stdlib.h>
#include <sys/sysinfo.h>

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
    ctx->threadCount = (unsigned int) get_nprocs();
    ctx->parameters = parameters;

    return ctx;
}


/* To prevent memory overcommitment, the array must be divided into blocks */
struct Block * mallocArray(struct ArrayCTX *array)
{
    /* Number of malloc() attempts before failure */
    const unsigned int MALLOC_ESCAPE_ITERATIONS = 16;

    struct BlockCTX *ctx;
    struct Block *block;
    size_t height, width, rowSize;

    if (!(array->parameters))
        return NULL;

    ctx = malloc(sizeof(*ctx));

    if (!ctx)
        return NULL;

    height = array->parameters->height;
    width = array->parameters->width;
    rowSize = width * sizeof(*(array->array));

    /* Try to malloc the array, with each iteration decreasing the array size */
    ctx->blockCount = 1;
    do
    {
        if (ctx->blockCount == MALLOC_ESCAPE_ITERATIONS)
        {
            /* If too many malloc() calls have failed */
            if (array)
                free(array);

            free(ctx);
            return NULL;
        }

        ctx->rows = height / ctx->blockCount;
        ctx->remainderRows = height % ctx->blockCount;

        logMessage(DEBUG, "Image array split as %u blocks (block: %zu rows, remainder: %zu rows)\n",
            ctx->blockCount, ctx->rows, ctx->remainderRows);

        array->array = malloc(ctx->rows * rowSize);

        ++(ctx->blockCount);
    }
    while (array->array == NULL);

    ctx->array = array;

    /* Create block structure */
    block = malloc(sizeof(*block));

    if (!block)
    {
        free(array);
        free(ctx);
        return NULL;
    }

    block->blockID = 0;
    block->rows = ctx->rows;
    block->ctx = ctx;

    return block;
}


/* Generate a list of threads */
struct Thread * createThreads(const struct ArrayCTX *ctx, struct Block *block)
{
    unsigned int i;

    struct Thread *threads;

    if (!ctx)
        return NULL;

    threads = malloc(ctx->threadCount * sizeof(*threads));
    
    if (!threads)
        return NULL;
    
    for (i = 0; i < ctx->threadCount; ++i)
    {
        /* Consecutive IDs allow for threads to work on different array rows */
        threads[i].threadID = i;
        threads[i].block = block;
    }

    return threads;
}


/* Free ArrayCTX struct */
void freeArrayCTX(struct ArrayCTX *ctx)
{
    if (ctx)
    {
        if (ctx->array)
            free(ctx->array);
        
        free(ctx);
    }

    return;
}


/* Free Block and nested BlockCTX structs */
void freeBlock(struct Block *block)
{
    if (block)
    {
        if (block->ctx)
            free(block->ctx);

        free(block);
    }

    return;
}


/* Free thread list */
void freeThreads(struct Thread *threads)
{
    if (threads)
        free(threads);

    return;
}