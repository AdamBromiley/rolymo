#include "arg_ranges.h"

#include <float.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#ifdef MP_PREC
#include <mpfr.h>
#endif


/* Range of permissible complex numbers */
const complex COMPLEX_MIN = -(DBL_MAX) - (DBL_MAX) * I;
const complex COMPLEX_MAX = (DBL_MAX) + (DBL_MAX) * I;

/* Range of permissible complex numbers (extended-precision) */
const long double complex COMPLEX_MIN_EXT = -(LDBL_MAX) - (LDBL_MAX) * I;
const long double complex COMPLEX_MAX_EXT = (LDBL_MAX) + (LDBL_MAX) * I;

/* Range of permissible constant values */
const complex C_MIN = -2.0 - 2.0 * I;
const complex C_MAX = 2.0 + 2.0 * I;

/* Range of permissible constant values (extended-precision) */
const long double complex C_MIN_EXT = -2.0L - 2.0L * I;
const long double complex C_MAX_EXT = 2.0L + 2.0L * I;

/* Range of permissible magnification values */
const double MAGNIFICATION_MIN = -(DBL_MAX);
const double MAGNIFICATION_MAX = DBL_MAX;

/* Range of permissible magnification values (extended-precision) */
const long double MAGNIFICATION_MIN_EXT = -(LDBL_MAX);
const long double MAGNIFICATION_MAX_EXT = LDBL_MAX;

/* Range of permissible iteration counts */
const unsigned long ITERATIONS_MIN = 0UL;
const unsigned long ITERATIONS_MAX = ULONG_MAX;

/* Range of permissible dimensions */
const size_t WIDTH_MIN = 1;
const size_t WIDTH_MAX = SIZE_MAX;
const size_t HEIGHT_MIN = 1;
const size_t HEIGHT_MAX = SIZE_MAX;

#ifdef MP_PREC
/* Range of permissible precisions (arbitrary-precision) */
const mpfr_prec_t MP_BITS_DEFAULT = 128;
const mpfr_prec_t MP_BITS_MIN = 1;
const mpfr_prec_t MP_BITS_MAX = 16384;
#endif