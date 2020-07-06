#ifndef EXT_PRECISION_H
#define EXT_PRECISION_H


#include <complex.h>

#include <gmp.h>
#include <mpc.h>


typedef enum PrecisionMode
{
    STD_PRECISION,
    EXT_PRECISION,
    ARB_PRECISION
} PrecisionMode;

typedef union ExtDouble
{
    double d;
    long double ld;
    mpf_t mpd;
} ExtDouble;

typedef union ExtComplex
{
    complex c;
    long double complex lc;
    mpc_t mpc;
} ExtComplex;


extern const mpfr_prec_t ARB_PRECISION_BITS;

extern const mpc_rnd_t ARB_CMPLX_ROUNDING;
extern const mpfr_rnd_t ARB_REAL_ROUNDING;
extern const mpfr_rnd_t ARB_IMAG_ROUNDING;


extern PrecisionMode precision;


#endif