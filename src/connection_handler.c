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


static void closeSocket(fd_set *set, int *highestFD, NetworkCTX *network, int i);
static int getHighestFD(const int *fd, int n);


/* Allocate NetworkCTX object */
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
        ctx->workers[i] = -1;

    return ctx;
}


/* Free NetworkCTX objects */
void freeNetworkCTX(NetworkCTX *ctx)
{
    if (ctx)
    {
        if (ctx->workers)
            free(ctx->workers);

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
            logMessage(INFO, "Awaiting worker connection requests");

            if (acceptConnections(network, CONNECTION_TIMEOUT) < network->n)
                return 1;

            logMessage(INFO, "All workers connected to master");
            logMessage(INFO, "Initiating worker machines");

            if (initialiseWorkers(network, *p))
                return 1;

            logMessage(INFO, "Worker machines initiated");
            break;
        case LAN_WORKER:
            logMessage(INFO, "Initialising as worker machine");

            if (initialiseWorker(network, p))
                return 1;
            break;
        default:
            logMessage(ERROR, "Unknown networking mode");
            return 1;
    }

    return 0;
}


/* Initialise machine as master - listen for worker connection requests */
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
    /*
    logMessage(DEBUG, "Changing socket mode to nonblocking");

	if (ioctl(network->s, FIONBIO, (const void *) &SOCK_OPT) < 0)
	{
        logMessage(DEBUG, "Socket mode could not be changed");
		close(network->s);
		return 1;
	}*/

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
        int worker;

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

        worker = acceptConnectionReq(network);

        if (worker >= 0)
        {
            logMessage(DEBUG, "Connection request accepted");
            network->workers[i] = worker;
        }
        else if (worker == -2)
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


/* Prepare worker machines */
int initialiseWorkers(NetworkCTX *network, PlotCTX *p)
{
    for (int i = 0; i < network->n; ++i)
    {
        int ret;

        if (network->workers[i] == -1)
            continue;

        logMessage(DEBUG, "Sending parameters to worker %d", i);

        ret = sendParameters(network->workers[i], p);

        if (ret == -2)
        {
            logMessage(INFO, "Closing connection with %d", i);
            close(network->workers[i]);
            network->workers[i] = -1;
        }
        else if (ret != 0)
        {
            return 1;
        }

        logMessage(DEBUG, "Parameters successfully sent to worker %d", i);
    }

	return 0;
}


/* Initialise machine as worker - connect to a master and read parameters */
int initialiseWorker(NetworkCTX *network, PlotCTX **p)
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
    int highestFD = getHighestFD(network->workers, network->n);

    /* Lists of socket file descriptors */
    int *workersTemp = malloc((size_t) network->n * sizeof(*(network->workers)));

    size_t rows = (block->remainder) ? block->remainderRows : block->rows;

    /* Start the row counting from the top of the block */
    size_t allocatedRows = block->id * block->rows;
    size_t wroteRows = 0;

    if (!workersTemp)
        return 1;

    /* Clear set */
    FD_ZERO(&set);

    for (int i = 0; i < network->n; ++i)
    {
        if (network->workers[i] == -1)
            continue;
        
        FD_SET(network->workers[i], &set);
    }

    while (1)
    {
        int activeSockCount;

        if (highestFD == -1)
        {
            logMessage(ERROR, "Premature disconnect from all worker machines");
            free(workersTemp);
            return 1;
        }

        /* Initialise working set and array */
        memcpy(&setTemp, &set, sizeof(set));
        memcpy(workersTemp, network->workers, (size_t) network->n * sizeof(*(network->workers)));

        /* Wait for a socket to become active. On return, the set is modified to
         * only contain the active sockets (hence the reinitialisation at the
         * top of the loop)
         */
        activeSockCount = select(highestFD + 1, &setTemp, NULL, NULL, NULL);

        if (activeSockCount <= 0)
        {
            logMessage(ERROR, "Failed to poll sockets");
            free(workersTemp);
            return 1;
        }

        for (int i = 0; i < network->n && activeSockCount > 0; ++i)
        {
            ssize_t readBytes;

            char buffer[NETWORK_BUFFER_SIZE] = {'\0'};

            int activeSock = workersTemp[i];

            if (!FD_ISSET(activeSock, &setTemp))
                continue;

            /* If socket is active */
            activeSockCount--;
            
            /* Read request into buffer */
            readBytes = readSocket(buffer, activeSock, sizeof(buffer));

            /* Client issued shutdown or error */
            if (readBytes <= 0)
            {
                closeSocket(&set, &highestFD, network, i);
                continue;
            }
            else if (readBytes != sizeof(buffer))
            {
                continue;
            }

            buffer[sizeof(buffer) - 1] = '\0';

            if (!strcmp(buffer, "REQ"))
            {
                if (allocatedRows < rows)
                {
                    memset(buffer, '\0', sizeof(buffer));
                    snprintf(buffer, sizeof(buffer), "%zu", allocatedRows++);
                    logMessage(DEBUG, "Allocating row %s to worker on socket %d", buffer, activeSock);

                    /* Client issued shutdown or error */
                    if (writeSocket(buffer, activeSock, sizeof(buffer)) <= 0)
                        closeSocket(&set, &highestFD, network, i);
                }
                else
                {
                    closeSocket(&set, &highestFD, network, i);
                }
            }
            else
            {
                char *endptr;
                uintmax_t tempUIntMax = 0;

                if (stringToUIntMax(&tempUIntMax, buffer, 0, block->rows - 1, &endptr, BASE_DEC) != PARSE_SUCCESS)
                {
                    logMessage(WARNING, "Invalid row number received on socket %d", activeSock);
                    
                    if (sendError(activeSock))
                        closeSocket(&set, &highestFD, network, i);
                }
                else
                {
                    size_t rowNum = (size_t) tempUIntMax;
                    size_t rowSize = block->rowSize;

                    char *tempRow = malloc(rowSize);

                    if (!tempRow)
                        continue;

                    /* Read request into buffer */
                    memset(tempRow, '\0', rowSize);
                    readBytes = readSocket(tempRow, activeSock, rowSize);

                    /* Client issued shutdown or error */
                    if (readBytes <= 0)
                    {
                        closeSocket(&set, &highestFD, network, i);
                        free(tempRow);
                    }
                    else if ((size_t) readBytes != rowSize)
                    {
                        logMessage(WARNING, "Row size does not correspond to request on socket %d", activeSock);
                        sendError(activeSock);
                        free(tempRow);
                    }
                    else
                    {                        
                        memcpy(block->array + rowNum * rowSize, tempRow, rowSize);
                        free(tempRow);

                        logMessage(INFO, "Row %zu from socket %d wrote to array", rowNum, activeSock);

                        if (sendAcknowledgement(activeSock))
                            closeSocket(&set, &highestFD, network, i);

                        if (++wroteRows >= rows)
                        {
                            logMessage(INFO, "All rows wrote to image");
                            free(workersTemp);
                            return 0;
                        }
                    }
                }
            }
        }
    }
}


/* Close socket connection and modify fd_set and NetworkCTX socket array */
static void closeSocket(fd_set *set, int *highestFD, NetworkCTX *network, int i)
{
    int s = network->workers[i];

    logMessage(INFO, "Closing connection with socket %d", s);

    close(s);
    FD_CLR(s, set);
    network->workers[i] = -1;

    /* Recalculate highest FD in set (if needed) */
    if (s == *highestFD)
        *highestFD = getHighestFD(network->workers, network->n);
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