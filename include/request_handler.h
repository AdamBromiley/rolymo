#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H


#include <stddef.h>

#include <sys/types.h>

#include "ext_precision.h"
#include "parameters.h"

#ifdef MP_PREC
#include <mpfr.h>
#endif


#define PARAMETERS_BUFFER_SIZE 4096

#define NETWORK_BUFFER_SIZE 16


extern const char *ROW_REQUEST;
extern const char *ROW_RESPONSE;
extern const char *ACK_RESPONSE;
extern const char *ERR_RESPONSE;


ssize_t writeSocket(const void *src, int s, size_t n);
ssize_t readSocket(void *dest, int s, size_t n);

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

int sendAcknowledgement(int s);
int sendError(int s);

int readParameters(PlotCTX **p, int s);
int sendParameters(int s, const PlotCTX *p);

int requestRowNumber(size_t *n, int s, const PlotCTX *p);
int sendRowData(int s, void *row, size_t n);


#endif