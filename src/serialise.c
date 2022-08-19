#include <complex.h>
#include <float.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "percy/include/parser.h"

#include "serialise.h"

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
