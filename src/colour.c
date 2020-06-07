#include <colour.h>

const enum ColourScheme COLOUR_MIN;
const enum ColourScheme COLOUR_MAX;


int mapASCII(int n, struct complex zN, int nMax, int bitDepth) /* Maps a given iteration count to an index of outputChars[] */
{
    double smoothedN;
    
    int charIndex = bitDepth - 1;
    
    if (n == nMax)
    {
        return charIndex; /* Black */
    }
    else
    {
        smoothedN = n + 1 - log(log(sqrt(pow(zN.re, 2) + pow(zN.im, 2)))) / log(ESCAPE_RADIUS); /* Makes discrete iteration count a continuous value */
    }
    
    charIndex = fmod((CHAR_SCALE_MULTIPLIER * smoothedN), charIndex); /* Excludes the black value */
    
    return charIndex;
}


void mapGrayscale(int n, struct complex zN, int nMax, unsigned char *grayscaleShade) /* Maps a given iteration count to a grayscale shade */
{
    double smoothedN;
    
    if (n != nMax)
    {
        smoothedN = n + 1 - log(log(sqrt(pow(zN.re, 2) + pow(zN.im, 2)))) / log(ESCAPE_RADIUS); /* Makes discrete iteration count a continuous value */
        
        *grayscaleShade = 255 - fabs(fmod(smoothedN * 8.5, 510) - 255); /* Gets values between 0 and 255 */
        
        if (*grayscaleShade < 30)
        {
            *grayscaleShade = 30; /* Prevents shade getting too dark */
        }
    }
    else /* If inside set, colour black */
    {
        *grayscaleShade = 0;
    }
}


void mapColour(int n, struct complex zN, int nMax, struct rgb *rgbColour, int colourScheme) /* Maps an iteration count to an HSV value */
{
    double smoothedN;
    
    double hue = 0, saturation = 0, value = 0;
    
    if (n != nMax)
    {
        smoothedN = n + 1 - log(log(sqrt(pow(zN.re, 2) + pow(zN.im, 2)))) / log(ESCAPE_RADIUS); /* Makes discrete iteration count a continuous value */
    }
    
    switch (colourScheme)
    {
        case 0: /* Default - all colours */
            value = 0.8;
            saturation = 0.6;
            
            if (n == nMax) /* If inside set */
            {
                value = 0; /* Black */
            }
            else
            {
                hue = fmod(COLOUR_SCALE_MULTIPLIER * smoothedN, 360); /* Any colour */
            }
            
            break;
        case 4: /* Red and white */
            hue = 0; /* Red */
            value = 1;
            
            if (n == nMax) /* If inside set */
            {
                saturation = 1;
            }
            else
            {
                saturation = 0.7 - fabs(fmod(smoothedN / 20, 1.4) - 0.7); /* Varies saturation of red between 0 and 0.7 */
                
                if (saturation > 0.7)
                {
                    saturation = 0.7;
                }
                else if (saturation < 0)
                {
                    saturation = 0;
                }
            }
            
            break;
        case 5: /* Fire */
            saturation = 0.85;
            value = 0.85;
            
            if (n == nMax) /* If inside set */
            {
                value = 0; /* Black */
            }
            else
            {
                hue = 50 - fabs(fmod(smoothedN * 2, 100) - 50); /* Varies hue between 0 and 50 - red to yellow */
            }
            
            break;
        case 6: /* Vibrant */
            saturation = 1;
            value = 1;
            
            if (n == nMax) /* If inside set */
            {
                value = 0; /* Black */
            }
            else
            {
                hue = fmod(COLOUR_SCALE_MULTIPLIER * smoothedN, 360); /* Any colour */
            }
            
            break;
        case 7: /* Red hot */
            if (n == nMax) /* If inside set */
            {
                value = 0; /* Black */
            }
            else
            {
                smoothedN = 90 - fabs(fmod(smoothedN * 2, 180) - 90); /* Gets values between 0 and 90 */
                
                if (smoothedN <= 30) /* Varying brightness of red */
                {
                    hue = 0;
                    saturation = 1;
                    value = smoothedN / 30;
                }
                else /* Varying hue between 0 and 60 - red to yellow */
                {
                    hue = smoothedN - 30;
                    saturation = 1;
                    value = 1;
                }
            }
            
            break;
        case 8: /* Matrix */
            if (n == nMax) /* If inside set */
            {
                value = 0; /* Black */
            }
            else /* Varying brightness of green */
            {
                hue = 120;
                saturation = 1;
                value =  (90 - fabs(fmod(smoothedN * 2, 180) - 90)) / 90;
            }
            
            break;
    }
        
    hsvToRGB(hue, saturation, value, rgbColour); /* Convert HSV value to an RGB value */
}


void hsvToRGB(double h, double s, double v, struct rgb *rgbColour)
{
    int i; /* Intermediate value for calculation */
    
    double p, q, t; /* RGB values */
    
    double rgbArray[3];
    
    if (v == 0)
    {
        rgbColour->r = rgbColour->g = rgbColour->b = 0; /* Black */
        
        return;
    }
    
    i = floor(h / 60); /* Integer from 0 to 6 */
    
    h = (h / 60) - i;
    
    p = v * (1 - s);
    q = v * (1 - s * h);
    t = v * (1 - s * (1 - h));
    
    switch (i)
    {
        case 0:
            rgbArray[0] = v;
            rgbArray[1] = t;
            rgbArray[2] = p;
            
            break;
        case 1:
            rgbArray[0] = q;
            rgbArray[1] = v;
            rgbArray[2] = p;
            
            break;
        case 2:
            rgbArray[0] = p;
            rgbArray[1] = v;
            rgbArray[2] = t;
            
            break;
        case 3:
            rgbArray[0] = p;
            rgbArray[1] = q;
            rgbArray[2] = v;
            
            break;
        case 4:
            rgbArray[0] = t;
            rgbArray[1] = p;
            rgbArray[2] = v;
            
            break;
        case 5:
            rgbArray[0] = v;
            rgbArray[1] = p;
            rgbArray[2] = q;
            break;
        case 6: /* Error handling - same as case 0 */
            rgbArray[0] = v;
            rgbArray[1] = t;
            rgbArray[2] = p;
            
            break;
    }
    
    /* Normalise to integer 0-255 ranges */
    rgbColour->r = 255 * rgbArray[0];
    rgbColour->g = 255 * rgbArray[1];
    rgbColour->b = 255 * rgbArray[2];
}