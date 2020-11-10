#ifndef ARG_RANGES_H
#define ARG_RANGES_H


#include <complex.h>
#include <stddef.h>
#include <stdint.h>

#ifdef MP_PREC
#include <mpfr.h>
#include <mpc.h>
#endif


extern const complex COMPLEX_MIN;
extern const complex COMPLEX_MAX;
extern const long double complex COMPLEX_MIN_EXT;
extern const long double complex COMPLEX_MAX_EXT;

extern const complex C_MIN;
extern const complex C_MAX;
extern const long double complex C_MIN_EXT;
extern const long double complex C_MAX_EXT;

#ifdef MP_PREC
extern mpc_t C_MIN_MP;
extern mpc_t C_MAX_MP;
#endif

extern const double MAGNIFICATION_MIN;
extern const double MAGNIFICATION_MAX;
extern const long double MAGNIFICATION_MIN_EXT;
extern const long double MAGNIFICATION_MAX_EXT;

extern const unsigned long ITERATIONS_MIN;
extern const unsigned long ITERATIONS_MAX;

extern const size_t WIDTH_MIN;
extern const size_t WIDTH_MAX;
extern const size_t HEIGHT_MIN;
extern const size_t HEIGHT_MAX;

extern const uint16_t PORT_MIN;
extern const uint16_t PORT_MAX;

#ifdef MP_PREC
const mpfr_prec_t MP_BITS_DEFAULT;
const mpfr_prec_t MP_BITS_MIN;
const mpfr_prec_t MP_BITS_MAX;


void initialiseArgRangesMP(void);
void freeArgRangesMP(void);
#endif


#endif