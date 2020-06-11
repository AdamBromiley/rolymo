#ifndef PARSER_H
#define PARSER_H


#include <stdint.h>


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


int stringToDouble(char *string, double *x, double xMin, double xMax);
int stringToULong(const char *string, unsigned long int *x, unsigned long int xMin, unsigned long int xMax, int base);
int stringToUIntMax(const char *string, uintmax_t *x, uintmax_t xMin, uintmax_t xMax, int base);


#endif