#include "connection_handler.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#include "array.h"
#include "request_handler.h"


#define READ_BUFFER_SIZE 32768


const char *MASTER_IP = "127.0.0.1";
const int CONNECTIONS_MAX = 1;


NetworkCTX * createNetworkCTX(void)
{
    return malloc(sizeof(NetworkCTX));
}


#ifdef UNUSED
int initialiseNetworkCTX(NetworkCTX *ctx)
{
    return 0;
}
#endif


void freeNetworkCTX(NetworkCTX *ctx)
{
    if (ctx)
        free(ctx);
}


int initialiseNetworkConnection(NetworkCTX *network, PlotCTX *p)
{
    const time_t CONNECTION_TIMEOUT = 10;

    network->n = 1; /* TODO: Make flexible */

    switch (network->mode)
    {
        case LAN_NONE:
            return 0;
        case LAN_MASTER:
            network->s = initialiseMaster(network);

            if (network->s < 0)
                return 1;

            for (int i = 0; i < network->n; ++i)
                network->slaves[i] = -1;

            if (acceptConnections(network->slaves, network->s, network->n, CONNECTION_TIMEOUT) < network-> n)
                return 1;

            if (initialiseSlaves(network->slaves, network->n, p))
                return 1;
            
            break;
        case LAN_SLAVE:
            network->s = initialiseSlave(p, network);

            if (network->s < 0)
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

    int s = socket(AF_INET, SOCK_STREAM, 0);

    if (s < 0)
        return -1;

    /* 
     * SOL_SOCKET = set option at socket API level
     * SO_REUSEADDR = Allow reuse of local address
     */
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const void *) &SOCK_OPT, (socklen_t) sizeof(SOCK_OPT)))
	{
		close(s);
		return -1;
	}

    /* Set socket to be nonblocking */
	if (ioctl(s, FIONBIO, (const void *) &SOCK_OPT) < 0)
	{
		close(s);
		return -1;
	}

    if (bind(s, (struct sockaddr *) &network->addr, (socklen_t) sizeof(network->addr)))
    {
        close(s);
        return -1;
    }
    
    if (listen(s, CONNECTIONS_MAX))
    {
        close(s);
        return -1;
    }

    return s;
}


/* Accept `n` connection requests on `s` with a specified timeout in seconds */
int acceptConnections(int *slaves, int s, int n, time_t timeout)
{
    int i;

    fd_set set;

    FD_ZERO(&set);
    FD_SET(s, &set);

    for (i = 0; i < n; ++i)
    {
        int slave;

        struct timeval t =
        {
            .tv_sec = timeout,
            .tv_usec = 0
        };

        int activeFD = select(s + 1, &set, NULL, NULL, &t);

        if (activeFD == -1)
            return -1;
        else if (activeFD == 0)
            return i;
        
        slave = acceptConnectionReq(s);

        if (slave >= 0)
            slaves[i] = slave;
        else if (slave == -2)
            break;
        else
            --i;
    }

    return i;
}


int acceptConnectionReq(int master)
{
    int slave;

    errno = 0;
    slave = accept(master, NULL, NULL);

	if (slave < 0)
	{
		/* If all requests have been accepted */
		if (errno == EWOULDBLOCK)
			return -2;
        else
            return -1;
	}

    return slave;
}


/* Prepare slave machines */
int initialiseSlaves(int *slaves, int n, PlotCTX *p)
{
    for (int i = 0; i < n; ++i)
    {
        int ret;

        if (slaves[i] == -1)
            continue;

        ret = sendParameters(slaves[i], p);

        if (ret == -2)
        {
            close(slaves[i]);
            slaves[i] = -1;
        }
        else if (ret != 0)
        {
            return 1;
        }
    }

	return 0;
}


/* Initialise machine as slave - connect to a master */
int initialiseSlave(PlotCTX *p, NetworkCTX *network)
{
    int s = socket(AF_INET, SOCK_STREAM, 0);

	if (s < 0)
		return -1;

	if (connect(s, (struct sockaddr *) &network->addr, (socklen_t) sizeof(network->addr)))
	{
        close(s);
		return -1;
	}

    if (readParameters(s, p))
    {
        close(s);
        return -1;
    }

	return s;
}


/* Listener */
int listener(int *slaves, int n, Block *block)
{
    size_t rows = block->rows;
    size_t wroteRows = 0;

    /* Sets of socket file descriptors */
    fd_set set, setTemp;

    /* Highest file descriptor in fd_set */
    int highestFD = -1;

    /* Lists of socket file descriptors */
    int *slavesTemp = malloc((size_t) n * sizeof(int));

    if (!slavesTemp)
        return 1;

    /* Clear set */
    FD_ZERO(&set);

    for (int i = 0; i < n; ++i)
    {
        if (slaves[i] == -1)
            continue;
        
        FD_SET(slaves[i], &set);

        if (slaves[i] > highestFD)
            highestFD = slaves[i];
    }

    while (1)
    {
        int activeSockCount;

        if (highestFD == -1)
            return 0;

        /* Initialise working set and array */
        memcpy(&setTemp, &set, sizeof(set));
        memcpy(slavesTemp, slaves, (size_t) n * sizeof(*slaves));

        /* 
         * Wait for a socket to become active. On return, the set is modified to
         * only contain the active sockets (hence the reinitialisation at the
         * top of the loop)
         */
        activeSockCount = select(highestFD + 1, &setTemp, NULL, NULL, NULL);

        if (activeSockCount <= 0)
            return 1;

        for (int i = 0; i < n && activeSockCount > 0; ++i)
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

            if (readBytes <= 0)
            {
                puts("<=0");
                /* Client issued shutdown */
                if (readBytes == 0)
                {
                    /* Close socket */
                    close(activeSock);
                    FD_CLR(activeSock, &set);
                    slaves[i] = -1;

                    /* Recalculate highest FD in set (if needed) */
                    if (activeSock == highestFD)
                    {
                        highestFD = -1;

                        for (int j = 0; j < n; j++)
                        {
                            if (slaves[j] > highestFD)
                                highestFD = slaves[j];
                        }
                    }
                }
            }
            else if (readBytes == 1)
            {
                if (rows != 0)
                {
                    snprintf(sendBuffer, sizeof(sendBuffer), "%zu", rows--);
                    writeSocket(sendBuffer, activeSock, strlen(sendBuffer));
                    memset(sendBuffer, '\0', sizeof(sendBuffer));
                }
                else
                {
                    /* Close socket */
                    close(activeSock);
                    FD_CLR(activeSock, &set);
                    slaves[i] = -1;

                    /* Recalculate highest FD in set (if needed) */
                    if (activeSock == highestFD)
                    {
                        highestFD = -1;

                        for (int j = 0; j < n; j++)
                        {
                            if (slaves[j] > highestFD)
                                highestFD = slaves[j];
                        }
                    }
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