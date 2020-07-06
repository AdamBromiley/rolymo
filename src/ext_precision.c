#include "ext_precision.h"

#include <mpfr.h>
#include <mpc.h>


/* Number of bits for the significand of arbitrary-precision numbers */
const mpfr_prec_t ARB_PRECISION_BITS = 256;

/* Rounding characteristic of MPC and MPFR operations (round both parts towards zero) */
const mpc_rnd_t ARB_CMPLX_ROUNDING = MPC_RNDZZ;
const mpfr_rnd_t ARB_REAL_ROUNDING = MPFR_RNDZ;
const mpfr_rnd_t ARB_IMAG_ROUNDING = MPFR_RNDZ;


/*
 * Default mode of operation is with the standard `double` and `double complex`
 * types.
 * 
 * Extended-precision mode enables the use of `long double` and
 * `long double complex` data types.
 * 
 * Arbitrary precision mode makes use of the GMP library for floating-points
 * (`mpf_t`), and the MPC library for complex types (`mpc_t`). The reason
 * `mpfr_t` (part of the MPFR library) is not used for arbitrary-precision
 * floating-points is purely for micro-optimisation at the sacrifice of accurate
 * rounding.
 */
PrecisionMode precision = STD_PRECISION;