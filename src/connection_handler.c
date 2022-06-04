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
#include <poll.h>
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


static void initialiseWorker(NetworkCTX *network, const Block *block, Stack *rowStack);
static void releaseWorker(NetworkCTX *network, int i, Stack *rows);
static void unallocateRow(NetworkCTX *network, int i, Stack *rows);
static void completeRow(NetworkCTX *network, int i);

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

    int s;
    Connection *c = &(network->connections[0]);

    logMessage(DEBUG, "Creating socket");
    s = socket(AF_INET, SOCK_STREAM, 0);

    if (s < 0)
    {
        logMessage(ERROR, "Socket could not be created");
        return 1;
    }

    /* SOL_SOCKET = set option at socket API level
     * SO_REUSEADDR = Allow reuse of local address
     */
    logMessage(DEBUG, "Setting socket for reuse");

	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const void *) &SOCK_OPT, (socklen_t) sizeof(SOCK_OPT)))
	{
        logMessage(ERROR, "Socket could not be set for reuse");
		close(s);
		return 1;
	}

    /* TODO: Sort this out */
    /* Set socket to be nonblocking */
    /*
    logMessage(DEBUG, "Changing socket mode to nonblocking");

	if (ioctl(s, FIONBIO, (const void *) &SOCK_OPT) < 0)
	{
        logMessage(DEBUG, "Socket mode could not be changed");
		close(s);
		return 1;
	}*/

    logMessage(DEBUG, "Binding %s:%" PRIu16 " to socket",
               inet_ntoa(c->addr.sin_addr),
               ntohs(c->addr.sin_port));

    if (bind(s, (struct sockaddr *) &(c->addr), (socklen_t) sizeof(c->addr)))
    {
        logMessage(ERROR, "Could not bind socket");
        close(s);
        return 1;
    }
    
    logMessage(DEBUG, "Setting socket to listen");

    if (listen(s, network->max - 1))
    {
        logMessage(ERROR, "Socket state could not be changed");
        close(s);
        return 1;
    }

    network->fds[0].fd = s;
    network->fds[0].events = POLLIN;
    ++(network->n);
    return 0;
}


/* Accept connection request and return index in Connection array */
int acceptConnection(NetworkCTX *network)
{
    int s;

    Connection c = createConnection();
    socklen_t addrLength = sizeof(c.addr);

    logMessage(INFO, "Accepting incoming connection request");

    errno = 0;
    s = accept(network->fds[0].fd, (struct sockaddr *) &(c.addr), &addrLength);

    if (s < 0)
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
               inet_ntoa(c.addr.sin_addr),
               ntohs(c.addr.sin_port),
               s);

    for (int i = 1; i < network->max; ++i)
    {
        /* If space for the new connection */
        if (network->fds[i].fd < 0)
        {
            network->connections[i] = c;

            network->fds[i] = createPollfd();
            network->fds[i].fd = s;
            network->fds[i].events = POLLIN;
            
            ++(network->n);
            return i;
        }
    }

    logMessage(WARNING, "Too many connections have already been accepted, closing connection");
    close(s);
    return -1;
}


/* Initialise machine as worker - connect to a master and read parameters */
int initialiseAsWorker(NetworkCTX *network, PlotCTX **p)
{
    int s;
    Connection *c = &(network->connections[0]);

    logMessage(DEBUG, "Creating socket");
    s = socket(AF_INET, SOCK_STREAM, 0);

	if (s < 0)
    {
        logMessage(ERROR, "Socket could not be created");
        return 1;
    }

    logMessage(INFO, "Connecting to master at %s:%" PRIu16,
               inet_ntoa(c->addr.sin_addr),
               ntohs(c->addr.sin_port));

	if (connect(s, (struct sockaddr *) &c->addr, (socklen_t) sizeof(c->addr)))
    {
        logMessage(ERROR, "Unable to connect to master");
        close(s);
		return 1;
	}

    logMessage(DEBUG, "Getting program parameters from master");

    if (readParameters(p, s))
    {
        close(s);
        return 1;
    }

    network->fds[0] = createPollfd();
    network->fds[0].fd = s;
    ++(network->n);
	return 0;
}


void closeConnection(NetworkCTX *network, int i)
{
    logMessage(INFO, "Closing connection with socket %d", network->fds[i].fd);
    close(network->fds[i].fd);

    network->fds[i] = createPollfd();
    --(network->n);

    freeClientReceiveBuffer(&(network->connections[i]));
}


void closeAllConnections(NetworkCTX *network)
{
    for (int i = 1; i < network->max; ++i)
    {
        if (network->fds[i].fd < 0)
            continue;

        closeConnection(network, i);
    }

    closeConnection(network, 0);
}


/* Listener */
int listener(NetworkCTX *network, const Block *block)
{
    size_t wroteRows = 0;

    Stack *rowStack = createRowStack(block);
    if (!rowStack)
        return 1;

    while (1)
    {
        /* Wait for a socket to become active. On return, the set is modified to
         * only contain the active sockets (hence the reinitialisation of sets
         * at the top of the loop)
         */
        int active = poll(network->fds, (nfds_t) network->n, -1);

        if (active <= 0)
        {
            logMessage(ERROR, "Failed to poll sockets");
            destroyRowStack(rowStack);
            return 1;
        }

        for (int i = 0; i < network->max && active > 0; ++i)
        {
            ssize_t readBytes;

            char buffer[NETWORK_BUFFER_SIZE] = {'\0'};
            int s = network->fds[i].fd;

            if (!network->fds[i].revents || s < 0)
            {
                continue;
            }
            else if (network->fds[i].revents != POLLIN)
            {
                releaseWorker(network, i, rowStack);
                continue;
            }

            /* If socket is active */
            --active;

            /* If data to be read on master socket, there is a connection request */
            if (i == 0)
            {
                initialiseWorker(network, block, rowStack);
                continue;
            }

            /* Read request into buffer */
            readBytes = readSocket(buffer, s, sizeof(buffer));

            /* Client issued shutdown or error */
            if (readBytes != sizeof(buffer))
            {
                if (readBytes <= 0)
                    releaseWorker(network, i, rowStack);

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
                unallocateRow(network, i, rowStack);

                /* Get next row number for the worker to work on */
                if (!popStack(&nextRow, rowStack))
                {
                    memset(buffer, '\0', sizeof(buffer));
                    snprintf(buffer, sizeof(buffer), "%zu", nextRow);
                    logMessage(DEBUG, "Allocating row %s to worker on socket %d", buffer, s);

                    if (writeSocket(buffer, s, sizeof(buffer)) <= 0)
                        /* Client issued shutdown or error */
                        releaseWorker(network, i, rowStack);

                    network->connections[i].row = nextRow;
                    network->connections[i].rowAllocated = true;
                }

                /* If popStack fails, the stack is empty and we ignore */
            }
            else if (!strcmp(buffer, ROW_RESPONSE)) /* Row data */
            {
                Connection *c = &(network->connections[i]);

                /* Read row data into buffer */
                readBytes = readSocket(c->buffer + c->read, s, c->n - c->read);

                /* Client issued shutdown or error */
                if (readBytes <= 0)
                {
                    releaseWorker(network, i, rowStack);
                }
                else if ((size_t) readBytes == c->n - c->read)
                {       
                    size_t rows = (block->remainder) ? block->remainderRows : block->rows;

                    memcpy(block->array + c->row * c->n, c->buffer, c->n);

                    logMessage(INFO, "Row %zu from socket %d wrote to array", c->row, s);
                    completeRow(network, i);

                    if (++wroteRows >= rows)
                    {
                        logMessage(INFO, "All rows wrote to image");
                        destroyRowStack(rowStack);
                        return 0;
                    }
                }
                else
                {
                    c->read += (size_t) readBytes;
                }
            }
            else
            {
                sendError(s);
            }
            
        }
    }
}


static void initialiseWorker(NetworkCTX *network, const Block *block, Stack *rowStack)
{
    int ret;

    int i = acceptConnection(network);

    if (i < 0)
        return;

    ret = sendParameters(network->fds[i].fd, block->parameters);

    if (ret == -2)
    {
        logMessage(INFO, "Worker shutdown connection, closing connection");
        releaseWorker(network, i, rowStack);
        return;
    }
    else if (ret)
    {
        logMessage(ERROR, "Sending parameters to worker failed, closing connection");
        releaseWorker(network, i, rowStack);
        return;
    }
    
    if (createClientReceiveBuffer(&(network->connections[i]), block->rowSize))
        releaseWorker(network, i, rowStack);
}


/* Close socket connection and modify fd_set and NetworkCTX socket array */
static void releaseWorker(NetworkCTX *network, int i, Stack *rows)
{
    unallocateRow(network, i, rows);
    closeConnection(network, i);
}


static void unallocateRow(NetworkCTX *network, int i, Stack *rows)
{
    if (network->connections[i].rowAllocated)
    {
        pushStack(rows, network->connections[i].row);
        completeRow(network, i);
    }
}


static void completeRow(NetworkCTX *network, int i)
{
    network->connections[i].rowAllocated = false;
    network->connections[i].row = 0;
    network->connections[i].read = 0;
    memset(network->connections[i].buffer, '\0', network->connections[i].n);
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