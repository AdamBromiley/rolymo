#include "request_handler.h"

#include <complex.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>

#ifdef MP_PREC
#include <mpfr.h>
#endif

#include "percy/include/parser.h"

#include "arg_ranges.h"
#include "colour.h"
#include "ext_precision.h"
#include "parameters.h"


#define REQUEST_SIZE_MAX 1024

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


ssize_t writeSocket(const void *src, int s, size_t n)
{
    ssize_t sentBytes = 0;

    const char *srctmp = src;

    do
    {
        ssize_t ret;

        errno = 0;
        ret = send(s, srctmp + sentBytes, n, 0);

        if (ret < 0)
        {
            if (errno == EINTR) /* Message too large to be sent at once */
                continue;
            else if (errno == ECONNRESET) /* Connection closed */
                return 0;
            else
                return -1;
        }

        sentBytes += ret;
        n -= (size_t) ret;
    }
    while (n > 0);

    return sentBytes;
}


/* Read input stream on socket */
ssize_t readSocket(void *dest, int s, size_t n)
{
    ssize_t readBytes;

    do
    {
        errno = 0;
        readBytes = recv(s, dest, n, 0);

        if (readBytes == 0)
        {
            /* Shutdown request */
            return 0;
        }
        else if (readBytes < 0)
        {
            /* Forced shutdown request */
            if (errno == ECONNRESET)
                return 0;
            
            /* If not interrupted */
            if (errno != EINTR)
                return -1;
        }
    }
    while (readBytes < 0 && errno == EINTR);

    return readBytes;
}


#ifdef MP_PREC
int serialisePrecision(char *dest, size_t n, PrecisionMode prec, mpfr_t bits)
{
    return snprintf(dest, n, "%u %zu", (int) prec, (size_t) bits);
}
#else
int serialisePrecision(char *dest, size_t n, PrecisionMode prec)
{
    return snprintf(dest, n, "%u", (int) prec);
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


#ifdef UNUSED
int deserialisePlotCTX(PlotCTX *p, char *src, size_t n)
{
    char *endptr = src;

    stringToULong(&(p->type), endptr, 0, ULONG_MAX, &endptr, 10);
    stringToComplex(&(p->minimum.c), endptr, CMPLX_MIN, CMPLX_MAX, &endptr);
    stringToComplex(&(p->maximum.c), endptr, CMPLX_MIN, CMPLX_MAX, &endptr);
    stringToComplex(&(p->c.c), endptr, CMPLX_MIN, CMPLX_MAX, &endptr);
    stringToULong(&(p->iterations), endptr, 0, ULONG_MAX, &endptr, 10);
    stringToUIntMax(&(p->width), endptr, 0, SIZE_MAX, &endptr, 10);
    stringToUIntMax(&(p->height), endptr, 0, SIZE_MAX, &endptr, 10);
    stringToULong(&(p->colour.scheme), endptr, 0, ULONG_MAX, &endptr, 10);

    return 0;
}
#endif


int readParameters(int s, PlotCTX *p)
{
    ssize_t bytes = readSocket(&precision, s, sizeof(precision));

    if (bytes == 0)
        return -2;
    else if (bytes != sizeof(precision))
        return -1;
    
    bytes = readSocket(p, s, sizeof(*p));

    if (bytes == 0)
        return -2;
    else if (bytes != sizeof(*p))
        return -1;

    initialiseColourScheme(&p->colour, COLOUR_SCHEME_DEFAULT);

    return 0;
}


#ifdef UNUSED
int readParameters(PlotCTX *p, int s)
{
    char response[REQUEST_SIZE_MAX];
    ssize_t responseSize = readSocket(response, s, sizeof(response));

    if (responseSize <= 0)
        return 1;
    
    deserialisePlotCTX(p, response, responseSize);

    return 0;
}
#endif


int sendParameters(int s, const PlotCTX *p)
{
    ssize_t bytes = writeSocket(&precision, s, sizeof(precision));

    if (bytes == 0)
        return -2;
    else if (bytes != sizeof(precision))
        return -1;
    
    bytes = writeSocket(p, s, sizeof(*p));

    if (bytes == 0)
        return -2;
    else if (bytes != sizeof(*p))
        return -1;

    return 0;
}


#ifdef UNUSED
int sendParameters(int s, const PlotCTX *p)
{
    char request[REQUEST_SIZE_MAX];
    size_t requestSize = 0;

    char *endptr = request;

    ssize_t wroteBytes;

    #ifdef MP_PREC
    int ret = serialisePrecision(request, sizeof(request), precision, mpSignificandSize);
    #else
    int ret = serialisePrecision(request, sizeof(request), precision);
    #endif

    /* Allow buffer space for separator and null-terminator */
    if (ret >= sizeof(request) - 2 || ret < 0)
        return -1;

    requestSize += (size_t) ret;
    endptr += ret;

    *endptr = ' ';
    ++requestSize;
    ++endptr;

    ret = serialisePlotCTX(endptr, sizeof(request) - requestSize, p);

    if (ret >= sizeof(request) - 2 || ret < 0)
        return -1;
    
    requestSize += (size_t) ret;

    wroteBytes = writeSocket(request, s, requestSize);
    
    if (wroteBytes == 0)
        return -2;
    else if (wroteBytes != requestSize)
        return -1;
    else
        return 0;
}
#endif