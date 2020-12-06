#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <unistd.h>

#include "libgroot/include/log.h"

#include "array.h"

#include "parameters.h"


/* Percentage of free physical memory that can be allocated by the program */
const unsigned int FREE_MEMORY_ALLOCATION = 80;


static int allocateImageBlock(Block *block, size_t mem);

static size_t getFreeMemory(void);
static unsigned int getThreadCount(void);


/* Create array metadata structure */
Block * createBlock(void)
{
    Block *block = malloc(sizeof(Block));

    if (block)
        block->array = NULL;
    
    return block;
}


int initialiseBlock(Block *block, PlotCTX *p, size_t mem)
{
    if (!block || !p)
        return 1;

    block->id = 0;
    block->parameters = p;
    block->remainder = false;

    block->memSize = (block->parameters->colour.depth <= CHAR_BIT || block->parameters->colour.depth == BIT_DEPTH_ASCII)
                     ? sizeof(char)
                     : block->parameters->colour.depth / CHAR_BIT;

    block->rowSize = (block->parameters->colour.depth == BIT_DEPTH_ASCII)
                     ? block->parameters->width
                     : (block->parameters->width * block->parameters->colour.depth) / CHAR_BIT;

    /* Allocate memory to the block */
    if (allocateImageBlock(block, mem))
        return 1;

    return 0;
}


int initialiseBlockAsRow(Block *block, PlotCTX *p)
{
    if (!block || !p)
        return 1;

    block->id = 0;
    block->parameters = p;
    block->rows = 1;
    block->remainderRows = 0;
    block->remainder = false;

    block->memSize = (block->parameters->colour.depth <= CHAR_BIT || block->parameters->colour.depth == BIT_DEPTH_ASCII)
                     ? sizeof(char)
                     : block->parameters->colour.depth / CHAR_BIT;

    block->rowSize = (block->parameters->colour.depth == BIT_DEPTH_ASCII)
                     ? block->parameters->width
                     : (block->parameters->width * block->parameters->colour.depth) / CHAR_BIT;

    block->blockSize = block->rowSize;
    block->remainderBlockSize = 0;

    block->array = malloc(block->blockSize);

    return (block->array) ? 0 : 1;
}


/* Generate a list of threads */
Thread * createThreads(Block *block, unsigned int n)
{
    Thread *threads;

    /* Get number of processors if user has not set a thread count limit */
    if (n < 1)
    {
        n = getThreadCount();

        if (n < 1)
        {
            n = 1;
            logMessage(WARNING, "Could not get number of online processors - limiting to %u thread(s)", n);
        }
    }

    logMessage(DEBUG, "Creating thread array");

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
        threads[i].tCount = n;
        threads[i].block = block;
    }

    logMessage(DEBUG, "Thread array generated");

    return threads;
}


/* Free Block object */
void freeBlock(Block *block)
{
    if (block)
    {
        if (block->array)
        {
            free(block->array);
            block->array = NULL;
        }

        free(block);
        logMessage(DEBUG, "Block structure freed");
    }
}


/* Free thread list */
void freeThreads(Thread *threads)
{
    if (threads)
    {
        free(threads);
        logMessage(DEBUG, "Thread array freed");
    }
}


/* To prevent memory overcommitment, the array must be divided into blocks */
static int allocateImageBlock(Block *block, size_t mem)
{
    /* Maximum number of blocks the array should be divided into */
    const unsigned int BLOCK_COUNT_MAX = 64;

    size_t freeMemory;

    logMessage(DEBUG, "Getting amount of free memory");

    freeMemory = getFreeMemory();

    if (!freeMemory)
    {
        logMessage(ERROR, "Failed to calculate amount of free memory");
        return 1;
    }

    logMessage(DEBUG, "%zu bytes of physical memory is free", freeMemory);

    /* If caller has specified max memory usage */
    if (mem > 0)
    {
        if (mem > freeMemory)
        {
            logMessage(WARNING, "Memory maximum of %zu bytes is greater than the amount of free physical memory (%zu"
                       " bytes). It is recommended to only allow allocation of physical memory for efficiency",
                       mem, freeMemory);
        }

        freeMemory = mem;
        logMessage(DEBUG, "Memory allocation will be limited to %zu bytes", freeMemory);
    }
    else
    {
        freeMemory = freeMemory * (FREE_MEMORY_ALLOCATION / 100.0);
        logMessage(DEBUG, "Memory allocation will be limited to %u%% of free physical memory (%zu bytes)",
                   FREE_MEMORY_ALLOCATION, freeMemory);
    }

    block->blockSize = block->parameters->height * block->rowSize;

    logMessage(DEBUG, "Full image is %zu bytes", block->blockSize);

    /* Try to malloc the array, with each iteration decreasing the array size */
    for (block->bCount = 1; block->bCount <= BLOCK_COUNT_MAX; ++(block->bCount))
    {
        block->rows = block->parameters->height / block->bCount;
        block->remainderRows = block->parameters->height % block->bCount;

        /* To fix the issue where the remainder block may be larger than the
         * regular ones
         */
        if (block->remainderRows > block->rows)
            continue;

        block->blockSize = block->rows * block->rowSize;
        block->remainderBlockSize = block->remainderRows * block->rowSize;

        if (block->blockSize <= freeMemory)
        {
            logMessage(DEBUG, "Splitting array into %u blocks (%zu bytes each)", block->bCount, block->blockSize);

            block->array = malloc(block->blockSize);

            if (block->array)
                break;
            
            if (block->bCount != BLOCK_COUNT_MAX)
                logMessage(DEBUG, "Memory allocation attempt failed. Retrying...");
        }
    }

    if (!block->array || block->blockSize == 0)
    {
        /* If too many malloc() calls have failed */
        logMessage(ERROR, "Memory allocation failed");

        if (block->array)
            free(block->array);

        return 1;
    }

    logMessage(DEBUG, "Image array split into %u blocks (%zu bytes - block: %zu rows, remainder block: %zu rows)",
               block->bCount, block->blockSize, block->rows, block->remainderRows);
    
    return 0;
}


/* Calculate amount of free physical memory on the system */
static size_t getFreeMemory(void)
{
    long availablePages = sysconf(_SC_AVPHYS_PAGES);
    long pageSize = sysconf(_SC_PAGE_SIZE);

    if (availablePages < 1 || pageSize < 1)
        return 0;

    return (size_t) pageSize * (size_t) availablePages;
}


/* Get number of online processors on the system (hence number of threads to
 * use)
 */
static unsigned int getThreadCount(void)
{
    long procs = sysconf(_SC_NPROCESSORS_ONLN);

    if (procs < 1)
    {
        procs = 1;
        logMessage(WARNING, "Could not get number of online processors - limiting to %ld thread(s)", procs);
    }
    else if (procs > UINT_MAX)
    {
        procs = UINT_MAX;
    }

    return (unsigned int) procs;
}