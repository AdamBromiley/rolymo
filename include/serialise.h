#ifndef SERIALISE_H
#define SERIALISE_H


#include <stddef.h>

#include "ext_precision.h"
#include "parameters.h"

#ifdef MP_PREC
#include <mpfr.h>
#endif


#ifndef MP_PREC
int serialisePrecision(char *dest, size_t n, PrecisionMode prec);
#else
int serialisePrecision(char *dest, size_t n, PrecisionMode prec, mpfr_prec_t bits);
#endif

#ifndef MP_PREC
int deserialisePrecision(PrecisionMode *prec, char *src);
#else
int deserialisePrecision(PrecisionMode *prec, mpfr_prec_t *bits, char *src);
#endif

int serialisePlotCTX(char *dest, size_t n, const PlotCTX *p);
int serialisePlotCTXExt(char *dest, size_t n, const PlotCTX *p);

#ifdef MP_PREC
int serialisePlotCTXMP(char *dest, size_t n, const PlotCTX *p);
#endif

int deserialisePlotCTX(PlotCTX *p, char *src);
int deserialisePlotCTXExt(PlotCTX *p, char *src);

#ifdef MP_PREC
int deserialisePlotCTXMP(PlotCTX *p, char *src);
#endif


#endif