#ifndef GETOPT_ERROR_H
#define GETOPT_ERROR_H


#include <complex.h>
#include <stdint.h>

#ifdef MP_PREC
#include <mpc.h>
#endif


typedef enum GetoptError
{
    OPT_NONE,
    OPT_ERROR,
    OPT_EOPT,
    OPT_ENOARG,
    OPT_EARG,
    OPT_EMANY,
    OPT_EARGC_LOW,
    OPT_EARGC_HIGH
} OptErr;


/* Precision of floating points in output */
extern const int FLT_PRINTF_PREC;


/* Option character for error messages */
extern char opt;
extern char *programName;


void getoptErrorMessage(OptErr error, const char *longOpt);

void uLongArgRangeErrorMessage(unsigned long min, unsigned long max);
void uIntMaxArgRangeErrorMessage(uintmax_t min, uintmax_t max);
void floatArgRangeErrorMessage(double min, double max);
void floatArgRangeErrorMessageExt(long double min, long double max);
void complexArgRangeErrorMessage(complex min, complex max);
void complexArgRangeErrorMessageExt(long double complex min, long double complex max);

#ifdef MP_PREC
void complexArgRangeErrorMessageMP(mpc_t min, mpc_t max);
#endif

#endif