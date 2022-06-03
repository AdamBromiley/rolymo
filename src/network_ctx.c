#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "network_ctx.h"


NetworkCTX * createNetworkCTX(int n)
{
    NetworkCTX *ctx = malloc(sizeof(*ctx));

    if (!ctx)
        return NULL;
    
    ctx->n = (n < 0) ? 0 : n;
    ctx->workers = malloc((size_t) ctx->n * sizeof(*(ctx->workers)));

    if (!ctx->workers)
    {
        free(ctx);
        return NULL;
    }

    for (int i = 0; i < ctx->n; ++i)
    {
        ctx->workers[i].s = -1;
        ctx->workers[i].rowAllocated = false;
        ctx->workers[i].row = 0;
        ctx->workers[i].n = 0;
        ctx->workers[i].read = 0;
        ctx->workers[i].buffer = NULL;
    }

    return ctx;
}


void freeNetworkCTX(NetworkCTX *ctx)
{
    if (ctx)
    {
        if (ctx->workers)
        {
            for (int i = 0; i < ctx->n; ++i)
                freeClientReceiveBuffer(&(ctx->workers[i]));
        }

        free(ctx->workers);
    }

    free(ctx);
}


int createClientReceiveBuffer(Client *client, size_t n)
{
    if (client)
    {
        client->buffer = malloc(n);

        if (client->buffer)
        {
            client->n = n;
            client->read = 0;
            return 0;
        }
    }

    return 1;
}


void freeClientReceiveBuffer(Client *client)
{
    if (client)
    {
        free(client->buffer);
        client->n = 0;
        client->read = 0;
        client->buffer = NULL;
    }
}