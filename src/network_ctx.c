#include <stddef.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <poll.h>

#include "connection.h"

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