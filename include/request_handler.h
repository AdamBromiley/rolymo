#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H


#include <stddef.h>

#include <sys/types.h>

#ifdef MP_PREC
#include <mpfr.h>
#endif

#include "ext_precision.h"
#include "parameters.h"


ssize_t writeSocket(const void *src, int s, size_t n);
ssize_t readSocket(void *dest, int s, size_t n);

#ifdef MP_PREC
int serialisePrecision(char *dest, size_t n, PrecisionMode prec, mpfr_t bits);
#else
int serialisePrecision(char *dest, size_t n, PrecisionMode prec);
#endif

int serialisePlotCTX(char *dest, size_t n, const PlotCTX *p);
int deserialisePlotCTX(PlotCTX *p, char *src, size_t n);
int readParameters(int s, PlotCTX *p);
int sendParameters(int s, const PlotCTX *p);


#endif