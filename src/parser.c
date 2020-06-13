#include "parser.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "parameters.h"


enum ComplexPart
{
    COMPLEX_NONE,
    COMPLEX_REAL,
    COMPLEX_IMAGINARY
};


/* Symbols to denote the imaginary unit */
static const char IMAGINARY_UPPER = 'I';
static const char IMAGINARY_LOWER = 'i';


static int parseSign(const char *c, const char **endptr);
static int parseImaginaryUnit(const char *c, const char **endptr);
static size_t stripWhitespace(char *dest, const char *src, size_t n);


/* Convert string to unsigned long and handle errors */
int stringToULong(unsigned long int *x, const char *nptr, unsigned long int min, unsigned long int max,
                     const char **endptr, int base)
{
    char sign;

    if ((base < 2 && base != 0) || base > 36)
        return PARSER_EBASE;

    /* Get pointer to start of number */
    while (isspace(*nptr))
        nptr++;

    sign = *nptr;

    errno = 0;
    *x = strtoul(nptr, (char **) endptr, base);
    
    /* Conversion check */
    if (*endptr == nptr || errno == EINVAL)
        return PARSER_ERROR;
    
    /* Range checks */
    if (errno == ERANGE)
        return PARSER_ERANGE;
    else if (*x < min)
        return PARSER_EMIN;
    else if (*x > max)
        return PARSER_EMAX;
    else if (sign == '-' && *x != 0)
        return PARSER_EMIN;

    /* Successful but more characters in string */
    if (**endptr != '\0')
        return PARSER_EEND;

    return PARSER_NONE;
}


/* Convert string to uintmax_t and handle errors */
int stringToUIntMax(uintmax_t *x, const char *nptr, uintmax_t min, uintmax_t max, const char **endptr, int base)
{
    char sign;

    if ((base < 2 && base != 0) || base > 36)
        return PARSER_EBASE;

    /* Get pointer to start of number */
    while (isspace(*nptr))
        nptr++;

    sign = *nptr;

    errno = 0;
    *x = strtoumax(nptr, (char **) endptr, base);
    
    /* Conversion check */
    if (*endptr == nptr || errno == EINVAL)
        return PARSER_ERROR;
    
    /* Range checks */
    if (errno == ERANGE)
        return PARSER_ERANGE;
    else if (*x < min)
        return PARSER_EMIN;
    else if (*x > max)
        return PARSER_EMAX;
    else if (sign == '-' && *x != 0)
        return PARSER_EMIN;

    /* Successful but more characters in string */
    if (**endptr != '\0')
        return PARSER_EEND;

    return PARSER_NONE;
}


/* Convert string to double and handle errors */
int stringToDouble(double *x, const char *nptr, double min, double max, const char **endptr)
{
    errno = 0;
    *x = strtod(nptr, (char **) endptr);
    
    /* Conversion check */
    if (*endptr == nptr)
        return PARSER_ERROR;
    
    /* Range checks */
    if (errno == ERANGE)
        return PARSER_ERANGE;
    else if (*x < min)
        return PARSER_EMIN;
    else if (*x > max)
        return PARSER_EMAX;
    
    /* Successful but more characters in string */
    if (**endptr != '\0')
        return PARSER_EEND;

    return PARSER_NONE;
}


/* Parse a string as an imaginary or real double
 *
 * Where:
 *   - The format is that of a `double` type - meaning a decimal, additional 
 *     exponent part, and hexadecimal sequence are all valid inputs
 *   - Whitespace will be stripped
 *   - The operator can be '+' or '-'
 *   - It can be preceded by an optional '+' or '-' sign
 *   - An imaginary number must be followed by the imaginary unit
 */
int stringToImaginary(struct ComplexNumber *z, const char *nptr, double min, double max, const char **endptr)
{
    double x;
    int sign;
    enum ParserErrorCode parseError;

    /* 
     * Manually parsing the sign enables detection of a complex unit lacking in
     * a coefficient but having a '+'/'-' sign
     */
    if ((sign = parseSign(nptr, endptr)) == 0)
        sign = 1;
    /*
     * Because the sign has been manually parsed, error on a second sign, which
     * strtod() will not detect
     */
    nptr = *endptr;
    if (parseSign(nptr, endptr) != 0)
        return PARSER_ERROR;

    /* 
     * `(char **)` cast on `endptr` is to remove compiler warning due to bad
     * standard library function signatures
     */
    nptr = *endptr;
    parseError = stringToDouble(&x, nptr, min, max, (char **) endptr);

    if (parseError == PARSER_ERROR)
    {
        if (**endptr != IMAGINARY_UPPER && **endptr != IMAGINARY_LOWER)
            return COMPLEX_NONE;

        /* Failed conversion must be an imaginary unit without coefficient */
        x = 1.0;
    }
    else if (parseError != PARSER_NONE && parseError != PARSER_EEND)
    {
        return COMPLEX_NONE;
    }

    x *= sign;
    
    nptr = *endptr;
    switch (parseImaginaryUnit(nptr, endptr))
    {
        case COMPLEX_REAL:
            z->re = x;
            return COMPLEX_REAL;
        case COMPLEX_IMAGINARY:
            z->im = x;
            return COMPLEX_IMAGINARY;
        default:
            return COMPLEX_NONE;
    }
}


/* 
 * Parse a complex number string into a {real, imaginary} struct
 * 
 * Input must be of the form:
 *   "a + bi" or
 *   "bi + a"
 * 
 * Where each part, `a` and `bi`, is parsed according to stringToImaginary():
 *   - The operator can be '+' or '-'
 *   - `a` and `bi` can be preceded by an optional '+' or '-' sign (independant
 *     of the expression's operator)
 *   - There cannot be multiple real or imaginary parts (e.g. "a + b + ci" is
 *     invalid)
 *   - Either parts can be omitted - the missing part will be interpreted as 0.0
 */
int stringToComplex(struct ComplexNumber *z, const char *nptr, double min, double max, const char **endptr)
{
    char *buffer;
    size_t nptrSize = strlen(nptr) + 1;

    const char *nptr;
    _Bool reFlag = 0, imFlag = 0;
    int operator;

    if ((buffer = malloc(nptrSize)) == NULL)
        return PARSER_ERROR;

    stripWhitespace(buffer, nptr, nptrSize);

    nptr = buffer;
    *endptr = nptr;

    z->re = z->im = 0.0;

    /* Get first operand in complex number */
    switch (stringToImaginary(z, nptr, min, max, endptr))
    {
        case COMPLEX_REAL:
            reFlag = 1;
            break;
        case COMPLEX_IMAGINARY:
            imFlag = 1;
            break;
        default:
            free(buffer);
            return PARSER_ERROR;
    }

    if (**endptr == '\0')
    {
        free(buffer);
        return PARSER_NONE;
    }

    /* Get operator between the two parts */
    nptr = *endptr;
    if ((operator = parseSign(nptr, endptr)) == 0)
    {
        free(buffer);
        return PARSER_ERROR;
    }

    /* Get second operand in complex number */
    nptr = *endptr;
    switch (stringToImaginary(z, nptr, min, max, endptr))
    {
        case COMPLEX_REAL:
            reFlag = 1;
            z->re *= operator;
            break;
        case COMPLEX_IMAGINARY:
            imFlag = 1;
            z->im *= operator;
            break;
        default:
            free(buffer);
            return PARSER_ERROR;
    }

    if (**endptr != '\0')
        free(buffer);
        return PARSER_EEND;

    free(buffer);

    /* If both flags have not been set */
    if (!(reFlag && imFlag))
        return PARSER_ERROR;

    return PARSER_NONE;
}


/* Parse the sign of a number */
static int parseSign(const char *c, const char **endptr)
{
    *endptr = c;
    switch (*c)
    {
        case '+':
            ++(*endptr);
            return 1;
        case '-':
            ++(*endptr);
            return -1;
        default:
            return 0;
    }
}


/* Parse the imaginary unit, or lack thereof */
static int parseImaginaryUnit(const char *c, const char **endptr)
{
    *endptr = c;
    if (*c != IMAGINARY_UPPER && *c != IMAGINARY_LOWER)
        return COMPLEX_REAL;

    ++(*endptr);
    return COMPLEX_IMAGINARY;
}


/* 
 * Strip `src` of white-space then copy a maximum of `n` characters (including
 * the null terminator) into `dest` - return the length of `dest`
 */
static size_t stripWhitespace(char *dest, const char *src, size_t n)
{
    size_t i, j;

    for (i = 0, j = 0; src[i] != '\0' && j < n - 1; ++i)
    {
        if (!isspace(src[i]))
            dest[j++] = src[i];
    }

    dest[j] = '\0';

    /* Length of `dest` */
    return j;
}