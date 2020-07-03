#ifndef EXT_PRECISION_H
#define EXT_PRECISION_H


#include <complex.h>
#include <stdbool.h>


typedef union ExtDouble
{
    double d;
    long double ld;
} ExtDouble;

typedef union ExtComplex
{
    complex c;
    long double complex lc;
} ExtComplex;


extern bool extPrecision;


#endif