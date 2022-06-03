#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
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
#include "network_ctx.h"
#include "request_handler.h"
#include "stack.h"


static Client createClient(void);
static fd_set initialiseClientSet(const NetworkCTX *network, int *highestFD);
static void initialiseWorker(NetworkCTX *network, fd_set *set, int *highestFD, const Block *block, Stack *rowStack);
static void closeSocket(fd_set *set, int *highestFD, NetworkCTX *network, int i, Stack *rows);
static void getHighestFD(const fd_set *set, int *highestFD);

static Stack * createRowStack(const Block *block);
static void destroyRowStack(Stack *s);


/* Bind sockets where relevant for distributed computing, and generate necessary
 * network objects
 */
int initialiseNetworkConnection(NetworkCTX *network, PlotCTX **p)
{
    switch (network->mode)
    {
        case LAN_NONE:
            logMessage(INFO, "Device initialised as standalone");
            return 0;
        case LAN_MASTER:
            logMessage(INFO, "Initialising as master machine");

            if (initialiseAsMaster(network))
                return 1;
            
            logMessage(INFO, "Device initialised as master");
            break;
        case LAN_WORKER:
            logMessage(INFO, "Initialising as worker machine");

            if (initialiseAsWorker(network, p))
                return 1;

            logMessage(INFO, "Device initialised as worker");
            break;
        default:
            logMessage(ERROR, "Unknown networking mode");
            return 1;
    }

    return 0;
}


/* Initialise machine as master - listen for worker connection requests */
int initialiseAsMaster(NetworkCTX *network)
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

    /* TODO: Sort this out */
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


/* Accept connection request and return index in Client array */
int acceptConnection(NetworkCTX *network)
{
    Client worker = createClient();
    socklen_t addrLength = sizeof(worker.addr);

    logMessage(INFO, "Accepting incoming connection request");

    errno = 0;
    worker.s = accept(network->s, (struct sockaddr *) &(worker.addr), &addrLength);

    if (worker.s < 0)
    {   
        /* If all requests have been accepted.
         * EAGAIN and EWOULDBLOCK may be equal macros, so separate conditionals
         * will silence a -Wlogical-op warning if they are
         */
        if (errno == EAGAIN)
            logMessage(WARNING, "Too many connections have already been accepted");
        else if (errno == EWOULDBLOCK)
            logMessage(WARNING, "Too many connections have already been accepted");
        else
            logMessage(ERROR, "Could not accept connection request");
        
        return -1;
    }

    logMessage(INFO, "Connected to worker at %s:%" PRIu16 " on socket %d",
                        inet_ntoa(worker.addr.sin_addr),
                        ntohs(worker.addr.sin_port),
                        worker.s);

    for (int i = 0; i < network->n; ++i)
    {
        /* If space for the new connection */
        if (network->workers[i].s < 0)
        {
            network->workers[i] = worker;
            return i;
        }
    }

    logMessage(WARNING, "Too many connections have already been accepted, closing connection");
    close(worker.s);

    return -1;
}


/* Initialise machine as worker - connect to a master and read parameters */
int initialiseAsWorker(NetworkCTX *network, PlotCTX **p)
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
    /* Set of socket file descriptors */
    int highestFD;
    fd_set set = initialiseClientSet(network, &highestFD);

    /* Lists of socket file descriptors */
    Client *workersTemp = malloc((size_t) network->n * sizeof(*(network->workers)));

    Stack *rowStack;
    size_t wroteRows = 0;

    if (!workersTemp)
        return 1;

    rowStack = createRowStack(block);
    if (!rowStack)
    {
        free(workersTemp);
        return 1;
    }

    while (1)
    {
        fd_set setTemp;

        /* Number of active sockets */
        int active;

        /* Initialise working set and array */
        memcpy(&setTemp, &set, sizeof(set));
        memcpy(workersTemp, network->workers, (size_t) network->n * sizeof(*(network->workers)));

        /* Wait for a socket to become active. On return, the set is modified to
         * only contain the active sockets (hence the reinitialisation of sets
         * at the top of the loop)
         */
        active = select(highestFD + 1, &setTemp, NULL, NULL, NULL);

        if (active <= 0)
        {
            logMessage(ERROR, "Failed to poll sockets");
            free(workersTemp);
            destroyRowStack(rowStack);
            return 1;
        }

        /* If data to be read on master socket, there is a connection request */
        if (FD_ISSET(network->s, &setTemp))
        {
            --active;
            initialiseWorker(network, &set, &highestFD, block, rowStack);
        }

        for (int i = 0; i < network->n && active > 0; ++i)
        {
            ssize_t readBytes;

            char buffer[NETWORK_BUFFER_SIZE] = {'\0'};

            int s = workersTemp[i].s;

            if (!FD_ISSET(s, &setTemp))
                continue;

            /* If socket is active */
            --active;

            /* Read request into buffer */
            readBytes = readSocket(buffer, s, sizeof(buffer));

            /* Client issued shutdown or error */
            if (readBytes != sizeof(buffer))
            {
                if (readBytes <= 0)
                    closeSocket(&set, &highestFD, network, i, rowStack);

                continue;
            }

            buffer[sizeof(buffer) - 1] = '\0';

            if (!strcmp(buffer, ROW_REQUEST)) /* New row request */
            {
                size_t nextRow = 0;

                /* If the worker requests a new row, we unallocate its
                 * already-assigned row.
                 * TODO: This will just reassign the worker the same row. Maybe
                 * implement a queue, not stack?
                 */
                if (network->workers[i].rowAllocated)
                {
                    pushStack(rowStack, network->workers[i].row);
                    network->workers[i].rowAllocated = false;
                }

                /* Get next row number for the worker to work on */
                if (!popStack(&nextRow, rowStack))
                {
                    memset(buffer, '\0', sizeof(buffer));
                    snprintf(buffer, sizeof(buffer), "%zu", nextRow);
                    logMessage(DEBUG, "Allocating row %s to worker on socket %d", buffer, s);

                    if (writeSocket(buffer, s, sizeof(buffer)) <= 0)
                        /* Client issued shutdown or error */
                        closeSocket(&set, &highestFD, network, i, rowStack);

                    network->workers[i].row = nextRow;
                    network->workers[i].rowAllocated = true;
                }

                /* If popStack fails, the stack is empty and we ignore */
            }
            else if (!strcmp(buffer, ROW_RESPONSE)) /* Row data */
            {
                Client *worker = &(workersTemp[i]);

                /* Read row data into buffer */
                readBytes = readSocket(worker->buffer + worker->read, s, worker->n - worker->read);

                /* Client issued shutdown or error */
                if (readBytes <= 0)
                {
                    closeSocket(&set, &highestFD, network, i, rowStack);
                }
                else if ((size_t) readBytes == worker->n - worker->read)
                {       
                    size_t rows = (block->remainder) ? block->remainderRows : block->rows;

                    memcpy(block->array + worker->row * worker->n, worker->buffer, worker->n);

                    network->workers[i].rowAllocated = false;
                    network->workers[i].row = 0;
                    network->workers[i].read = 0;
                    memset(network->workers[i].buffer, '\0', network->workers[i].n);

                    logMessage(INFO, "Row %zu from socket %d wrote to array", worker->row, s);

                    if (++wroteRows >= rows)
                    {
                        logMessage(INFO, "All rows wrote to image");
                        free(workersTemp);
                        destroyRowStack(rowStack);
                        return 0;
                    }
                }
                else
                {
                    network->workers[i].read += (size_t) readBytes;
                }
            }
            else
            {
                sendError(s);
            }
            
        }
    }
}


static Client createClient(void)
{
    Client client =
    {
        .s = -1,
        .rowAllocated = false,
        .row = 0,
        .n = 0,
        .read = 0,
        .buffer = NULL
    };

    return client;
}


static fd_set initialiseClientSet(const NetworkCTX *network, int *highestFD)
{
    fd_set set;

    /* Clear set */
    FD_ZERO(&set);

    /* Add master socket to set for connection requests */
    FD_SET(network->s, &set);
    *highestFD = network->s;

    /* Add client sockets to set */
    for (int i = 0; i < network->n; ++i)
    {
        int s = network->workers[i].s;

        if (s == -1)
            continue;

        FD_SET(s, &set);

        if (s > *highestFD)
            *highestFD = s;
    }

    return set;
}


static void initialiseWorker(NetworkCTX *network, fd_set *set, int *highestFD, const Block *block, Stack *rowStack)
{
    int s;
    int ret;

    int i = acceptConnection(network);

    if (i < 0)
        return;

    s = network->workers[i].s;

    FD_SET(s, set);

    if (s > *highestFD)
        *highestFD = s;

    ret = sendParameters(s, block->parameters);

    if (ret == -2)
    {
        logMessage(INFO, "Worker shutdown connection, closing connection");
        closeSocket(set, highestFD, network, i, rowStack);
        return;
    }
    else if (ret)
    {
        logMessage(ERROR, "Sending parameters to worker failed, closing connection");
        closeSocket(set, highestFD, network, i, rowStack);
        return;
    }
    
    if (createClientReceiveBuffer(&(network->workers[i]), block->rowSize))
        closeSocket(set, highestFD, network, i, rowStack);

    printf("network->workers[i].rowAllocated = %d\n", network->workers[i].rowAllocated);
}


/* Close socket connection and modify fd_set and NetworkCTX socket array */
static void closeSocket(fd_set *set, int *highestFD, NetworkCTX *network, int i, Stack *rows)
{
    int s = network->workers[i].s;

    logMessage(INFO, "Closing connection with socket %d", s);

    close(s);
    FD_CLR(s, set);
    network->workers[i].s = -1;

    if (network->workers[i].rowAllocated)
    {
        pushStack(rows, network->workers[i].row);
        network->workers[i].rowAllocated = false;
    }
    
    freeClientReceiveBuffer(&(network->workers[i]));

    /* Recalculate highest FD in set (if needed) */
    if (s == *highestFD)
        getHighestFD(set, highestFD);
}


/* Calculate the highest FD in set */
static void getHighestFD(const fd_set *set, int *highestFD)
{
    while (!FD_ISSET(*highestFD, set))
        --(*highestFD);
}


static Stack * createRowStack(const Block *block)
{
    size_t rows = (block->remainder) ? block->remainderRows : block->rows;
    Stack *s = createStack(rows);

    size_t blockOffset = block->id * block->rows;

    if (!s)
        return NULL;
    
    for (size_t i = blockOffset; i < blockOffset + rows; ++i)
    {
        if (pushStack(s, i))
        {
            freeStack(s);
            return NULL;
        }
    }

    return s;
}


static void destroyRowStack(Stack *s)
{
    freeStack(s);
}