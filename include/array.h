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


extern unsigned int THREAD_COUNT_MIN;
extern unsigned int THREAD_COUNT_MAX;


struct ArrayCTX * createArrayCTX(struct PlotCTX *parameters);
struct Block * mallocArray(struct ArrayCTX *array);
struct Thread * createThreads(unsigned int n, struct Block *block);

void freeArrayCTX(struct ArrayCTX *ctx);
void freeBlock(struct Block *block);
void freeThreads(struct Thread *threads);


#endif