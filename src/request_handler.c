#include <complex.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "libgroot/include/log.h"
#include "percy/include/parser.h"

#include "request_handler.h"

#include "arg_ranges.h"
#include "colour.h"
#include "ext_precision.h"
#include "parameters.h"

#ifdef MP_PREC
#include <mpfr.h>
#include <mpc.h>
#endif


#ifdef DBL_DECIMAL_DIG
    #define SERIALISE_FLT_DIG (DBL_DECIMAL_DIG)
#else
    #ifdef DECIMAL_DIG
        #define SERIALISE_FLT_DIG (DECIMAL_DIG)
        #define SERIALISE_FLT_DIG_EXT (DECIMAL_DIG)
    #else
        #define SERIALISE_FLT_DIG (DBL_DIG + 3)
        #define SERIALISE_FLT_DIG_EXT (LDBL_DIG + 3)
    #endif
#endif


const char *ROW_REQUEST = "REQ";
const char *ACK_RESPONSE = "ACK";
const char *ERR_RESPONSE = "ERR";


static int sendRowNumber(int s, size_t n);


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


/* Read exactly n bytes of the input stream on a socket */
ssize_t readSocket(void *dest, int s, size_t n)
{
    size_t offset = 0;

    /* Repeating the recv() call allows for all n bytes to be read */
    do
    {
        ssize_t readBytes;
        char *tmpDest = dest;

        errno = 0;
        readBytes = recv(s, tmpDest + offset, n - offset, 0);

        if (readBytes == 0)
        {
            /* Shutdown request */
            logMessage(INFO, "Connection with peer closed");
            return (ssize_t) offset;
        }
        else if (readBytes < 0)
        {
            if (errno == ECONNRESET)
            {
                /* Connection closed */
                logMessage(INFO, "Connection with peer closed");
                return (ssize_t) offset;
            }
            else if (errno != EINTR)
            {
                /* If recv() was not interrupted */
                logMessage(ERROR, "Could not read from connection");
                return -1;
            }
        }

        offset += (size_t) readBytes;
    }
    while (offset < n);

    return (offset > SSIZE_MAX) ? SSIZE_MAX : (ssize_t) offset;
}


#ifndef MP_PREC
int serialisePrecision(char *dest, size_t n, PrecisionMode prec)
{
    return snprintf(dest, n, "%u", prec);
}
#else
int serialisePrecision(char *dest, size_t n, PrecisionMode prec, mpfr_prec_t bits)
{
    return mpfr_snprintf(dest, n, "%u %Pu", prec, bits);
}
#endif


#ifndef MP_PREC
int deserialisePrecision(PrecisionMode *prec, char *src)
{
    char *endptr = src;
    unsigned long tempUL = 0;
    ParseErr ret = stringToULong(&tempUL, endptr, PREC_MODE_MIN, PREC_MODE_MAX, &endptr, BASE_DEC);
    
    *prec = tempUL;

    return (ret == PARSE_SUCCESS) ? 0 : 1;
}
#else
int deserialisePrecision(PrecisionMode *prec, mpfr_prec_t *bits, char *src)
{
    char *endptr = src;
    unsigned long tempUL = 0;
    ParseErr ret = stringToULong(&tempUL, endptr, PREC_MODE_MIN, PREC_MODE_MAX, &endptr, BASE_DEC);
    
    if (ret != PARSE_EEND)
        return 1;

    *prec = tempUL;

    ret = stringToULong(&tempUL, endptr, (unsigned long) MP_BITS_MIN, (unsigned long) MP_BITS_MAX, &endptr, BASE_DEC);

    if (tempUL < MPFR_PREC_MIN || tempUL > MPFR_PREC_MAX || ret != PARSE_SUCCESS)
        return 1;

    *bits = (mpfr_prec_t) tempUL;

    return 0;
}
#endif


int serialisePlotCTX(char *dest, size_t n, const PlotCTX *p)
{
    int ret = snprintf(dest, n,
                       "%u"
                       " %.*e+%.*ei"
                       " %.*e+%.*ei"
                       " %.*e+%.*ei"
                       " %lu"
                       " %zu %zu"
                       " %u",
                       p->type,
                       SERIALISE_FLT_DIG, creal(p->minimum.c), SERIALISE_FLT_DIG, cimag(p->minimum.c),
                       SERIALISE_FLT_DIG, creal(p->maximum.c), SERIALISE_FLT_DIG, cimag(p->maximum.c),
                       SERIALISE_FLT_DIG, creal(p->c.c), SERIALISE_FLT_DIG, cimag(p->c.c),
                       p->iterations,
                       p->width, p->height,
                       p->colour.scheme);
    
    return ret;
}


int serialisePlotCTXExt(char *dest, size_t n, const PlotCTX *p)
{
    int ret = snprintf(dest, n,
                       "%u"
                       " %.*Le+%.*Lei"
                       " %.*Le+%.*Lei"
                       " %.*Le+%.*Lei"
                       " %lu"
                       " %zu %zu"
                       " %u",
                       p->type,
                       SERIALISE_FLT_DIG_EXT, creall(p->minimum.lc), SERIALISE_FLT_DIG_EXT, cimagl(p->minimum.lc),
                       SERIALISE_FLT_DIG_EXT, creall(p->maximum.lc), SERIALISE_FLT_DIG_EXT, cimagl(p->maximum.lc),
                       SERIALISE_FLT_DIG_EXT, creall(p->c.lc), SERIALISE_FLT_DIG_EXT, cimagl(p->c.lc),
                       p->iterations,
                       p->width, p->height,
                       p->colour.scheme);
    
    return ret;
}


#ifdef MP_PREC
int serialisePlotCTXMP(char *dest, size_t n, const PlotCTX *p)
{
    int ret;

    char *min = mpc_get_str(10, 0, p->minimum.mpc, MP_COMPLEX_RND);
    char *max = mpc_get_str(10, 0, p->maximum.mpc, MP_COMPLEX_RND);
    char *c = mpc_get_str(10, 0, p->c.mpc, MP_COMPLEX_RND);

    if (!min || !max || !c)
    {
        mpc_free_str(min);
        mpc_free_str(max);
        mpc_free_str(c);
        return -1;
    }

    ret = snprintf(dest, n,
                   "%u"
                   " %s"
                   " %s"
                   " %s"
                   " %lu"
                   " %zu %zu"
                   " %u",
                   p->type,
                   min,
                   max,
                   c,
                   p->iterations,
                   p->width, p->height,
                   p->colour.scheme);

    mpc_free_str(min);
    mpc_free_str(max);
    mpc_free_str(c);
    
    return ret;
}
#endif


int deserialisePlotCTX(PlotCTX *p, char *src)
{
    char *endptr = src;

    unsigned long int tempPlotType = 0UL;
    uintmax_t tempWidth = 0;
    uintmax_t tempHeight = 0;
    unsigned long int tempColourScheme = 0UL;

    if (stringToULong(&tempPlotType, endptr, 0, ULONG_MAX, &endptr, BASE_DEC) != PARSE_EEND
        || stringToComplex(&(p->minimum.c), endptr, CMPLX_MIN, CMPLX_MAX, &endptr) != PARSE_EEND
        || stringToComplex(&(p->maximum.c), endptr, CMPLX_MIN, CMPLX_MAX, &endptr) != PARSE_EEND
        || stringToComplex(&(p->c.c), endptr, CMPLX_MIN, CMPLX_MAX, &endptr) != PARSE_EEND
        || stringToULong(&(p->iterations), endptr, 0, ULONG_MAX, &endptr, BASE_DEC) != PARSE_EEND
        || stringToUIntMax(&tempWidth, endptr, WIDTH_MIN, WIDTH_MAX, &endptr, BASE_DEC) != PARSE_EEND
        || stringToUIntMax(&tempHeight, endptr, HEIGHT_MIN, HEIGHT_MAX, &endptr, BASE_DEC) != PARSE_EEND
        || stringToULong(&tempColourScheme, endptr, 0UL, ULONG_MAX, &endptr, BASE_DEC) != PARSE_SUCCESS)
    {
        return 1;
    }

    if (tempPlotType != PLOT_JULIA && tempPlotType != PLOT_MANDELBROT)
        return 1;

    p->type = tempPlotType;
    p->width = tempWidth;
    p->height = tempHeight;

    p->output = OUTPUT_NONE;
    p->file = NULL;

    if (initialiseColourScheme(&p->colour, tempColourScheme))
        return 1;

    return 0;
}


int deserialisePlotCTXExt(PlotCTX *p, char *src)
{
    char *endptr = src;

    unsigned long int tempPlotType = 0UL;
    uintmax_t tempWidth = 0;
    uintmax_t tempHeight = 0;
    unsigned long int tempColourScheme = 0UL;

    if (stringToULong(&tempPlotType, endptr, 0, ULONG_MAX, &endptr, BASE_DEC) != PARSE_EEND
        || stringToComplexL(&(p->minimum.lc), endptr, LCMPLX_MIN, LCMPLX_MAX, &endptr) != PARSE_EEND
        || stringToComplexL(&(p->maximum.lc), endptr, LCMPLX_MIN, LCMPLX_MAX, &endptr) != PARSE_EEND
        || stringToComplexL(&(p->c.lc), endptr, LCMPLX_MIN, LCMPLX_MAX, &endptr) != PARSE_EEND
        || stringToULong(&(p->iterations), endptr, ITERATIONS_MIN, ITERATIONS_MAX, &endptr, BASE_DEC) != PARSE_EEND
        || stringToUIntMax(&tempWidth, endptr, WIDTH_MIN, WIDTH_MAX, &endptr, BASE_DEC) != PARSE_EEND
        || stringToUIntMax(&tempHeight, endptr, HEIGHT_MIN, HEIGHT_MAX, &endptr, BASE_DEC) != PARSE_EEND
        || stringToULong(&tempColourScheme, endptr, 0UL, ULONG_MAX, &endptr, BASE_DEC) != PARSE_SUCCESS)
    {
        return 1;
    }

    if (tempPlotType != PLOT_JULIA && tempPlotType != PLOT_MANDELBROT)
        return 1;
    
    p->type = tempPlotType;
    p->width = tempWidth;
    p->height = tempHeight;

    p->output = OUTPUT_NONE;
    p->file = NULL;

    if (initialiseColourScheme(&p->colour, tempColourScheme))
        return 1;

    return 0;
}


#ifdef MP_PREC
int deserialisePlotCTXMP(PlotCTX *p, char *src)
{
    char *endptr = src;

    unsigned long int tempPlotType = 0UL;
    uintmax_t tempWidth = 0;
    uintmax_t tempHeight = 0;
    unsigned long int tempColourScheme = 0UL;

    if (stringToULong(&tempPlotType, endptr, 0, ULONG_MAX, &endptr, BASE_DEC) != PARSE_EEND
        || mpc_strtoc(p->minimum.mpc, endptr, &endptr, 10, MP_COMPLEX_RND) == -1
        || mpc_strtoc(p->maximum.mpc, endptr, &endptr, 10, MP_COMPLEX_RND) == -1
        || mpc_strtoc(p->c.mpc, endptr, &endptr, 10, MP_COMPLEX_RND) == -1
        || stringToULong(&(p->iterations), endptr, 0, ULONG_MAX, &endptr, BASE_DEC) != PARSE_EEND
        || stringToUIntMax(&tempWidth, endptr, WIDTH_MIN, WIDTH_MAX, &endptr, BASE_DEC) != PARSE_EEND
        || stringToUIntMax(&tempHeight, endptr, HEIGHT_MIN, HEIGHT_MAX, &endptr, BASE_DEC) != PARSE_EEND
        || stringToULong(&tempColourScheme, endptr, 0UL, ULONG_MAX, &endptr, BASE_DEC) != PARSE_SUCCESS)
    {
        return 1;
    }

    if (tempPlotType != PLOT_JULIA && tempPlotType != PLOT_MANDELBROT)
        return 1;

    p->type = tempPlotType;
    p->width = tempWidth;
    p->height = tempHeight;

    p->output = OUTPUT_NONE;
    p->file = NULL;

    if (initialiseColourScheme(&p->colour, tempColourScheme))
        return 1;

    return 0;
}
#endif


int sendAcknowledgement(int s)
{
    ssize_t bytes;
    char buffer[NETWORK_BUFFER_SIZE] = {'\0'};

    strncpy(buffer, ACK_RESPONSE, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    bytes = writeSocket(buffer, s, sizeof(buffer));

    if (bytes == 0)
    {
        return -2;
    }
    else if (bytes < 0)
    {
        return -1;
    }
    else if ((size_t) bytes != sizeof(buffer))
    {
        logMessage(ERROR, "Could not write full request to connection");
        return -1;
    }

    return 0;
}


int sendError(int s)
{
    ssize_t bytes;
    char buffer[NETWORK_BUFFER_SIZE] = {'\0'};

    strncpy(buffer, ERR_RESPONSE, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    bytes = writeSocket(buffer, s, sizeof(buffer));

    if (bytes == 0)
    {
        return -2;
    }
    else if (bytes < 0)
    {
        return -1;
    }
    else if ((size_t) bytes != sizeof(buffer))
    {
        logMessage(ERROR, "Could not write full request to connection");
        return -1;
    }

    return 0;
}


int readParameters(PlotCTX **p, int s)
{
    ssize_t bytes;
    char buffer[PARAMETERS_BUFFER_SIZE] = {'\0'};

    PrecisionMode precision;

    logMessage(DEBUG, "Reading precision mode");
    
    bytes = readSocket(buffer, s, sizeof(buffer));

    if (bytes <= 0 || (size_t) bytes != sizeof(buffer))
        return (bytes == 0) ? -2 : -1;

    logMessage(DEBUG, "Deserialising precision mode");

    #ifndef MP_PREC
    if (deserialisePrecision(&precision, buffer))
    #else
    if (deserialisePrecision(&precision, &mpSignificandSize, buffer))
    #endif
    {
        logMessage(ERROR, "Could not deserialise precision mode");
        return -1;
    }

    logMessage(DEBUG, "Reading plot parameters");

    memset(buffer, '\0', sizeof(buffer));

    bytes = readSocket(buffer, s, sizeof(buffer));

    if (bytes <= 0 || (size_t) bytes != sizeof(buffer))
        return (bytes == 0) ? -2 : -1;

    logMessage(DEBUG, "Creating plot parameters structure");

    *p = createPlotCTX(precision);

    if (!*p)
    {
        logMessage(ERROR, "Could not create plot parameters structure");
        return -1;
    }

    logMessage(DEBUG, "Deserialising plot parameters");

    switch((*p)->precision)
    {
        case STD_PRECISION:
            if (deserialisePlotCTX(*p, buffer))
            {
                logMessage(ERROR, "Could not deserialise plot parameters");
                freePlotCTX(*p);
                return -1;
            }
            break;
        case EXT_PRECISION:
            if (deserialisePlotCTXExt(*p, buffer))
            {
                logMessage(ERROR, "Could not deserialise plot parameters");
                freePlotCTX(*p);
                return -1;
            }
            break;

        #ifdef MP_PREC
        case MUL_PRECISION:
            if (deserialisePlotCTXMP(*p, buffer))
            {
                logMessage(ERROR, "Could not deserialise plot parameters");
                freePlotCTX(*p);
                return -1;
            }
            break;
        #endif

        default:
            logMessage(ERROR, "Unknown precision mode");
            freePlotCTX(*p);
            return -1;
    }

    return 0;
}


int sendParameters(int s, const PlotCTX *p)
{
    int ret;
    ssize_t bytes;
    char buffer[PARAMETERS_BUFFER_SIZE] = {'\0'};

    logMessage(DEBUG, "Serialising precision mode");

    #ifndef MP_PREC
    ret = serialisePrecision(buffer, sizeof(buffer), p->precision);
    #else
    ret = serialisePrecision(buffer, sizeof(buffer), p->precision, mpSignificandSize);
    #endif

    /* If truncated or error */
    if (ret < 0 || (size_t) ret >= sizeof(buffer))
    {
        logMessage(ERROR, "Could not serialise precision mode");
        return -1;
    }

    logMessage(DEBUG, "Sending precision mode");

    bytes = writeSocket(buffer, s, sizeof(buffer));

    if (bytes == 0)
    {
        return -2;
    }
    else if (bytes < 0)
    {
        return -1;
    }
    else if ((size_t) bytes != sizeof(buffer))
    {
        logMessage(ERROR, "Could not write full request to connection");
        return -1;
    }

    logMessage(DEBUG, "Serialising plot parameters");

    memset(buffer, '\0', sizeof(buffer));

    switch(p->precision)
    {
        case STD_PRECISION:
            ret = serialisePlotCTX(buffer, sizeof(buffer), p);
            break;
        case EXT_PRECISION:
            ret = serialisePlotCTXExt(buffer, sizeof(buffer), p);
            break;

        #ifdef MP_PREC
        case MUL_PRECISION:
            ret = serialisePlotCTXMP(buffer, sizeof(buffer), p);
            break;
        #endif

        default:
            logMessage(ERROR, "Invalid precision mode");
            return -1;
    }

    if (ret < 0 || (size_t) ret >= sizeof(buffer))
    {
        logMessage(ERROR, "Could not serialise plot context structure");
        return -1;
    }

    logMessage(DEBUG, "Sending plot parameters");

    bytes = writeSocket(buffer, s, sizeof(buffer));
    
    if (bytes == 0)
    {
        return -2;
    }
    else if (bytes < 0)
    {
        return -1;
    }
    else if ((size_t) bytes != sizeof(buffer))
    {
        logMessage(ERROR, "Could not write full request to connection");
        return -1;
    }
    
    return 0;
}


int requestRowNumber(size_t *n, int s, const PlotCTX *p)
{
    ssize_t ret;

    char buffer[NETWORK_BUFFER_SIZE] = {'\0'};

    uintmax_t tempUIntMax = 0;
    char *endptr;

    /* Send request to master for a new row number */
    strncpy(buffer, ROW_REQUEST, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    ret = writeSocket(buffer, s, sizeof(buffer));

    if (ret == 0)
    {
        return -2;
    }
    else if (ret < 0 || (size_t) ret != sizeof(buffer))
    {
        logMessage(ERROR, "Could not write to socket connection");
        return -1;
    }

    /* Read and parse response from master */
    memset(buffer, '\0', sizeof(buffer));
    ret = readSocket(buffer, s, sizeof(buffer));

    if (ret == 0)
    {
        return -2;
    }
    else if (ret < 0 || (size_t) ret != sizeof(buffer))
    {
        logMessage(ERROR, "Error reading from socket connection");
        return -1;
    }

    buffer[sizeof(buffer) - 1] = '\0';

    if (stringToUIntMax(&tempUIntMax, buffer, 0, p->height - 1, &endptr, BASE_DEC) != PARSE_SUCCESS)
    {
        logMessage(ERROR, "Error parsing row number \'%s\'", buffer);
        return -3;
    }

    *n = (size_t) tempUIntMax;

    return 0;
}


int sendRowData(int s, size_t rowNum, void *row, size_t n)
{
    ssize_t bytes;
    char buffer[NETWORK_BUFFER_SIZE] = {'\0'};

    int ret = sendRowNumber(s, rowNum);

    if (ret)
        return ret;

    bytes = writeSocket(row, s, n);

    if (bytes == 0)
    {
        return -2;
    }
    else if (bytes < 0 || (size_t) bytes != n)
    {
        logMessage(ERROR, "Could not write to socket connection");
        return -1;
    }

    /* Read acknowledgement from master */
    bytes = readSocket(buffer, s, sizeof(buffer));

    if (bytes == 0)
    {
        return -2;
    }
    else if (bytes < 0 || (size_t) bytes != sizeof(buffer))
    {
        logMessage(ERROR, "Error reading from socket connection");
        return -1;
    }

    buffer[sizeof(buffer) - 1] = '\0';

    if (strcmp(buffer, ACK_RESPONSE))
    {
        logMessage(ERROR, "Invalid acknowledgement from master");
        return -3;
    }

    return 0;
}


static int sendRowNumber(int s, size_t n)
{
    ssize_t ret;
    char buffer[NETWORK_BUFFER_SIZE] = {'\0'};

    snprintf(buffer, sizeof(buffer), "%zu", n);

    ret = writeSocket(buffer, s, sizeof(buffer));

    if (ret == 0)
    {
        return -2;
    }
    else if (ret < 0 || (size_t) ret != sizeof(buffer))
    {
        logMessage(ERROR, "Could not write to socket connection");
        return -1;
    }

    return 0;
}