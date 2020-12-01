#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "libgroot/include/log.h"
#include "percy/include/parser.h"

#include "connection_handler.h"

#include "arg_ranges.h"
#include "array.h"
#include "request_handler.h"


/* Offset in slave response where image data starts (before is row number) */
const size_t RESPONSE_IMAGE_DATA_OFFSET = 6;


static int getHighestFD(const int *fd, int n);


/* Allocate NetworkCTX object */
NetworkCTX * createNetworkCTX(int n)
{
    NetworkCTX *ctx = malloc(sizeof(*ctx));

    if (!ctx)
        return NULL;
    
    ctx->n = (n < 0) ? 0 : n;
    ctx->slaves = malloc((size_t) ctx->n * sizeof(*(ctx->slaves)));

    if (!ctx->slaves)
    {
        free(ctx);
        return NULL;
    }

    for (int i = 0; i < ctx->n; ++i)
        ctx->slaves[i] = -1;

    return ctx;
}


/* Free NetworkCTX objects */
void freeNetworkCTX(NetworkCTX *ctx)
{
    if (ctx)
    {
        if (ctx->slaves)
            free(ctx->slaves);

        free(ctx);
    }
}


/* Bind sockets where relevant for distributed computing, and generate necessary
 * network objects
 */
int initialiseNetworkConnection(NetworkCTX *network, PlotCTX **p)
{
    /* 1 minute connection request timeout */
    const time_t CONNECTION_TIMEOUT = 60;

    switch (network->mode)
    {
        case LAN_NONE:
            logMessage(INFO, "Device initialised as standalone");
            return 0;
        case LAN_MASTER:
            logMessage(INFO, "Initialising as master machine");

            if (initialiseMaster(network))
                return 1;
            
            logMessage(INFO, "Master socket initialised");
            logMessage(INFO, "Awaiting slave connection requests");

            if (acceptConnections(network, CONNECTION_TIMEOUT) < network->n)
                return 1;

            logMessage(INFO, "All slaves connected to master");
            logMessage(INFO, "Initiating slave machines");

            if (initialiseSlaves(network, *p))
                return 1;

            logMessage(INFO, "Slave machines initiated");
            break;
        case LAN_SLAVE:
            logMessage(INFO, "Initialising as slave machine");

            if (initialiseSlave(network, p))
                return 1;
            break;
        default:
            logMessage(ERROR, "Unknown networking mode");
            return 1;
    }

    return 0;
}


/* Initialise machine as master - listen for slave connection requests */
int initialiseMaster(NetworkCTX *network)
{
    const int SOCK_OPT = 1;

    logMessage(DEBUG, "Creating socket");

    network->s = socket(AF_INET, SOCK_STREAM, 0);

    if (network->s < 0)
    {
        logMessage(ERROR, "Socket could not be created");
        return 1;
    }

    /* SOL_SOCKET = set option at socket API level
     * SO_REUSEADDR = Allow reuse of local address
     */
    logMessage(DEBUG, "Setting socket for reuse");

	if (setsockopt(network->s, SOL_SOCKET, SO_REUSEADDR, (const void *) &SOCK_OPT, (socklen_t) sizeof(SOCK_OPT)))
	{
        logMessage(ERROR, "Socket could not be set for reuse");
		close(network->s);
		return 1;
	}

    /* Set socket to be nonblocking */
    logMessage(DEBUG, "Changing socket mode to nonblocking");

	if (ioctl(network->s, FIONBIO, (const void *) &SOCK_OPT) < 0)
	{
        logMessage(DEBUG, "Socket mode could not be changed");
		close(network->s);
		return 1;
	}

    logMessage(DEBUG, "Binding %s:%" PRIu16 " to socket",
               inet_ntoa(network->addr.sin_addr),
               ntohs(network->addr.sin_port));

    if (bind(network->s, (struct sockaddr *) &network->addr, (socklen_t) sizeof(network->addr)))
    {
        logMessage(ERROR, "Could not bind socket");
        close(network->s);
        return 1;
    }
    
    logMessage(DEBUG, "Setting socket to listen");

    if (listen(network->s, network->n))
    {
        logMessage(ERROR, "Socket state could not be changed");
        close(network->s);
        return 1;
    }

    return 0;
}


/* Accept connection requests on socket with a specified timeout in seconds */
int acceptConnections(NetworkCTX *network, time_t timeout)
{
    int i;
    fd_set set;

    /* Use the select() API for connection request timeout */
    FD_ZERO(&set);

    for (i = 0; i < network->n; ++i)
    {
        int activeFD;
        int slave;

        struct timeval t =
        {
            .tv_sec = timeout,
            .tv_usec = 0
        };

        FD_SET(network->s, &set);

        logMessage(DEBUG, "Waiting on connection request...");

        activeFD = select(network->s + 1, &set, NULL, NULL, &t);

        if (activeFD == -1)
        {
            logMessage(ERROR, "Socket polling failed");
            return -1;
        }
        else if (activeFD == 0)
        {
            logMessage(WARNING, "Socket polling timed out with %d connections", i);
            return i;
        }
        
        logMessage(DEBUG, "Accepting connection request");

        slave = acceptConnectionReq(network);

        if (slave >= 0)
        {
            logMessage(DEBUG, "Connection request accepted");
            network->slaves[i] = slave;
        }
        else if (slave == -2)
        {
            break;
        }
        else
        {
            --i;
        }
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
        {
            logMessage(WARNING, "%d connections have already been accepted", network->n);
			return -2;
        }
        else
        {
            logMessage(WARNING, "Could not accept connection request");
            return -1;
        }
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

        logMessage(DEBUG, "Sending parameters to slave %d", i);

        ret = sendParameters(network->slaves[i], p);

        if (ret == -2)
        {
            logMessage(INFO, "Closing connection with %d", i);
            close(network->slaves[i]);
            network->slaves[i] = -1;
        }
        else if (ret != 0)
        {
            return 1;
        }

        logMessage(DEBUG, "Parameters successfully sent to slave %d", i);
    }

	return 0;
}


/* Initialise machine as slave - connect to a master and read parameters */
int initialiseSlave(NetworkCTX *network, PlotCTX **p)
{
    logMessage(DEBUG, "Creating socket");

    network->s = socket(AF_INET, SOCK_STREAM, 0);

	if (network->s < 0)
    {
        logMessage(ERROR, "Socket could not be created");
        return 1;
    }

    logMessage(INFO, "Connecting to master at %s:%" PRIu16,
               inet_ntoa(network->addr.sin_addr),
               ntohs(network->addr.sin_port));

	if (connect(network->s, (struct sockaddr *) &network->addr, (socklen_t) sizeof(network->addr)))
	{
        logMessage(ERROR, "Unable to connect to master");
        close(network->s);
		return 1;
	}

    logMessage(DEBUG, "Getting program parameters from master");

    if (readParameters(p, network->s))
    {
        close(network->s);
        return 1;
    }

	return 0;
}


/* Listener */
int listener(NetworkCTX *network, const Block *block)
{
    /* Sets of socket file descriptors */
    fd_set set, setTemp;

    /* Stores the highest file descriptor in fd_set */
    int highestFD = getHighestFD(network->slaves, network->n);

    /* Lists of socket file descriptors */
    int *slavesTemp = malloc((size_t) network->n * sizeof(*(network->slaves)));

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

        /* Wait for a socket to become active. On return, the set is modified to
         * only contain the active sockets (hence the reinitialisation at the
         * top of the loop)
         */
        activeSockCount = select(highestFD + 1, &setTemp, NULL, NULL, NULL);

        if (activeSockCount <= 0)
            return 1;

        for (int i = 0; i < network->n && activeSockCount > 0; ++i)
        {
            char request[READ_BUFFER_SIZE] = {0};
            char sendBuffer[10] = {0};

            ssize_t readBytes;

            int activeSock = slavesTemp[i];

            if (!FD_ISSET(activeSock, &setTemp))
                continue;

            /* If socket is active */
            activeSockCount--;
            
            /* Read request into buffer */
            readBytes = readSocket(request, activeSock, sizeof(request));

            /* Client issued shutdown or error */
            if (readBytes <= 0)
            {
                /* Close socket */
                logMessage(INFO, "Closing connection with socket %d", activeSock);
                close(activeSock);
                FD_CLR(activeSock, &set);
                network->slaves[i] = -1;

                /* Recalculate highest FD in set (if needed) */
                if (activeSock == highestFD)
                    highestFD = getHighestFD(network->slaves, network->n);
            }
            else if (readBytes == 2)
            {
                request[sizeof(request) - 1] = '\0';

                if (strcmp(">", request))
                {
                    logMessage(WARNING, "Invalid request from slave on socket %d", activeSock);
                    continue;
                }
                else if (allocatedRows < rows)
                {
                    snprintf(sendBuffer, sizeof(sendBuffer), "%zu", allocatedRows++);
                    logMessage(INFO, "Allocating row %s to slave on socket %d", sendBuffer, activeSock);

                    /* Client issued shutdown or error */
                    if (writeSocket(sendBuffer, activeSock, strlen(sendBuffer) + 1) <= 0)
                    {
                        /* Close socket */
                        logMessage(INFO, "Closing connection with socket %d", activeSock);
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
                    /* Close socket */
                    logMessage(INFO, "Closing connection with socket %d", activeSock);
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
                uintmax_t tempUIntMax = 0;
                char *endptr;
                size_t rowNum;
                char *a = block->ctx->array->array;
                size_t rowSize = (block->ctx->array->params->colour.depth == BIT_DEPTH_ASCII)
                                 ? block->ctx->array->params->width * sizeof(char)
                                 : block->ctx->array->params->width * block->ctx->array->params->colour.depth / 8;

                request[RESPONSE_IMAGE_DATA_OFFSET - 1] = '\0';
                request[sizeof(request) - 1] = '\0';

                if (stringToUIntMax(&tempUIntMax, request, 0, block->rows - 1, &endptr, BASE_DEC) != PARSE_SUCCESS)
                {
                    logMessage(WARNING, "Invalid row number \'%s\' on socket %d", activeSock);
                }
                else
                {
                    rowNum = (size_t) tempUIntMax;
                    logMessage(INFO, "Writing row %zu to array", rowNum);
                    memcpy(a + rowNum * rowSize, request + RESPONSE_IMAGE_DATA_OFFSET, rowSize);

                    if (++wroteRows == block->rows)
                        return 0;
                }

                snprintf(sendBuffer, sizeof(sendBuffer), ">");

                /* Client issued shutdown or error */
                if (writeSocket(sendBuffer, activeSock, strlen(sendBuffer) + 1) <= 0)
                {
                    /* Close socket */
                    logMessage(INFO, "Closing connection with socket %d", activeSock);
                    close(activeSock);
                    FD_CLR(activeSock, &set);
                    network->slaves[i] = -1;

                    /* Recalculate highest FD in set (if needed) */
                    if (activeSock == highestFD)
                        highestFD = getHighestFD(network->slaves, network->n);
                }
            }
        }
    }
}


/* Calculate the highest FD in set */
static int getHighestFD(const int *fd, int n)
{
    int highestFD = -1;

    for (int i = 0; i < n; ++i)
    {
        if (fd[i] > highestFD)
            highestFD = fd[i];
    }

    return highestFD;
}