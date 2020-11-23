#ifndef EXT_PRECISION_H
#define EXT_PRECISION_H


#include <complex.h>
#include <stddef.h>

#ifdef MP_PREC
#include <mpfr.h>
#include <mpc.h>
#endif


typedef enum PrecisionMode
{
    STD_PRECISION,
    EXT_PRECISION,

    #ifdef MP_PREC
    MUL_PRECISION
    #endif

} PrecisionMode;

typedef union ExtComplex
{
    complex c;
    long double complex lc;

    #ifdef MP_PREC
    mpc_t mpc;
    #endif

} ExtComplex;


extern const PrecisionMode PREC_MODE_MIN;
extern const PrecisionMode PREC_MODE_MAX;

#ifdef MP_PREC
extern const mpc_rnd_t MP_COMPLEX_RND;
extern const mpfr_rnd_t MP_REAL_RND;
extern const mpfr_rnd_t MP_IMAG_RND;


extern mpfr_prec_t mpSignificandSize;
#endif

extern PrecisionMode precision;


int getPrecisionString(char *dest, PrecisionMode prec, size_t n);


#endif