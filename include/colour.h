#ifndef COLOUR_H
#define COLOUR_H


#include <stdint.h>


enum EscapeStatus
{
    UNESCAPED,
    ESCAPED
};


enum ColourSchemeType
{
    COLOUR_SCHEME_TYPE_DEFAULT,
    COLOUR_SCHEME_TYPE_ASCII,
    COLOUR_SCHEME_TYPE_ALL,
    COLOUR_SCHEME_TYPE_BLACK_WHITE,
    COLOUR_SCHEME_TYPE_WHITE_BLACK,
    COLOUR_SCHEME_TYPE_GREYSCALE,
    COLOUR_SCHEME_TYPE_RED_WHITE,
    COLOUR_SCHEME_TYPE_FIRE,
    COLOUR_SCHEME_TYPE_ALL_VIBRANT,
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
    double h, s, v;
};


struct ColourScheme
{
    enum ColourSchemeType colour;
    enum BitDepth depth;
    int (*mapColour) (struct ColourRGB *rgb, double n, enum EscapeStatus status);
};


extern const enum ColourSchemeType COLOUR_SCHEME_TYPE_MIN;
extern const enum ColourSchemeType COLOUR_SCHEME_TYPE_MAX;


void initialiseColourScheme(struct ColourScheme *scheme, enum ColourSchemeType colour);
void setSmoothFactor(double escapeRadius);
int mapColour(struct ColourRGB *rgb, struct ColourScheme *scheme, unsigned int iterations, enum EscapeStatus status);


#endif