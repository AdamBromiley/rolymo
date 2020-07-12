#include "mandelbrot_parameters.h"


/* 
 * Maximum magnitude before a number is considered to have escaped, assuming the
 * calculation has already hit the maximum iteration count.
 * The number is mathematically defined as 2, however a larger number allows for
 * smoother colour mapping.
 */
const double ESCAPE_RADIUS = 256.0;
const long double ESCAPE_RADIUS_EXT = 256.0L;

#ifdef MP_PREC
const double ESCAPE_RADIUS_MP = 256.0;
#endif