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
    struct PlotCTX *parameters;
};

struct BlockCTX
{
    unsigned int count;
    size_t rows, remainder;
    struct ArrayCTX *array;
};

struct Block
{
    unsigned int id;
    size_t rows;
    struct BlockCTX *ctx;
};

struct ThreadCTX
{
    unsigned int count;
};

struct Thread
{
    pthread_t pid;
    unsigned int tid;
    struct Block *block;
    struct ThreadCTX *ctx;
};


struct ArrayCTX * createArrayCTX(struct PlotCTX *parameters);
struct Block * mallocArray(struct ArrayCTX *array, size_t bytes);
struct Thread * createThreads(struct Block *block, unsigned int n);

void freeArrayCTX(struct ArrayCTX *ctx);
void freeBlock(struct Block *block);
void freeThreads(struct Thread *threads);


#endif