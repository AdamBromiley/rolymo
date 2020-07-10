#include "ext_precision.h"

#include <mpfr.h>
#include <mpc.h>


/* Rounding characteristic of MPC and MPFR operations (round towards zero) */
const mpc_rnd_t MP_COMPLEX_RND = MPC_RNDZZ;
const mpfr_rnd_t MP_REAL_RND = MPFR_RNDZ;
const mpfr_rnd_t MP_IMAG_RND = MPFR_RNDZ;


/*
 * Default mode of operation is with the standard `double` and `double complex`
 * types.
 * 
 * Extended-precision mode enables the use of `long double` and
 * `long double complex` data types.
 * 
 * MPitrary precision mode makes use of the GMP library for floating-points
 * (`mpfr_t`), and the MPC library for complex types (`mpc_t`).
 */
PrecisionMode precision = STD_PRECISION;

/* Number of bits for the significand of multi-precision numbers */
mpfr_prec_t mpSignificandSize = 128;