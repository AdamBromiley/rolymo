#ifndef PARSER_H
#define PARSER_H


#include <stdint.h>

#include "parameters.h"


enum ParserErrorCode
{
    PARSER_NONE,
    PARSER_ERROR,
    PARSER_ERANGE,
    PARSER_EMIN,
    PARSER_EMAX,
    PARSER_EEND,
    PARSER_EBASE
};


int stringToULong(unsigned long int *x, const char *nptr, unsigned long int min, unsigned long int max,
                     const char **endptr, int base);
int stringToUIntMax(uintmax_t *x, const char *nptr, uintmax_t min, uintmax_t max, const char **endptr, int base);
int stringToDouble(double *x, const char *nptr, double min, double max, const char **endptr);
int stringToImaginary(struct ComplexNumber *z, const char *nptr, double min, double max, const char **endptr);
int stringToComplex(struct ComplexNumber *z, const char *nptr, double min, double max, const char **endptr);


#endif