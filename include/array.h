#ifndef ARRAY_H
#define ARRAY_H


#include <pthread.h>
#include <stddef.h>

#include "parameters.h"


typedef struct ArrayCTX
{
    void *array;
    PlotCTX *params;
} ArrayCTX;

typedef struct BlockCTX
{
    unsigned int count;
    size_t rows, remainder;
    ArrayCTX *array;
} BlockCTX;

typedef struct Block
{
    unsigned int id;
    size_t rows;
    BlockCTX *ctx;
} Block;

typedef struct ThreadCTX
{
    unsigned int count;
} ThreadCTX;

typedef struct Thread
{
    pthread_t pid;
    unsigned int tid;
    Block *block;
    ThreadCTX *ctx;
} Thread;


ArrayCTX * createArrayCTX(PlotCTX *p);
Block * mallocArray(ArrayCTX *array, size_t bytes);
Thread * createThreads(Block *block, unsigned int n);

void freeArrayCTX(ArrayCTX *ctx);
void freeBlock(Block *block);
void freeThreads(Thread *threads);


#endif