#include <complex.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>

#include "getopt_error.h"

#ifdef MP_PREC
#include <mpfr.h>
#include <mpc.h>
#endif


/* Precision of floating points in output */
const int FLT_PRINTF_PREC = 3;


/* Option character for error messages */
char opt = '\0';
char *programName = "";


/* Convert custom getopt error code to message */
void getoptErrorMessage(OptErr error, const char *longOpt)
{
    switch (error)
    {
        case OPT_NONE:
            break;
        case OPT_ERROR:
            fprintf(stderr, "%s: Unknown error when reading command-line options\n", programName);
            break;
        case OPT_EOPT:
            if (opt == '\0' && longOpt != NULL)
                fprintf(stderr, "%s: Invalid option: \'%s\'\n", programName, longOpt);
            else if (opt != '\0')
                fprintf(stderr, "%s: Invalid option: \'-%c\'\n", programName, opt);
            break;
        case OPT_ENOARG:
            fprintf(stderr, "%s: -%c: Option argument required\n", programName, opt);
            break;
        case OPT_EARG:
            fprintf(stderr, "%s: -%c: Failed to parse argument\n", programName, opt);
            break;
        case OPT_EMANY:
            fprintf(stderr, "%s: -%c: Option can only appear once\n", programName, opt);
            break;
        case OPT_EARGC_LOW:
            fprintf(stderr, "%s: Too few arguments supplied\n", programName);
            break;
        case OPT_EARGC_HIGH:
            fprintf(stderr, "%s: Too many arguments supplied\n", programName);
            break;
        default:
            break;
    }

    fprintf(stderr, "Try \'%s --help\' for more information\n", programName);
}


void uLongArgRangeErrorMessage(unsigned long min, unsigned long max)
{
    fprintf(stderr, "%s: -%c: Argument out of range, it must be between %lu and %lu\n", programName, opt, min, max);
}


void uIntMaxArgRangeErrorMessage(uintmax_t min, uintmax_t max)
{
    fprintf(stderr, "%s: -%c: Argument out of range, it must be between %" PRIuMAX " and %" PRIuMAX "\n",
            programName, opt, min, max);
}


void floatArgRangeErrorMessage(double min, double max)
{
    fprintf(stderr, "%s: -%c: Argument out of range, it must be between %.*g and %.*g\n", 
            programName, opt, FLT_PRINTF_PREC, min, FLT_PRINTF_PREC, max);
}


void floatArgRangeErrorMessageExt(long double min, long double max)
{
    fprintf(stderr, "%s: -%c: Argument out of range, it must be between %.*Lg and %.*Lg\n", 
            programName, opt, FLT_PRINTF_PREC, min, FLT_PRINTF_PREC, max);
}


void complexArgRangeErrorMessage(complex min, complex max)
{
    fprintf(stderr, "%s: -%c: Argument out of range, it must be between %.*g + %.*gi and %.*g + %.*gi\n", 
            programName, opt,
            FLT_PRINTF_PREC, creal(min), FLT_PRINTF_PREC, cimag(min),
            FLT_PRINTF_PREC, creal(max), FLT_PRINTF_PREC, cimag(max));
}


void complexArgRangeErrorMessageExt(long double complex min, long double complex max)
{
    fprintf(stderr, "%s: -%c: Argument out of range, it must be between %.*Lg + %.*Lgi and %.*Lg + %.*Lgi\n", 
            programName, opt,
            FLT_PRINTF_PREC, creall(min), FLT_PRINTF_PREC, cimagl(min),
            FLT_PRINTF_PREC, creall(max), FLT_PRINTF_PREC, cimagl(max));
}


#ifdef MP_PREC
void complexArgRangeErrorMessageMP(mpc_t min, mpc_t max)
{
    mpfr_fprintf(stderr, "%s: -%c: Argument out of range, it must be between %.*Rg + %.*Rgi and %.*Rg + %.*Rgi",
                 programName, opt,
                 FLT_PRINTF_PREC, mpc_realref(min), FLT_PRINTF_PREC, mpc_imagref(min),
                 FLT_PRINTF_PREC, mpc_realref(max), FLT_PRINTF_PREC, mpc_imagref(max));
}
#endif