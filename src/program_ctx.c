#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "program_ctx.h"


ProgramCTX * createProgramCTX(void)
{
    return malloc(sizeof(ProgramCTX));
}


int initialiseProgramCTX(ProgramCTX *ctx)
{
    if (!ctx)
        return 1;

    strncpy(ctx->logFilepath, LOG_FILEPATH_DEFAULT, sizeof(ctx->logFilepath));
    ctx->logFilepath[sizeof(ctx->logFilepath) - 1] = '\0';

    ctx->logToFile = false;

    return 0;
}


void freeProgramCTX(ProgramCTX *ctx)
{
    if (ctx)
        free(ctx);
}