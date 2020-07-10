#ifndef EXT_PRECISION_H
#define EXT_PRECISION_H


#include <complex.h>

#include <gmp.h>
#include <mpc.h>


typedef enum PrecisionMode
{
    STD_PRECISION,
    EXT_PRECISION,
    MUL_PRECISION
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


extern const mpc_rnd_t MP_COMPLEX_RND;
extern const mpfr_rnd_t MP_REAL_RND;
extern const mpfr_rnd_t MP_IMAG_RND;


extern PrecisionMode precision;

extern mpfr_prec_t mpSignificandSize;


#endif