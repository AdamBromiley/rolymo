#ifndef COLOUR_H
#define COLOUR_H


#include <complex.h>
#include <stddef.h>
#include <stdint.h>


#define COLOUR_STRING_LENGTH_MAX 32


typedef enum EscapeStatus
{
    UNESCAPED,
    ESCAPED
} EscapeStatus;

typedef enum ColourSchemeType
{
    COLOUR_SCHEME_TYPE_DEFAULT,
    COLOUR_SCHEME_TYPE_ASCII,
    COLOUR_SCHEME_TYPE_BLACK_WHITE,
    COLOUR_SCHEME_TYPE_WHITE_BLACK,
    COLOUR_SCHEME_TYPE_GREYSCALE,
    COLOUR_SCHEME_TYPE_RAINBOW,
    COLOUR_SCHEME_TYPE_RAINBOW_VIBRANT,
    COLOUR_SCHEME_TYPE_RED_WHITE,
    COLOUR_SCHEME_TYPE_FIRE,
    COLOUR_SCHEME_TYPE_RED_HOT,
    COLOUR_SCHEME_TYPE_MATRIX
} ColourSchemeType;

typedef enum BitDepth
{
    BIT_DEPTH_ASCII = 0,
    BIT_DEPTH_1 = 1,
    BIT_DEPTH_8 = 8,
    BIT_DEPTH_24 = 24
} BitDepth;

typedef struct ColourRGB
{
    uint8_t r, g, b;
} RGB;

typedef struct ColourHSV
{
    double h, s, v;
} HSV;

typedef union ColourMapFunction
{
    char (*ascii) (double n, EscapeStatus status);
    void (*monochrome) (char *byte, int offset, EscapeStatus status);
    uint8_t (*greyscale) (double n, EscapeStatus status);
    void (*trueColour) (RGB *rgb, double n, EscapeStatus status);
} ColourMapFunction;

typedef struct ColourScheme
{
    ColourSchemeType scheme;
    BitDepth depth;
    ColourMapFunction mapColour;
} ColourScheme;


extern const ColourSchemeType COLOUR_SCHEME_DEFAULT;
extern const ColourSchemeType COLOUR_SCHEME_MIN;
extern const ColourSchemeType COLOUR_SCHEME_MAX;


void initialiseColourScheme(ColourScheme *scheme, ColourSchemeType colour);
void mapColour(void *pixel, unsigned long n, complex z, int offset, unsigned long max, ColourScheme *scheme);
void mapColourExt(void *pixel, unsigned long n, long double complex z, int offset, unsigned long max,
                     ColourScheme *scheme);

void getColourString(char *dest, ColourSchemeType colour, size_t n);


#endif