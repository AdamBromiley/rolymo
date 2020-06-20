#ifndef ARRAY_H
#define ARRAY_H


#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

#include "colour.h"
#include "parameters.h"


struct ArrayCTX
{
    void *array;
    unsigned int threadCount;
    struct PlotCTX *parameters;
};

struct BlockCTX
{
    unsigned int blockCount;
    size_t rows, remainderRows;
    struct ArrayCTX *array;
};

struct Block
{
    unsigned int blockID;
    size_t rows;
    struct BlockCTX *ctx;
};

struct Thread
{
    pthread_t pthreadID;
    unsigned int threadID;
    struct Block *block;
};


struct ArrayCTX * createArrayCTX(struct PlotCTX *parameters);
struct Block * mallocArray(struct ArrayCTX *array);
struct Thread * createThreads(const struct ArrayCTX *ctx, struct Block *block);

void freeArrayCTX(struct ArrayCTX *ctx);
void freeBlock(struct Block *block);
void freeThreads(struct Thread *threads);


#endif