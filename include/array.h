#ifndef ARRAY_H
#define ARRAY_H


#include <stdbool.h>
#include <stddef.h>

#include <pthread.h>

#include "parameters.h"


typedef struct Block
{
    size_t id;                 /* ID of block (also used as row number) */
    unsigned int bCount;       /* Number of blocks in image */
    PlotCTX *parameters;       /* Image parameters */
    size_t rows;               /* Number of rows in each block */
    size_t remainderRows;      /* Number of rows in the remainder block */
    bool remainder;            /* Whether a remainder block or not */
    size_t memSize;            /* Size of each array element */
    size_t rowSize;            /* Size of each row */
    size_t blockSize;          /* Size of full-size block */
    size_t remainderBlockSize; /* Size of remainder block */
    char *array;               /* Full-size block array */
} Block;

typedef struct Thread
{
    pthread_t pid;
    unsigned int tid;
    unsigned int tCount;
    Block *block;
} Thread;


extern const unsigned int FREE_MEMORY_ALLOCATION;


Block * createBlock(void);
int initialiseBlock(Block *block, PlotCTX *p, size_t mem);
int initialiseBlockAsRow(Block *block, PlotCTX *p);
Thread * createThreads(Block *block, unsigned int n);

void freeBlock(Block *block);
void freeThreads(Thread *threads);


#endif