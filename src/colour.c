#include "colour.h"

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>


#define OUTPUT_TERMINAL_CHARSET_ " .:-=+*#%@"


/* Don't set to DEFAULT because of recursion in initialiseColourScheme() */
const enum ColourSchemeType COLOUR_SCHEME_DEFAULT = COLOUR_SCHEME_TYPE_RAINBOW;

/* Range of permissible colour scheme enum values */
const enum ColourSchemeType COLOUR_SCHEME_MIN = COLOUR_SCHEME_TYPE_DEFAULT;
const enum ColourSchemeType COLOUR_SCHEME_MAX = COLOUR_SCHEME_TYPE_MATRIX;


/* Character set for terminal output 'colouring' */
static const char *OUTPUT_TERMINAL_CHARSET = OUTPUT_TERMINAL_CHARSET_;
static const size_t OUTPUT_TERMINAL_CHARSET_LENGTH = (sizeof(OUTPUT_TERMINAL_CHARSET_) - 1) / sizeof(char);

/* Constant factor dependent on escape radius to calculate a continuous value from the iteration count */
static double smoothFactor;

/* Multiplier values to normalise the smoothed iteration values */
static const double COLOUR_SCALE_MULTIPLIER = 20.0;
static const double CHAR_SCALE_MULTIPLIER = 0.3;


static void hsvToRGB(struct ColourRGB *rgb, struct ColourHSV *hsv);
static double smooth(unsigned long int iterations);

static void mapColourSchemeASCII(struct ColourRGB *rgb, double n, enum EscapeStatus status);
static void mapColourSchemeBlackWhite(struct ColourRGB *rgb, double n, enum EscapeStatus status);
static void mapColourSchemeWhiteBlack(struct ColourRGB *rgb, double n, enum EscapeStatus status);
static void mapColourSchemeGreyscale(struct ColourRGB *rgb, double n, enum EscapeStatus status);
static void mapColourSchemeRainbow(struct ColourRGB *rgb, double n, enum EscapeStatus status);
static void mapColourSchemeRainbowVibrant(struct ColourRGB *rgb, double n, enum EscapeStatus status);
static void mapColourSchemeRedWhite(struct ColourRGB *rgb, double n, enum EscapeStatus status);
static void mapColourSchemeFire(struct ColourRGB *rgb, double n, enum EscapeStatus status);
static void mapColourSchemeRedHot(struct ColourRGB *rgb, double n, enum EscapeStatus status);
static void mapColourSchemeMatrix(struct ColourRGB *rgb, double n, enum EscapeStatus status);


/* Initialise ColourScheme struct */
void initialiseColourScheme(struct ColourScheme *scheme, enum ColourSchemeType colour)
{
    scheme->colour = colour;

    switch (colour)
    {
        case COLOUR_SCHEME_TYPE_DEFAULT:
            initialiseColourScheme(scheme, COLOUR_SCHEME_DEFAULT);
            break;
        case COLOUR_SCHEME_TYPE_ASCII:
            scheme->depth = BIT_DEPTH_ASCII;
            scheme->mapColour = mapColourSchemeASCII;
            break;
        case COLOUR_SCHEME_TYPE_BLACK_WHITE:
            scheme->depth = BIT_DEPTH_1;
            scheme->mapColour = mapColourSchemeBlackWhite;
            break;
        case COLOUR_SCHEME_TYPE_WHITE_BLACK:
            scheme->depth = BIT_DEPTH_1;
            scheme->mapColour = mapColourSchemeWhiteBlack;
            break;
        case COLOUR_SCHEME_TYPE_GREYSCALE:
            scheme->depth = BIT_DEPTH_8;
            scheme->mapColour = mapColourSchemeGreyscale;
            break;
        case COLOUR_SCHEME_TYPE_RAINBOW:
            scheme->depth = BIT_DEPTH_24;
            scheme->mapColour = mapColourSchemeRainbow;
            break;
        case COLOUR_SCHEME_TYPE_RAINBOW_VIBRANT:
            scheme->depth = BIT_DEPTH_24;
            scheme->mapColour = mapColourSchemeRainbowVibrant;
            break;
        case COLOUR_SCHEME_TYPE_RED_WHITE:
            scheme->depth = BIT_DEPTH_24;
            scheme->mapColour = mapColourSchemeRedWhite;
            break;
        case COLOUR_SCHEME_TYPE_FIRE:
            scheme->depth = BIT_DEPTH_24;
            scheme->mapColour = mapColourSchemeFire;
            break;
        case COLOUR_SCHEME_TYPE_RED_HOT:
            scheme->depth = BIT_DEPTH_24;
            scheme->mapColour = mapColourSchemeRedHot;
            break;
        case COLOUR_SCHEME_TYPE_MATRIX:
            scheme->depth = BIT_DEPTH_24;
            scheme->mapColour = mapColourSchemeMatrix;
            break;
        default:
            initialiseColourScheme(scheme, COLOUR_SCHEME_DEFAULT);
            break;
    }

    return;
}


/* Set iteration count smoothing factor */
void setSmoothFactor(double escapeRadius)
{
    smoothFactor = log(log(escapeRadius)) / log(escapeRadius);
    return;
}


/* Smooth the iteration count then map it to an RGB value */
void mapColour(struct ColourRGB *rgb, struct ColourScheme *scheme,
                 unsigned long int iterations, enum EscapeStatus status)
{
    double n = (double) iterations;

    if (scheme->depth == BIT_DEPTH_1)
        n = (double) iterations;
    else if (status == ESCAPED)
        n = smooth(iterations);

    scheme->mapColour(rgb, n, status);
    return;
}


/* Convert colour scheme enum to a string */
void getColourString(char *dest, enum ColourSchemeType colour, size_t n)
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


/* Convert bit depth enum to integer */
unsigned int getBitDepth(enum BitDepth depth)
{
    switch (depth)
    {
        case BIT_DEPTH_ASCII:
            return 0;
        case BIT_DEPTH_1:
            return 1;
        case BIT_DEPTH_8:
            return 8;
        case BIT_DEPTH_24:
            return 24;
        default:
            return 0;
    }

    return 0;
}


/* Makes discrete iteration count a continuous value */
static double smooth(unsigned long int iterations)
{
    return (iterations + 1.0 - smoothFactor);
}


/* Map HSV colour values to RGB */
static void hsvToRGB(struct ColourRGB *rgb, struct ColourHSV *hsv)
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
    i = (unsigned char) floor(hsv->h / 60.0);
    
    x = (hsv->h / 60.0) - i;
    p = hsv->v * (1.0 - hsv->s) * 255.0;
    q = hsv->v * (1.0 - hsv->s * x) * 255.0;
    r = hsv->v * (1.0 - hsv->s * (1.0 - x)) * 255.0;

    switch (i)
    {
        case 0:
            rgb->r = (uint8_t) hsv->v;
            rgb->g = (uint8_t) r;
            rgb->b = (uint8_t) p;
            break;
        case 1:
            rgb->r = (uint8_t) q;
            rgb->g = (uint8_t) hsv->v;
            rgb->b = (uint8_t) p;
            break;
        case 2:
            rgb->r = (uint8_t) p;
            rgb->g = (uint8_t) hsv->v;
            rgb->b = (uint8_t) r;
            break;
        case 3:
            rgb->r = (uint8_t) p;
            rgb->g = (uint8_t) q;
            rgb->b = (uint8_t) hsv->v;
            break;
        case 4:
            rgb->r = (uint8_t) r;
            rgb->g = (uint8_t) p;
            rgb->b = (uint8_t) hsv->v;
            break;
        case 5:
            rgb->r = (uint8_t) hsv->v;
            rgb->g = (uint8_t) p;
            rgb->b = (uint8_t) q;
            break;
        default:
            rgb->r = (uint8_t) hsv->v;
            rgb->g = (uint8_t) r;
            rgb->b = (uint8_t) p;
            break;
    }
    
    return;
}


/* Maps a given iteration count to an index of outputChars[] */
static void mapColourSchemeASCII(struct ColourRGB *rgb, double n, enum EscapeStatus status)
{
    size_t i = OUTPUT_TERMINAL_CHARSET_LENGTH - 1;

    if (status == ESCAPED)
        i = (size_t) fmod((CHAR_SCALE_MULTIPLIER * n), i);

    rgb->r = (uint8_t) OUTPUT_TERMINAL_CHARSET[i];

    return;
}


/* Black and white bit map */
static void mapColourSchemeBlackWhite(struct ColourRGB *rgb, double n, enum EscapeStatus status)
{
    /* Set/unset n'th bit of the byte */
    if (status == UNESCAPED)
        rgb->r |= 1 << ((CHAR_BIT - 1) - (unsigned char) n); 
    else if (status == ESCAPED)
        rgb->r &= ~(1 << ((CHAR_BIT - 1) - (unsigned char) n));

    return;
}


/* White and black bit map */
static void mapColourSchemeWhiteBlack(struct ColourRGB *rgb, double n, enum EscapeStatus status)
{
    /* Unset/set n'th bit of the byte */
    if (status == UNESCAPED)
        rgb->r &= ~(1 << ((CHAR_BIT - 1) - (unsigned int) n));
    else if (status == ESCAPED)
        rgb->r |= 1 << ((CHAR_BIT - 1) - (unsigned int) n);

    return;
}


/* 8-bit greyscale */
static void mapColourSchemeGreyscale(struct ColourRGB *rgb, double n, enum EscapeStatus status)
{
    rgb->r = 0;

    if (status == UNESCAPED)
    {
        /* Gets values between 0 and 255 */
        rgb->r = (uint8_t) (255 - fabs(fmod(n * 8.5, 510) - 255));
        
        /* Prevent shade getting too dark */
        if (rgb->r < 30)
            rgb->r = 30;
    }

    return;
}


/* All hues */
static void mapColourSchemeRainbow(struct ColourRGB *rgb, double n, enum EscapeStatus status)
{
    struct ColourHSV hsv =
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
static void mapColourSchemeRainbowVibrant(struct ColourRGB *rgb, double n, enum EscapeStatus status)
{
    struct ColourHSV hsv =
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
static void mapColourSchemeRedWhite(struct ColourRGB *rgb, double n, enum EscapeStatus status)
{
    struct ColourHSV hsv =
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
static void mapColourSchemeFire(struct ColourRGB *rgb, double n, enum EscapeStatus status)
{
    struct ColourHSV hsv =
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
static void mapColourSchemeRedHot(struct ColourRGB *rgb, double n, enum EscapeStatus status)
{
    struct ColourHSV hsv =
    {
        .h = 0.0,
        .s = 1.0,
        .v = 0.0
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
static void mapColourSchemeMatrix(struct ColourRGB *rgb, double n, enum EscapeStatus status)
{
    struct ColourHSV hsv =
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