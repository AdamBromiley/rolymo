#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "libgroot/include/log.h"
#include "percy/include/parser.h"

#include "request_handler.h"

#include "array.h"
#include "ext_precision.h"
#include "network_ctx.h"
#include "parameters.h"
#include "serialise.h"


ssize_t writeSocket(const void *src, int s, size_t n)
{
    size_t sentBytes = 0;

    do
    {
        ssize_t ret;
        const char *srctmp = src;

        errno = 0;
        ret = send(s, srctmp + sentBytes, n - sentBytes, 0);

        if (ret < 0)
        {
            if (errno == EINTR)
            {
                /* Write call interrupted - try again */
                continue;
            }
            else if (errno == ECONNRESET) /* Connection closed */
            {
                logMessage(INFO, "Connection with peer closed");
                return 0;
            }
            else
            {
                logMessage(ERROR, "Could not write to connection");
                return -1;
            }
        }

        sentBytes += (size_t) ret;
    }
    while (sentBytes < n);

    return (sentBytes > SSIZE_MAX) ? SSIZE_MAX : (ssize_t) sentBytes;
}


int blockingRead(NetworkCTX *network, int i, size_t n)
{
    size_t freeSpace = network->connections[i].n - network->connections[i].read;
    size_t totalReadBytes = 0;

    if (freeSpace < n)
    {
        logMessage(WARNING, "Cannot read %zu bytes into buffer with %zu free bytes", n, freeSpace);
        return 2;
    }

    while (totalReadBytes < n)
    {
        ssize_t readBytes;
        errno = 0;
        readBytes = recv(
            network->fds[i].fd,
            network->connections[i].buffer + network->connections[i].read,
            n - totalReadBytes,
            0
        );

        if (readBytes == 0)
        {
            logMessage(INFO, "Connection with peer closed");
            return 1;
        }
        else if (readBytes < 0)
        {
            if (errno == EINTR)
                continue;
            
            logMessage(WARNING, "Could not read data from peer");
            return 2;
        }

        network->connections[i].read += (size_t) readBytes;
        totalReadBytes += (size_t) readBytes;
    }

    return 0;
}


int getRowNumber(Block *block, NetworkCTX *network, const PlotCTX *p)
{
    int ret;
    char *endptr;
    uintmax_t tempRow = 0;

    clearClientReceiveBuffer(&(network->connections[0]));
    ret = blockingRead(network, 0, network->connections[0].n);

    if (ret)
        return ret;

    network->connections[0].buffer[sizeof(network->connections[0].n) - 1] = '\0';

    if (stringToUIntMax(&tempRow, network->connections[0].buffer, 0, p->height - 1, &endptr, BASE_DEC) != PARSE_SUCCESS)
        return 2;

    block->id = tempRow;
    return 0;
}


int nonblockingRead(NetworkCTX *network, int i)
{
    while (network->connections[i].read < network->connections[i].n)
    {
        ssize_t readBytes;

        errno = 0;
        readBytes = recv(
            network->fds[i].fd,
            network->connections[i].buffer + network->connections[i].read,
            network->connections[i].n - network->connections[i].read,
            0
        );

        if (readBytes == 0)
        {
            logMessage(INFO, "Connection with peer closed");
            return 1;
        }
        else if (readBytes < 0)
        {
            if (errno == EAGAIN)
                break;
            else if (errno == EWOULDBLOCK)
                break;

            logMessage(WARNING, "Could not read data from peer");
            return 2;
        }

        network->connections[i].read += (size_t) readBytes;
    }

    return 0;
}


int readParameters(NetworkCTX *network, PlotCTX **p)
{
    int ret;
    PrecisionMode precision;

    clearClientReceiveBuffer(&(network->connections[0]));

    logMessage(DEBUG, "Reading precision mode");
    if (blockingRead(network, 0, network->connections[0].n))
        return 1;

    logMessage(DEBUG, "Deserialising precision mode");

    #ifndef MP_PREC
    if (deserialisePrecision(&precision, network->connections[0].buffer))
    #else
    if (deserialisePrecision(&precision, &mpSignificandSize, network->connections[0].buffer))
    #endif
    {
        logMessage(ERROR, "Could not deserialise precision mode");
        return 1;
    }

    clearClientReceiveBuffer(&(network->connections[0]));

    logMessage(DEBUG, "Reading plot parameters");
    if (blockingRead(network, 0, network->connections[0].n))
        return 1;

    logMessage(DEBUG, "Creating plot parameters structure");
    *p = createPlotCTX(precision);

    if (!*p)
    {
        logMessage(ERROR, "Could not create plot parameters structure");
        return 1;
    }

    logMessage(DEBUG, "Deserialising plot parameters");
    switch((*p)->precision)
    {
        case STD_PRECISION:
            ret = deserialisePlotCTX(*p, network->connections[0].buffer);
            break;
        case EXT_PRECISION:
            ret = deserialisePlotCTXExt(*p, network->connections[0].buffer);
            break;

        #ifdef MP_PREC
        case MUL_PRECISION:
            ret = deserialisePlotCTXMP(*p, network->connections[0].buffer);
            break;
        #endif

        default:
            logMessage(ERROR, "Unknown precision mode");
            freePlotCTX(*p);
            return 1;
    }

    if (ret)
    {
        logMessage(ERROR, "Could not deserialise plot parameters");
        freePlotCTX(*p);
        return 1;
    }

    return 0;
}


int sendParameters(NetworkCTX *network, int i, const PlotCTX *p)
{
    int ret;
    ssize_t bytes;

    logMessage(DEBUG, "Serialising precision mode");
    clearClientReceiveBuffer(&(network->connections[0]));

    #ifndef MP_PREC
    ret = serialisePrecision(
        network->connections[0].buffer,
        network->connections[0].n,
        p->precision
    );
    #else
    ret = serialisePrecision(
        network->connections[0].buffer,
        network->connections[0].n,
        p->precision,
        mpSignificandSize
    );
    #endif

    /* If truncated or error */
    if (ret < 0 || (size_t) ret >= network->connections[0].n)
    {
        logMessage(ERROR, "Could not serialise precision mode");
        return 1;
    }

    logMessage(DEBUG, "Sending precision mode");
    bytes = writeSocket(network->connections[0].buffer, network->fds[i].fd, network->connections[0].n);

    if (bytes == 0)
    {
        /* Safe shutdown */
        return 1;
    }
    else if (bytes < 0)
    {
        /* Failure to write to the connection */
        return 2;
    }

    logMessage(DEBUG, "Serialising plot parameters");
    clearClientReceiveBuffer(&(network->connections[0]));

    switch(p->precision)
    {
        case STD_PRECISION:
            ret = serialisePlotCTX(network->connections[0].buffer, network->connections[0].n, p);
            break;
        case EXT_PRECISION:
            ret = serialisePlotCTXExt(network->connections[0].buffer, network->connections[0].n, p);
            break;

        #ifdef MP_PREC
        case MUL_PRECISION:
            ret = serialisePlotCTXMP(network->connections[0].buffer, network->connections[0].n, p);
            break;
        #endif

        default:
            logMessage(ERROR, "Invalid precision mode");
            return 2;
    }

    if (ret < 0 || (size_t) ret >= network->connections[0].n)
    {
        logMessage(ERROR, "Could not serialise plot context structure");
        return 2;
    }

    logMessage(DEBUG, "Sending plot parameters");
    bytes = writeSocket(network->connections[0].buffer, network->fds[i].fd, network->connections[0].n);
    
    if (bytes == 0)
    {
        /* Safe shutdown */
        return 1;
    }
    else if (bytes < 0)
    {
        /* Failure to write to the connection */
        return 2;
    }
    
    return 0;
}


int sendRowData(const NetworkCTX *network, Block *block)
{
    ssize_t bytes = writeSocket(block->array, network->fds[0].fd, block->rowSize);

    if (bytes == 0)
    {
        return -2;
    }
    else if (bytes < 0 || (size_t) bytes != block->rowSize)
    {
        logMessage(ERROR, "Could not write to socket connection");
        return -1;
    }

    return 0;
}