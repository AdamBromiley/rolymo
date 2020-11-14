#ifndef PROGRAM_CTX_H
#define PROGRAM_CTX_H


#include <stdbool.h>
#include <stddef.h>


#define FILEPATH_LEN_MAX 4096
#define PLOT_FILEPATH_DEFAULT "var/mandelbrot.pnm"
#define LOG_FILEPATH_DEFAULT "var/mandelbrot.log"


typedef struct ProgramCTX
{
    char plotFilepath[FILEPATH_LEN_MAX];
    char logFilepath[FILEPATH_LEN_MAX];
    bool logToFile;
    size_t mem;
    unsigned int threads;
} ProgramCTX;


ProgramCTX * createProgramCTX(void);
int initialiseProgramCTX(ProgramCTX *ctx);
void freeProgramCTX(ProgramCTX *ctx);


#endif