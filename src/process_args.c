#include <complex.h>
#include <ctype.h>
#include <math.h>
#include <stdint.h>

#include "percy/include/parser.h"

#include "process_args.h"

#include "getopt_error.h"
#include "parameters.h"

#ifdef MP_PREC
#include <mpfr.h>
#include <mpc.h>
#endif


/* Wrapper for stringToULong() */
ParseErr uLongArg(unsigned long *x, char *arg, unsigned long min, unsigned long max)
{
    char *endptr;
    ParseErr argError = stringToULong(x, arg, min, max, &endptr, BASE_DEC);

    if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        uLongArgRangeErrorMessage(min, max);
        return PARSE_ERANGE;
    }
    else if (argError != PARSE_SUCCESS)
    {
        return PARSE_EERR;
    }

    return PARSE_SUCCESS;
}


/* Wrapper for stringToUIntMax() */
ParseErr uIntMaxArg(uintmax_t *x, char *arg, uintmax_t min, uintmax_t max)
{
    char *endptr;
    ParseErr argError = stringToUIntMax(x, arg, min, max, &endptr, BASE_DEC);

    if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        uIntMaxArgRangeErrorMessage(min, max);
        return PARSE_ERANGE;
    }
    else if (argError != PARSE_SUCCESS)
    {
        return PARSE_EERR;
    }

    return PARSE_SUCCESS;
}


/* Wrapper for stringToDouble() */
ParseErr floatArg(double *x, char *arg, double min, double max)
{
    char *endptr;
    ParseErr argError = stringToDouble(x, arg, min, max, &endptr);

    if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        floatArgRangeErrorMessage(min, max);
        return PARSE_ERANGE;
    }
    else if (argError != PARSE_SUCCESS)
    {
        return PARSE_EERR;
    }

    return PARSE_SUCCESS;
}


/* Wrapper for stringToDoubleL() */
ParseErr floatArgExt(long double *x, char *arg, long double min, long double max)
{
    char *endptr;
    ParseErr argError = stringToDoubleL(x, arg, min, max, &endptr);

    if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        floatArgRangeErrorMessageExt(min, max);
        return PARSE_ERANGE;
    }
    else if (argError != PARSE_SUCCESS)
    {
        return PARSE_EERR;
    }

    return PARSE_SUCCESS;
}


/* Wrapper for stringToComplex() */
ParseErr complexArg(complex *z, char *arg, complex min, complex max)
{
    char *endptr;
    ParseErr argError = stringToComplex(z, arg, min, max, &endptr);

    if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        complexArgRangeErrorMessage(min, max);
        return PARSE_ERANGE;
    }
    else if (argError != PARSE_SUCCESS)
    {
        return PARSE_EERR;
    }

    return PARSE_SUCCESS;
}


/* Wrapper for stringToComplexL() */
ParseErr complexArgExt(long double complex *z, char *arg, long double complex min, long double complex max)
{
    char *endptr;
    ParseErr argError = stringToComplexL(z, arg, min, max, &endptr);

    if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        complexArgRangeErrorMessageExt(min, max);
        return PARSE_ERANGE;
    }
    else if (argError != PARSE_SUCCESS)
    {
        return PARSE_EERR;
    }

    return PARSE_SUCCESS;
}


ParseErr magArg(PlotCTX *p, char *arg, complex cMin, complex cMax, double mMin, double mMax)
{
    complex range, centre;
    double magnification;

    char *endptr;
    ParseErr argError = stringToComplex(&centre, arg, cMin, cMax, &endptr);

    if (argError == PARSE_SUCCESS)
    {
        /* Magnification not explicitly mentioned - default to 1.0 */
        magnification = 1.0;
    }
    else if (argError == PARSE_EEND)
    {
        /* Check for comma separator */
        while (isspace(*endptr))
            ++endptr;

        if (*endptr != ',')
            return PARSE_EFORM;

        ++endptr;

        /* Get magnification argument */
        argError = floatArg(&magnification, endptr, mMin, mMax);

        if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
        {
            floatArgRangeErrorMessage(mMin, mMax);
            return PARSE_ERANGE;
        }
        else if (argError != PARSE_SUCCESS)
        {
            return PARSE_EERR;
        }
    }
    else if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        complexArgRangeErrorMessage(cMin, cMax);
        return PARSE_ERANGE;
    }
    else
    {
        return PARSE_EFORM;
    }

    /* Convert centrepoint and magnification to range */
    range = p->maximum.c - p->minimum.c;
    
    p->minimum.c = centre - 0.5 * range * pow(0.9, magnification - 1.0);
    p->maximum.c = centre + 0.5 * range * pow(0.9, magnification - 1.0);

    return PARSE_SUCCESS;
}


ParseErr magArgExt(PlotCTX *p, char *arg, long double complex cMin, long double complex cMax, double mMin, double mMax)
{
    long double complex range, centre;
    double magnification;

    char *endptr;
    ParseErr argError = stringToComplexL(&centre, arg, cMin, cMax, &endptr);

    if (argError == PARSE_SUCCESS)
    {
        /* Magnification not explicitly mentioned - default to 1 */
        magnification = 1.0;
    }
    else if (argError == PARSE_EEND)
    {
        /* Check for comma separator */
        while (isspace(*endptr))
            ++endptr;

        if (*endptr != ',')
            return PARSE_EFORM;

        ++endptr;

        /* Get magnification argument */
        argError = floatArg(&magnification, endptr, mMin, mMax);

        if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
        {
            floatArgRangeErrorMessageExt(mMin, mMax);
            return PARSE_ERANGE;
        }
        else if (argError != PARSE_SUCCESS)
        {
            return PARSE_EERR;
        }
    }
    else if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        complexArgRangeErrorMessageExt(cMin, cMax);
        return PARSE_ERANGE;
    }
    else
    {
        return PARSE_EFORM;
    }

    /* Convert centrepoint and magnification to range */
    range = p->maximum.lc - p->minimum.lc;

    p->minimum.lc = centre - 0.5L * range * powl(0.9L, magnification - 1.0L);
    p->maximum.lc = centre + 0.5L * range * powl(0.9L, magnification - 1.0L);

    return PARSE_SUCCESS;
}


#ifdef MP_PREC
/* Wrapper for stringToComplexMPC() */
ParseErr complexArgMP(mpc_t z, char *arg, mpc_t min, mpc_t max)
{
    const int BASE = 0;

    char *endptr;
    ParseErr argError = stringToComplexMPC(z, arg, min, max, &endptr, BASE, mpSignificandSize, MP_COMPLEX_RND);

    if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        complexArgRangeErrorMessageMP(min, max);
        return PARSE_ERANGE;
    }
    else if (argError != PARSE_SUCCESS)
    {
        return PARSE_EERR;
    }

    return PARSE_SUCCESS;
}


ParseErr magArgMP(PlotCTX *p, char *arg, mpc_t cMin, mpc_t cMax, long double mMin, long double mMax)
{
    const int BASE = 0;

    mpc_t range, centre;
    long double magnification;

    mpfr_t tmp;

    char *endptr;
    ParseErr argError;
    
    mpc_init2(centre, mpSignificandSize);
    argError = stringToComplexMPC(centre, arg, cMin, cMax, &endptr, BASE, mpSignificandSize, MP_COMPLEX_RND);

    if (argError == PARSE_SUCCESS)
    {
        /* Magnification not explicitly mentioned - default to 1 */
        magnification = 1.0L;
    }
    else if (argError == PARSE_EEND)
    {
        /* Check for comma separator */
        while (isspace(*endptr))
            ++endptr;

        if (*endptr != ',')
        {
            mpc_clear(centre);
            return PARSE_EFORM;
        }

        ++endptr;

        /* Get magnification argument */
        argError = floatArgExt(&magnification, endptr, mMin, mMax);

        if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
        {
            mpc_clear(centre);
            floatArgRangeErrorMessageExt(mMin, mMax);
            return PARSE_ERANGE;
        }
        else if (argError != PARSE_SUCCESS)
        {
            mpc_clear(centre);
            return PARSE_EERR;
        }
    }
    else if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        mpc_clear(centre);
        complexArgRangeErrorMessageMP(cMin, cMax);
        return PARSE_ERANGE;
    }
    else
    {
        mpc_clear(centre);
        return PARSE_EFORM;
    }

    mpc_init2(range, mpSignificandSize);

    /* Convert centrepoint and magnification to range */
    mpc_sub(range, p->maximum.mpc, p->minimum.mpc, MP_COMPLEX_RND);

    mpfr_init2(tmp, mpSignificandSize);
    mpfr_set_ld(tmp, 0.5L * powl(0.9L, magnification - 1.0L), MP_REAL_RND);

    mpc_mul_fr(range, range, tmp, MP_COMPLEX_RND);

    mpfr_clear(tmp);

    mpc_sub(p->minimum.mpc, centre, range, MP_COMPLEX_RND);
    mpc_add(p->maximum.mpc, centre, range, MP_COMPLEX_RND);

    mpc_clear(centre);
    mpc_clear(range);

    return PARSE_SUCCESS;
}
#endif


/* Check if dotted quad IP address is valid */
int validateIPAddress(char *addr)
{
    const unsigned long IP_ADDR_BYTE_MIN = 0;
    const unsigned long IP_ADDR_BYTE_MAX = 255;
    const char IP_ADDR_BYTE_SEPARATOR = '.';

    char *nptr = addr;

    for (int i = 0; i < 4; ++i)
    {
        unsigned long x;
        char *endptr;
        
        ParseErr err = stringToULong(&x, nptr, IP_ADDR_BYTE_MIN, IP_ADDR_BYTE_MAX, &endptr, BASE_DEC);

        if ((i < 3 && (err != PARSE_EEND || *endptr != IP_ADDR_BYTE_SEPARATOR))
            || (i == 3 && err != PARSE_SUCCESS))
            return 1;

        nptr = endptr + 1;
    }

    return 0;
}