#ifndef PROCESS_ARGS_H
#define PROCESS_ARGS_H


#include "parameters.h"

#include "percy/include/parser.h"

#include <complex.h>
#include <stdint.h>

#ifdef MP_PREC
#include <mpc.h>
#endif


#define IP_ADDR_STR_LEN_MAX ((4 * 3) + 3 + 1)


ParseErr uLongArg(unsigned long *x, char *arg, unsigned long min, unsigned long max);
ParseErr uIntMaxArg(uintmax_t *x, char *arg, uintmax_t min, uintmax_t max);

ParseErr floatArg(double *x, char *arg, double min, double max);
ParseErr floatArgExt(long double *x, char *arg, long double min, long double max);

ParseErr complexArg(complex *z, char *arg, complex min, complex max);
ParseErr complexArgExt(long double complex *z, char *arg, long double complex min, long double complex max);

ParseErr magArg(PlotCTX *p, char *arg, complex cMin, complex cMax, double mMin, double mMax);
ParseErr magArgExt(PlotCTX *p, char *arg, long double complex cMin, long double complex cMax, double mMin, double mMax);

#ifdef MP_PREC
ParseErr complexArgMP(mpc_t z, char *arg, mpc_t min, mpc_t max);
ParseErr magArgMP(PlotCTX *p, char *arg, mpc_t cMin, mpc_t cMax, long double mMin, long double mMax);
#endif

int validateIPAddress(char *addr);


#endif