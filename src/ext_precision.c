#include "ext_precision.h"

#include <stdbool.h>


/*
 * Default mode of operation is with the standard `double` and `double complex`
 * types. Extended-precision mode enables the use of `long double` and
 * `long double complex` data types.
 */
bool extPrecision = false;