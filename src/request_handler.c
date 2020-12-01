#include <complex.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
            logMessage(INFO, "Connection with peer closed");
            return 0;
        }
        else if (readBytes < 0)
        {
            if (errno == EINTR)
            {
                /* Read call interrupted - try again */
                continue;
            }
            else if (errno == ECONNRESET)
            {
                /* Connection closed */
                logMessage(INFO, "Connection with peer closed");
                return 0;
            }
            else
            {
                logMessage(ERROR, "Could not read from connection");
                return -1;
            }
        }
    }
    while (readBytes < 0 && errno == EINTR);

    return readBytes;
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

    if (initialiseColourScheme(&p->colour, tempColourScheme))
        return 1;

    return 0;
}
#endif


int readParameters(PlotCTX **p, int s)
{
    ssize_t bytes;
    char buffer[READ_BUFFER_SIZE];

    PrecisionMode precision;

    memset(buffer, '\0', sizeof(buffer));

    logMessage(DEBUG, "Reading precision mode");
    
    bytes = readSocket(buffer, s, sizeof(buffer));

    if (bytes <= 0)
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
    
    memset(buffer, '\0', sizeof(buffer));

    logMessage(DEBUG, "Reading plot parameters");

    bytes = readSocket(buffer, s, sizeof(buffer));

    if (bytes <= 0)
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
    char buffer[WRITE_BUFFER_SIZE];

    memset(buffer, '\0', sizeof(buffer));

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

    bytes = writeSocket(buffer, s, strlen(buffer) + 1);

    if (bytes == 0)
    {
        return -2;
    }
    else if (bytes < 0)
    {
        return -1;
    }
    else if ((size_t) bytes != strlen(buffer) + 1)
    {
        logMessage(ERROR, "Could not write full request to connection");
        return -1;
    }

    memset(buffer, '\0', sizeof(buffer));

    logMessage(DEBUG, "Serialising plot parameters");

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

    bytes = writeSocket(buffer, s, strlen(buffer) + 1);
    
    if (bytes == 0)
    {
        return -2;
    }
    else if (bytes < 0)
    {
        return -1;
    }
    else if ((size_t) bytes != strlen(buffer) + 1)
    {
        logMessage(ERROR, "Could not write full request to connection");
        return -1;
    }
    
    return 0;
}