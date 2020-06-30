#ifndef COLOUR_H
#define COLOUR_H


#include <complex.h>
#include <stddef.h>
#include <stdint.h>


#define COLOUR_STRING_LENGTH_MAX 32


enum EscapeStatus
{
    UNESCAPED,
    ESCAPED
};

enum ColourSchemeType
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
};

enum BitDepth
{
    BIT_DEPTH_ASCII = 0,
    BIT_DEPTH_1 = 1,
    BIT_DEPTH_8 = 8,
    BIT_DEPTH_24 = 24
};

struct ColourRGB
{
    uint8_t r, g, b;
};

struct ColourHSV
{
    long double h, s, v;
};

union ColourMapFunction
{
    char (*ascii) (long double n, enum EscapeStatus status);
    void (*monochrome) (char *byte, int offset, enum EscapeStatus status);
    uint8_t (*greyscale) (long double n, enum EscapeStatus status);
    void (*trueColour) (struct ColourRGB *rgb, long double n, enum EscapeStatus status);
};

struct ColourScheme
{
    enum ColourSchemeType colour;
    enum BitDepth depth;
    union ColourMapFunction mapColour;
};


extern const enum ColourSchemeType COLOUR_SCHEME_DEFAULT;
extern const enum ColourSchemeType COLOUR_SCHEME_MIN;
extern const enum ColourSchemeType COLOUR_SCHEME_MAX;


void initialiseColourScheme(struct ColourScheme *scheme, enum ColourSchemeType colour);
void mapColour(void *pixel, unsigned long n, long double complex z, int offset, unsigned long max, struct ColourScheme *scheme);

void getColourString(char *dest, enum ColourSchemeType colour, size_t n);


#endif