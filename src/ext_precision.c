#include "ext_precision.h"

#include <stddef.h>
#include <string.h>

#ifdef MP_PREC
#include <mpfr.h>


/* Rounding characteristic of MPC and MPFR operations (round towards zero) */
const mpc_rnd_t MP_COMPLEX_RND = MPC_RNDZZ;
const mpfr_rnd_t MP_REAL_RND = MPFR_RNDZ;
const mpfr_rnd_t MP_IMAG_RND = MPFR_RNDZ;


/* Number of bits for the significand of multiple-precision numbers */
mpfr_prec_t mpSignificandSize = 128;
#endif


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


int getPrecisionString(char *dest, PrecisionMode prec, size_t n)
{
    const char *precStr;

    switch (prec)
    {
        case STD_PRECISION:
            precStr = "STANDARD";
            break;
        case EXT_PRECISION:
            precStr = "EXTENDED";
            break;
        
        #ifdef MP_PREC
        case MUL_PRECISION:
            precStr = "MULTIPLE";
            break;
        #endif
        
        default:
            return 1;
    }

    strncpy(dest, precStr, n);
    dest[n - 1] = '\0';

    return 0;
}