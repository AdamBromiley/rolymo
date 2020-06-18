/* 
 * Orig. Author:  Adam Bromiley <adam@bromiley.co.uk>
 * Created:       5 December 2019
 * Modified:      19 April 2020
 *
 * Name:          mandelbrot.c
 * Description:   A Mandelbrot/Julia set plotter
 *
 * Copyright 2020, Adam Bromiley, All rights reserved.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/


#include <mandelbrot.h>


static int processorCount; /* Number of processors */

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /* Mutex so threads can get unique IDs */

struct plottingParameters /* Parameters to be passed into the generator (pthread_create() takes one argument) */
{
    struct plotSettings *imagePlot;
    void *ppmArray;
    struct complex c;
    int blockId;
    _Bool pFlag;
    int threadId;
};


int imageOutput(struct plotSettings *imagePlot, _Bool mFlag, char *fileName);
int ppmArrayMalloc(void **ppmArray, struct plotSettings *imagePlot, struct blockInfo *block);

void setPixelSize(struct plotSettings *imagePlot);

void * juliaSet(void *parameters);

void * mandelbrotSet(void *parameters);


int exitError(char *fileName);
int exitSuccess(char *fileName);



int exitSuccess(char *fileName)
{
    printf("[MAIN]      Freeing memory...\n");
    if (fileName != NULL)
        free(fileName);	
    
    if (pthread_mutex_destroy(&mutex) != 0)
    {
        fprintf(stderr, "[ERROR]     Failure in destroying mutex\n");
        printf("[MAIN]      Exiting...\n");
        return EXIT_FAILURE;
    } 
    printf("[MAIN]      Exiting...\n");
    return EXIT_SUCCESS;
}


int exitError(char *fileName)
{
    printf("[MAIN]      Freeing memory...\n");
    if (fileName != NULL)
        free(fileName);	 
    if (pthread_mutex_destroy(&mutex) != 0)
        fprintf(stderr, "[ERROR]     Failure in destroying mutex\n");
    printf("[MAIN]      Exiting...\n");
    return EXIT_FAILURE;
}


void * juliaSet(void *parameters)
{	
    int xP, yP, n; /* Index variables */
    
    struct complex zStart, z, c; /* Function variables */
    double a; /* Temporary z.re value */
    
    int bitCount = 0; /* Index of B&W bit inside byte */
    unsigned char blackAndWhiteBitField; /* B&W value */
    unsigned char grayscaleShade; /* Grayscale value */
    struct rgb rgbColour; /* RGB pixel values */
    
    struct plotSettings *imagePlot;
    
    char outputChars[] = " .:-=+*#%@"; /* Output characters in order from light to dark for terminal output*/
    int bitDepth = strlen(outputChars);
    
    struct plottingParameters *plotParameters = parameters;
    int threadId;
    
    if (plotParameters->pFlag == 1)
    {
        pthread_mutex_lock(&mutex); /* Ensure all threads get a unique ID number with mutex */
        
        if (plotParameters->threadId == processorCount) /* A new block */
        {
            plotParameters->threadId = 0;
        }
        
        threadId = plotParameters->threadId;
        ++plotParameters->threadId;
        
        pthread_mutex_unlock(&mutex);

        printf("[THREAD %d]  Generating plot...\n", threadId);
    }
    else
    {
        processorCount = 1;
        threadId = 0;
        
        printf("[MAIN]      Generating plot...\n\n");
    }
    
    c = plotParameters->c;
    imagePlot = plotParameters->imagePlot;
    
    for (yP = 0 + threadId; yP < imagePlot->yPMax; yP += processorCount) /* For each row in the block (offset by thread ID to ensure each thread gets a unique row) */
    {
        if (plotParameters->pFlag == 0 && yP == 0)
        {
            continue; /* Centers the plot vertically - noticeable with the terminal output */
        }
        
        zStart.im = imagePlot->yCMax - (plotParameters->blockId * imagePlot->yPMax + yP) * imagePlot->pixelHeight; /* Imaginary part of pixel */

        for (xP = 0; xP < imagePlot->xPMax; ++xP) /* For each column in the row */
        {
            zStart.re = imagePlot->xCMin + xP * imagePlot->pixelWidth; /* Real part of pixel */
            
            z = zStart;
            n = 0; /* Iteration count */
            
            while (sqrt(pow(z.re, 2) + pow(z.im, 2)) < ESCAPE_RADIUS && n < imagePlot->nMax) /* If the number grows larger than the escape radius or the iteration count exceeds the defined maximum */
            {
                a = z.re;
                
                z.re = pow(z.re, 2) - pow(z.im, 2) + c.re;
                z.im = 2 * a * z.im + c.im;
                
                ++n;
            }
            
            if (plotParameters->pFlag == 1)
            {				
                if (imagePlot->colourScheme == 1) /* Black and white */
                {
                    ++bitCount;
                
                    if (bitCount == 8)
                    {
                        *((unsigned char *) plotParameters->ppmArray + yP * imagePlot->xPMax / 8 + xP / 8) = blackAndWhiteBitField;
                        bitCount = 0;
                    }

                    /* SET BIT FIELD */
                }
                else if (imagePlot->colourScheme == 2) /* White and black */
                {
                    ++bitCount;
                
                    if (bitCount == 8)
                    {
                        *((unsigned char *) plotParameters->ppmArray + yP * imagePlot->xPMax / 8 + xP / 8) = blackAndWhiteBitField;
                        bitCount = 0;
                    }
                    
                    /* SET BIT FIELD */
                }
                else if (imagePlot->colourScheme == 3) /* Grayscale */
                {
                    mapGrayscale(n, z, imagePlot->nMax, &grayscaleShade);
                    *((unsigned char *) plotParameters->ppmArray + yP * imagePlot->xPMax + xP) = grayscaleShade;
                }
                else /* 24-bit colour */
                {
                    mapColour(n, z, imagePlot->nMax, &rgbColour, imagePlot->colourScheme);
                    *((struct rgb *) plotParameters->ppmArray + yP * imagePlot->xPMax + xP) = rgbColour;
                }
            }
            else
            {
                putchar(outputChars[mapASCII(n, z, imagePlot->nMax, bitDepth)]); /* Print ASCII character to screen for terminal plot */
            }
        }
        
        if (plotParameters->pFlag == 0)
        {
            putchar('\n');
        }
    }
    
    if (plotParameters->pFlag == 0)
    {
        return NULL;
    }
    
    pthread_exit(0);
}


