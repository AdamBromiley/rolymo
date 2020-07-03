#include "colour.h"

#include <complex.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "ext_precision.h"
#include "mandelbrot_parameters.h"


#define OUTPUT_TERMINAL_CHARSET_ " .:-=+*#%@"


/* Don't set to DEFAULT because of recursion in initialiseColourScheme() */
const ColourSchemeType COLOUR_SCHEME_DEFAULT = COLOUR_SCHEME_TYPE_RAINBOW;

/* Range of permissible colour scheme enum values */
const ColourSchemeType COLOUR_SCHEME_MIN = COLOUR_SCHEME_TYPE_DEFAULT;
const ColourSchemeType COLOUR_SCHEME_MAX = COLOUR_SCHEME_TYPE_MATRIX;


/* Character set for terminal output 'colouring' */
static const char *OUTPUT_TERMINAL_CHARSET = OUTPUT_TERMINAL_CHARSET_;
static const size_t OUTPUT_TERMINAL_CHARSET_LENGTH = (sizeof(OUTPUT_TERMINAL_CHARSET_) - 1) / sizeof(char);

/* Multiplier values to normalise the smoothed iteration values */
static const double COLOUR_SCALE_MULTIPLIER = 30.0;
static const double CHAR_SCALE_MULTIPLIER = 0.3;


static void hsvToRGB(RGB *rgb, HSV *hsv);

static char mapColourSchemeASCII(double n, EscapeStatus status);

static void mapColourSchemeBlackWhite(char *byte, int offset, EscapeStatus status);
static void mapColourSchemeWhiteBlack(char *byte, int offset, EscapeStatus status);

static uint8_t mapColourSchemeGreyscale(double n, EscapeStatus status);

static void mapColourSchemeRainbow(RGB *rgb, double n, EscapeStatus status);
static void mapColourSchemeRainbowVibrant(RGB *rgb, double n, EscapeStatus status);
static void mapColourSchemeRedWhite(RGB *rgb, double n, EscapeStatus status);
static void mapColourSchemeFire(RGB *rgb, double n, EscapeStatus status);
static void mapColourSchemeRedHot(RGB *rgb, double n, EscapeStatus status);
static void mapColourSchemeMatrix(RGB *rgb, double n, EscapeStatus status);


/* Initialise ColourScheme struct */
void initialiseColourScheme(ColourScheme *scheme, ColourSchemeType colour)
{
    scheme->scheme = colour;

    switch (colour)
    {
        case COLOUR_SCHEME_TYPE_DEFAULT:
            initialiseColourScheme(scheme, COLOUR_SCHEME_DEFAULT);
            break;
        case COLOUR_SCHEME_TYPE_ASCII:
            scheme->depth = BIT_DEPTH_ASCII;
            scheme->mapColour.ascii = mapColourSchemeASCII;
            break;
        case COLOUR_SCHEME_TYPE_BLACK_WHITE:
            scheme->depth = BIT_DEPTH_1;
            scheme->mapColour.monochrome = mapColourSchemeBlackWhite;
            break;
        case COLOUR_SCHEME_TYPE_WHITE_BLACK:
            scheme->depth = BIT_DEPTH_1;
            scheme->mapColour.monochrome = mapColourSchemeWhiteBlack;
            break;
        case COLOUR_SCHEME_TYPE_GREYSCALE:
            scheme->depth = BIT_DEPTH_8;
            scheme->mapColour.greyscale = mapColourSchemeGreyscale;
            break;
        case COLOUR_SCHEME_TYPE_RAINBOW:
            scheme->depth = BIT_DEPTH_24;
            scheme->mapColour.trueColour = mapColourSchemeRainbow;
            break;
        case COLOUR_SCHEME_TYPE_RAINBOW_VIBRANT:
            scheme->depth = BIT_DEPTH_24;
            scheme->mapColour.trueColour = mapColourSchemeRainbowVibrant;
            break;
        case COLOUR_SCHEME_TYPE_RED_WHITE:
            scheme->depth = BIT_DEPTH_24;
            scheme->mapColour.trueColour = mapColourSchemeRedWhite;
            break;
        case COLOUR_SCHEME_TYPE_FIRE:
            scheme->depth = BIT_DEPTH_24;
            scheme->mapColour.trueColour = mapColourSchemeFire;
            break;
        case COLOUR_SCHEME_TYPE_RED_HOT:
            scheme->depth = BIT_DEPTH_24;
            scheme->mapColour.trueColour = mapColourSchemeRedHot;
            break;
        case COLOUR_SCHEME_TYPE_MATRIX:
            scheme->depth = BIT_DEPTH_24;
            scheme->mapColour.trueColour = mapColourSchemeMatrix;
            break;
        default:
            initialiseColourScheme(scheme, COLOUR_SCHEME_DEFAULT);
            break;
    }

    return;
}


/* Smooth the iteration count then map it to an RGB value */
void mapColour(void *pixel, unsigned long n, complex z, int offset, unsigned long max, ColourScheme *scheme)
{
    EscapeStatus status = (n < max) ? ESCAPED : UNESCAPED;
    double nSmooth = 0.0;

    /* Makes discrete iteration count a continuous value */
    if (status == ESCAPED && scheme->depth != BIT_DEPTH_1)
        nSmooth = n + 1.0 - log2(log2(cabs(z)));

    switch (scheme->depth)
    {
        case BIT_DEPTH_1:
            /* Only write every byte */
            scheme->mapColour.monochrome(pixel, offset, status);
            break;
        case BIT_DEPTH_8:
            *((uint8_t *) pixel) = scheme->mapColour.greyscale(nSmooth, status);
            break;
        case BIT_DEPTH_24:
            scheme->mapColour.trueColour(pixel, nSmooth, status);
            break;
        default:
            return;
    }

    return;
}


/* Smooth the iteration count then map it to an RGB value (extended-precision) */
void mapColourExt(void *pixel, unsigned long n, long double complex z, int offset, unsigned long max,
                     ColourScheme *scheme)
{
    EscapeStatus status = (n < max) ? ESCAPED : UNESCAPED;
    double nSmooth = 0.0;

    /* Makes discrete iteration count a continuous value */
    if (status == ESCAPED && scheme->depth != BIT_DEPTH_1)
        nSmooth = n + 1.0L - log2l(log2l(cabsl(z)));

    switch (scheme->depth)
    {
        case BIT_DEPTH_1:
            /* Only write every byte */
            scheme->mapColour.monochrome(pixel, offset, status);
            break;
        case BIT_DEPTH_8:
            *((uint8_t *) pixel) = scheme->mapColour.greyscale(nSmooth, status);
            break;
        case BIT_DEPTH_24:
            scheme->mapColour.trueColour(pixel, nSmooth, status);
            break;
        default:
            return;
    }

    return;
}


/* Convert colour scheme enum to a string */
void getColourString(char *dest, ColourSchemeType colour, size_t n)
{
    const char *colourString;

    switch (colour)
    {
        case COLOUR_SCHEME_TYPE_ASCII:
            colourString = "ASCII";
            break;
        case COLOUR_SCHEME_TYPE_BLACK_WHITE:
            colourString = "Black and white";
            break;
        case COLOUR_SCHEME_TYPE_WHITE_BLACK:
            colourString = "White and black";
            break;
        case COLOUR_SCHEME_TYPE_GREYSCALE:
            colourString = "Greyscale";
            break;
        case COLOUR_SCHEME_TYPE_RAINBOW:
            colourString = "Rainbow";
            break;
        case COLOUR_SCHEME_TYPE_RAINBOW_VIBRANT:
            colourString = "Vibrant rainbow";
            break;
        case COLOUR_SCHEME_TYPE_RED_WHITE:
            colourString = "Red and white";
            break;
        case COLOUR_SCHEME_TYPE_FIRE:
            colourString = "Fire";
            break;
        case COLOUR_SCHEME_TYPE_RED_HOT:
            colourString = "Red hot";
            break;
        case COLOUR_SCHEME_TYPE_MATRIX:
            colourString = "Matrix";
            break;
        default:
            colourString = "Unknown colour scheme";
            break;
    }

    strncpy(dest, colourString, n);
    dest[n - 1] = '\0';

    return;
}


/* Map HSV colour values to RGB */
static void hsvToRGB(RGB *rgb, HSV *hsv)
{
    unsigned char i;    
    double x, p, q, r;

    if (hsv->h < 0.0)
        hsv->h = 0.0;

    if (hsv->s < 0.0)
        hsv->s = 0.0;

    if (hsv->v < 0.0)
        hsv->v = 0.0;

    /* If value = 0, the colour is black */
    if (hsv->v == 0.0)
    {
        rgb->r = rgb->g = rgb->b = 0.0;
        return;
    }

    /* Determines the RGB parameters that vary */
    i = (unsigned char) floorl(hsv->h / 60.0);
    
    x = (hsv->h / 60.0) - i;

    p = hsv->v * (1.0 - hsv->s) * 255.0;
    q = hsv->v * (1.0 - hsv->s * x) * 255.0;
    r = hsv->v * (1.0 - hsv->s * (1.0 - x)) * 255.0;

    switch (i)
    {
        case 0:
            rgb->r = (uint8_t) (hsv->v * 255.0);
            rgb->g = (uint8_t) r;
            rgb->b = (uint8_t) p;
            break;
        case 1:
            rgb->r = (uint8_t) q;
            rgb->g = (uint8_t) (hsv->v * 255.0);
            rgb->b = (uint8_t) p;
            break;
        case 2:
            rgb->r = (uint8_t) p;
            rgb->g = (uint8_t) (hsv->v * 255.0);
            rgb->b = (uint8_t) r;
            break;
        case 3:
            rgb->r = (uint8_t) p;
            rgb->g = (uint8_t) q;
            rgb->b = (uint8_t) (hsv->v * 255.0);
            break;
        case 4:
            rgb->r = (uint8_t) r;
            rgb->g = (uint8_t) p;
            rgb->b = (uint8_t) (hsv->v * 255.0);
            break;
        case 5:
            rgb->r = (uint8_t) (hsv->v * 255.0);
            rgb->g = (uint8_t) p;
            rgb->b = (uint8_t) q;
            break;
        default:
            rgb->r = (uint8_t) (hsv->v * 255.0);
            rgb->g = (uint8_t) r;
            rgb->b = (uint8_t) p;
            break;
    }
    
    return;
}


/* Maps a given iteration count to an index of outputChars[] */
static char mapColourSchemeASCII(double n, EscapeStatus status)
{
    size_t i = OUTPUT_TERMINAL_CHARSET_LENGTH - 1;

    if (status == ESCAPED)
        i = (size_t) fmod((CHAR_SCALE_MULTIPLIER * n), i);  

    return OUTPUT_TERMINAL_CHARSET[i];
}


/* Black and white bit map */
static void mapColourSchemeBlackWhite(char *byte, int offset, EscapeStatus status)
{
    /* Set/unset n'th bit of the byte */
    if (status == UNESCAPED)
        *byte |= 1 << ((CHAR_BIT - 1) - offset); 
    else if (status == ESCAPED)
        *byte &= ~(1 << ((CHAR_BIT - 1) - offset));

    return;
}


/* White and black bit map */
static void mapColourSchemeWhiteBlack(char *byte, int offset, EscapeStatus status)
{
    /* Unset/set n'th bit of the byte */
    if (status == UNESCAPED)
        *byte &= ~(1 << ((CHAR_BIT - 1) - offset));
    else if (status == ESCAPED)
        *byte |= 1 << ((CHAR_BIT - 1) - offset);

    return;
}


/* 8-bit greyscale */
static uint8_t mapColourSchemeGreyscale(double n, EscapeStatus status)
{
    uint8_t shade = 0;

    if (status == ESCAPED)
    {
        /* Gets values between 0 and 255 */
        shade = (uint8_t) (255.0 - fabs(fmod(n * 8.5, 510.0) - 255.0));
        
        /* Prevent shade getting too dark */
        if (shade < 30)
            shade = 30;
    }

    return shade;
}


/* All hues */
static void mapColourSchemeRainbow(RGB *rgb, double n, EscapeStatus status)
{
    HSV hsv =
    {
        .s = 0.6,
        .v = 0.0
    };

    if (status == ESCAPED)
    {
        /* Vary across all hues */
        hsv.h = fmod(COLOUR_SCALE_MULTIPLIER * n, 360.0);
        hsv.v = 0.8;
    }

    hsvToRGB(rgb, &hsv);

    return;
}


/* All hues with full saturation */
static void mapColourSchemeRainbowVibrant(RGB *rgb, double n, EscapeStatus status)
{
    HSV hsv =
    {
        .s = 1.0,
        .v = 0.0
    };

    if (status == ESCAPED)
    {
        hsv.h = fmod(COLOUR_SCALE_MULTIPLIER * n, 360.0);
        hsv.v = 1.0;
    }

    hsvToRGB(rgb, &hsv);

    return;
}


/* Red inside the set, red/white outside */
static void mapColourSchemeRedWhite(RGB *rgb, double n, EscapeStatus status)
{
    HSV hsv =
    {
        .h = 0.0,
        .s = 1.0,
        .v = 1.0
    };

    if (status == ESCAPED)
    {
        /* Vary saturation between white and nearly saturated */
        hsv.s = 0.7 - fabs(fmod(n / 20.0, 1.4) - 0.7);

        if (hsv.s > 0.7)
            hsv.s = 0.7;
    }

    hsvToRGB(rgb, &hsv);

    return;
}


/* Red to yellow */
static void mapColourSchemeFire(RGB *rgb, double n, EscapeStatus status)
{
    HSV hsv =
    {
        .s = 0.85,
        .v = 0.0
    };

    if (status == ESCAPED)
    {
        /* Vary hue between red and yellow */
        hsv.h = 50.0 - fabs(fmod(n * 2.0, 100.0) - 50.0);
        hsv.v = 0.85;
    }

    hsvToRGB(rgb, &hsv);

    return;
}


/* Dark red glow */
static void mapColourSchemeRedHot(RGB *rgb, double n, EscapeStatus status)
{
    HSV hsv =
    {
        .h = 0.0L,
        .s = 1.0L,
        .v = 0.0L
    };

    if (status == ESCAPED)
    {
        /* Gets values between 0 and 90 */
        n = 90.0 - fabs(fmod(n * 2.0, 180.0) - 90.0);

        if (n <= 30.0)
        {
            /* Varies brightness of red */
            hsv.v = n / 30.0;
        }
        else
        {
            /* Varies hue between 0 and 60 - red to yellow */
            hsv.h = n - 30.0;
            hsv.v = 1.0;
        }
    }

    hsvToRGB(rgb, &hsv);

    return;
}


/* Dark green glow */
static void mapColourSchemeMatrix(RGB *rgb, double n, EscapeStatus status)
{
    HSV hsv =
    {
        .h = 120.0,
        .s = 1.0,
        .v = 0.0
    };

    if (status == ESCAPED)
        hsv.v = (90.0 - fabs(fmod(n * 2.0, 180.0) - 90.0)) / 90.0;

    hsvToRGB(rgb, &hsv);

    return;
}