#ifndef ARRAY_H
#define ARRAY_H


#include <stddef.h>

#include <pthread.h>

#include "parameters.h"


typedef struct ArrayCTX
{
    void *array;
    PlotCTX *params;
} ArrayCTX;

typedef struct RowCTX
{
    size_t row;
    ArrayCTX *array;
} RowCTX;

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

typedef struct SlaveThread
{
    pthread_t pid;
    unsigned int tid;
    RowCTX *row;
    ThreadCTX *ctx;
} SlaveThread;


extern const unsigned int FREE_MEMORY_ALLOCATION;


ArrayCTX * createArrayCTX(PlotCTX *p);
Block * mallocArray(ArrayCTX *array, size_t bytes);
Thread * createThreads(Block *block, unsigned int n);
SlaveThread * createSlaveThreads(RowCTX *row, unsigned int n);

void freeArrayCTX(ArrayCTX *ctx);
void freeBlock(Block *block);
void freeThreads(Thread *threads);
void freeSlaveThreads(SlaveThread *threads);


#endif