#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <poll.h>

#include "network_ctx.h"


NetworkCTX * createNetworkCTX(LANStatus status, int n, struct sockaddr_in *addr)
{
    NetworkCTX *ctx = malloc(sizeof(*ctx));

    if (!ctx)
        return NULL;
    
    ctx->mode = status;

    ctx->max = (status == LAN_MASTER) ? n + 1 : 1;
    ctx->n = 0;
    ctx->connections = malloc((size_t) ctx->max * sizeof(*(ctx->connections)));
    ctx->fds = malloc((size_t) ctx->max * sizeof(*(ctx->fds)));

    if (!ctx->connections || !ctx->fds)
    {
        free(ctx->connections);
        free(ctx->fds);
        free(ctx);
        return NULL;
    }

    for (int i = 0; i < ctx->max; ++i)
    {
        ctx->connections[i] = createConnection();
        ctx->fds[i] = createPollfd();
    }

    /* Allocate a general-purpose network buffer for the host */
    if (createClientReceiveBuffer(&(ctx->connections[0]), GENERAL_NETWORK_BUFFER_SIZE))
    {
        free(ctx->connections);
        free(ctx->fds);
        free(ctx);
        return NULL;
    }

    ctx->connections[0].addr = *addr;

    return ctx;
}


void freeNetworkCTX(NetworkCTX *ctx)
{
    if (ctx)
    {
        if (ctx->connections)
        {
            for (int i = 0; i < ctx->max; ++i)
                freeClientReceiveBuffer(&(ctx->connections[i]));
        }

        free(ctx->connections);
        free(ctx->fds);
    }

    free(ctx);
}


Connection createConnection(void)
{
    Connection c =
    {
        .rowAllocated = false,
        .row = 0,
        .n = 0,
        .read = 0,
        .buffer = NULL
    };

    return c;
}


struct pollfd createPollfd(void)
{
    struct pollfd p =
    {
        .fd = -1,
        .events = 0,
        .revents = 0
    };

    return p;
}


int createClientReceiveBuffer(Connection *c, size_t n)
{
    if (c)
    {
        c->buffer = calloc(1, n);

        if (c->buffer)
        {
            c->n = n;
            c->read = 0;
            return 0;
        }
    }

    return 1;
}


void clearClientReceiveBuffer(Connection *c)
{
    memset(c->buffer, '\0', c->n);
    c->read = 0;
}


void freeClientReceiveBuffer(Connection *c)
{
    if (c)
    {
        free(c->buffer);
        c->n = 0;
        c->read = 0;
        c->buffer = NULL;
    }
}