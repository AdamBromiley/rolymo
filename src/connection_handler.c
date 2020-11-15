#include "connection_handler.h"

#include "array.h"
#include "request_handler.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#define READ_BUFFER_SIZE 32768


static int getHighestFD(int *fd, int n);


NetworkCTX * createNetworkCTX(void)
{
    return malloc(sizeof(NetworkCTX));
}


int initialiseNetworkCTX(NetworkCTX *ctx, int n)
{
    if (!ctx || ctx->slaves)
        return 1;

    ctx->n = (n < 0) ? 0 : n;
    ctx->slaves = malloc((size_t) ctx->n * sizeof(*(ctx->slaves)));

    if (!ctx->slaves)
        return 1;

    for (int i = 0; i < ctx->n; ++i)
        ctx->slaves[i] = -1;

    return 0;
}


void freeNetworkCTX(NetworkCTX *ctx)
{
    if (ctx)
    {
        free(ctx->slaves);
        free(ctx);
    }
}


int initialiseNetworkConnection(NetworkCTX *network, PlotCTX *p)
{
    const time_t CONNECTION_TIMEOUT = 10;

    switch (network->mode)
    {
        case LAN_NONE:
            return 0;
        case LAN_MASTER:
            if (initialiseMaster(network))
                return 1;

            if (acceptConnections(network, CONNECTION_TIMEOUT) < network->n)
                return 1;

            if (initialiseSlaves(network, p))
                return 1;
            
            break;
        case LAN_SLAVE:
            if (initialiseSlave(network, p))
                return 1;

            break;
        default:
            return 1;
    }

    return 0;
}


/* Initialise machine as master - listen for slave connection requests */
int initialiseMaster(NetworkCTX *network)
{
    const int SOCK_OPT = 1;

    network->s = socket(AF_INET, SOCK_STREAM, 0);

    if (network->s < 0)
        return 1;

    /* 
     * SOL_SOCKET = set option at socket API level
     * SO_REUSEADDR = Allow reuse of local address
     */
	if (setsockopt(network->s, SOL_SOCKET, SO_REUSEADDR, (const void *) &SOCK_OPT, (socklen_t) sizeof(SOCK_OPT)))
	{
		close(network->s);
		return 1;
	}

    /* Set socket to be nonblocking */
	if (ioctl(network->s, FIONBIO, (const void *) &SOCK_OPT) < 0)
	{
		close(network->s);
		return 1;
	}

    if (bind(network->s, (struct sockaddr *) &network->addr, (socklen_t) sizeof(network->addr)))
    {
        close(network->s);
        return 1;
    }
    
    if (listen(network->s, network->n))
    {
        close(network->s);
        return 1;
    }

    return 0;
}


/* Accept `n` connection requests on `s` with a specified timeout in seconds */
int acceptConnections(NetworkCTX *network, time_t timeout)
{
    int i;
    fd_set set;

    FD_ZERO(&set);
    FD_SET(network->s, &set);

    for (i = 0; i < network->n; ++i)
    {
        int slave;

        struct timeval t =
        {
            .tv_sec = timeout,
            .tv_usec = 0
        };

        int activeFD = select(network->s + 1, &set, NULL, NULL, &t);

        if (activeFD == -1)
            return -1;
        else if (activeFD == 0)
            return i;
        
        slave = acceptConnectionReq(network);

        if (slave >= 0)
            network->slaves[i] = slave;
        else if (slave == -2)
            break;
        else
            --i;
    }

    return i;
}


int acceptConnectionReq(NetworkCTX *network)
{
    int s;

    errno = 0;
    s = accept(network->s, NULL, NULL);

	if (s < 0)
	{
		/* If all requests have been accepted */
		if (errno == EWOULDBLOCK)
			return -2;
        else
            return -1;
	}

    return s;
}


/* Prepare slave machines */
int initialiseSlaves(NetworkCTX *network, PlotCTX *p)
{
    for (int i = 0; i < network->n; ++i)
    {
        int ret;

        if (network->slaves[i] == -1)
            continue;

        ret = sendParameters(network->slaves[i], p);

        if (ret == -2)
        {
            close(network->slaves[i]);
            network->slaves[i] = -1;
        }
        else if (ret != 0)
        {
            return 1;
        }
    }

	return 0;
}


/* Initialise machine as slave - connect to a master */
int initialiseSlave(NetworkCTX *network, PlotCTX *p)
{
    network->s = socket(AF_INET, SOCK_STREAM, 0);

	if (network->s < 0)
		return 1;

	if (connect(network->s, (struct sockaddr *) &network->addr, (socklen_t) sizeof(network->addr)))
	{
        close(network->s);
		return 1;
	}

    if (readParameters(network->s, p))
    {
        close(network->s);
        return 1;
    }

	return 0;
}


/* Listener */
int listener(NetworkCTX *network, Block *block)
{
    /* Sets of socket file descriptors */
    fd_set set, setTemp;

    /* Stores the highest file descriptor in fd_set */
    int highestFD = getHighestFD(network->slaves, network->n);

    /* Lists of socket file descriptors */
    int *slavesTemp = malloc((size_t) network->n * sizeof(int));

    size_t rows = block->rows;
    size_t allocatedRows = 0;
    size_t wroteRows = 0;

    if (!slavesTemp)
        return 1;

    /* Clear set */
    FD_ZERO(&set);

    for (int i = 0; i < network->n; ++i)
    {
        if (network->slaves[i] == -1)
            continue;
        
        FD_SET(network->slaves[i], &set);
    }

    while (1)
    {
        int activeSockCount;

        if (highestFD == -1)
            return 0;

        /* Initialise working set and array */
        memcpy(&setTemp, &set, sizeof(set));
        memcpy(slavesTemp, network->slaves, (size_t) network->n * sizeof(*(network->slaves)));

        /* 
         * Wait for a socket to become active. On return, the set is modified to
         * only contain the active sockets (hence the reinitialisation at the
         * top of the loop)
         */
        activeSockCount = select(highestFD + 1, &setTemp, NULL, NULL, NULL);

        if (activeSockCount <= 0)
            return 1;

        for (int i = 0; i < network->n && activeSockCount > 0; ++i)
        {
            char sendBuffer[10];

            char request[READ_BUFFER_SIZE];
            ssize_t readBytes;

            int activeSock = slavesTemp[i];

            if (!FD_ISSET(activeSock, &setTemp))
                continue;

            /* If socket is active */
            activeSockCount--;
            
            /* Read request into buffer */
            readBytes = readSocket(request, activeSock, sizeof(request));

            /* Client issued shutdown */
            if (readBytes == 0)
            {
                /* Close socket */
                close(activeSock);
                FD_CLR(activeSock, &set);
                network->slaves[i] = -1;

                /* Recalculate highest FD in set (if needed) */
                if (activeSock == highestFD)
                    highestFD = getHighestFD(network->slaves, network->n);
            }
            else if (readBytes < 0)
            {
                continue;
            }
            else if (readBytes == 1)
            {
                if (allocatedRows < rows)
                {
                    snprintf(sendBuffer, sizeof(sendBuffer), "%zu", allocatedRows++);
                    writeSocket(sendBuffer, activeSock, strlen(sendBuffer));
                    memset(sendBuffer, '\0', sizeof(sendBuffer));
                }
                else
                {
                    /* Close socket */
                    close(activeSock);
                    FD_CLR(activeSock, &set);
                    network->slaves[i] = -1;

                    /* Recalculate highest FD in set (if needed) */
                    if (activeSock == highestFD)
                        highestFD = getHighestFD(network->slaves, network->n);
                }
            }
            else
            {
                long rowNum = strtol(request, NULL, 10);
                char *a = block->ctx->array->array;
                size_t rowSize = (block->ctx->array->params->colour.depth == BIT_DEPTH_ASCII)
                                    ? block->ctx->array->params->width * sizeof(char)
                                    : block->ctx->array->params->width * block->ctx->array->params->colour.depth / 8;

                memcpy(a + ((size_t) rowNum * rowSize), request + 6, rowSize);
                ++wroteRows;

                writeSocket("", activeSock, 1);

                if (wroteRows == block->rows)
                    return 0;
            }
        }
    }
}


/* Calculate the highest FD in set */
static int getHighestFD(int *fd, int n)
{
    int highestFD = -1;

    for (int i = 0; i < n; ++i)
    {
        if (fd[i] > highestFD)
            highestFD = fd[i];
    }

    return highestFD;
}