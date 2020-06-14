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

struct blockInfo /* Outline of PPM file data blocks to be wrote to */
{
    int blockCount, blockRows, remainderRows;
};

struct plottingParameters /* Parameters to be passed into the generator (pthread_create() takes one argument) */
{
    struct plotSettings *imagePlot;
    void *ppmArray;
    struct complex c;
    int blockId;
    _Bool pFlag;
    int threadId;
};


void terminalOutput(struct plotSettings *terminalPlot, _Bool mFlag);
void initialiseTerminalPlot(struct plotSettings *terminalPlot);
int mapASCII(int n, struct complex zN, int nMax, int bitDepth);

int imageOutput(struct plotSettings *imagePlot, _Bool mFlag, char *fileName);
int ppmArrayMalloc(void **ppmArray, struct plotSettings *imagePlot, struct blockInfo *block);

void setPixelSize(struct plotSettings *imagePlot);

void * juliaSet(void *parameters);
int complexParser(struct complex *c);
int validateInput(char *buffer, int bufferSize);
int isFloat(char *floatStr);
double stringToDouble(char *floatStr);

void * mandelbrotSet(void *parameters);

void mapGrayscale(int n, struct complex zN, int nMax, unsigned char *grayscaleShade);
void mapColour(int n, struct complex zN, int nMax, struct rgb *rgbColour, int colourScheme);
void hsvToRGB(double h, double s, double v, struct rgb *rgbColour);

int exitError(char *fileName);
int exitSuccess(char *fileName);


#include <sys/sysinfo.h>
/* Get number of online processors */
    int processorCount = get_nprocs();


int exitSuccess(char *fileName)
{
    printf("[MAIN]      Freeing memory...\n");
    
    if (fileName != NULL)
    {
        free(fileName);	
    }
    
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
    {
        free(fileName);	
    }
    
    if (pthread_mutex_destroy(&mutex) != 0)
    {
        fprintf(stderr, "[ERROR]     Failure in destroying mutex\n");
    }
    
    printf("[MAIN]      Exiting...\n");
    
    return EXIT_FAILURE;
}


void terminalOutput(struct plotSettings *terminalPlot, _Bool mFlag) /* For outputting a plot with ASCII characters instead of a PPM file */
{
    struct complex *c = malloc(sizeof(struct complex));
    struct plottingParameters plotParameters =
    {
        terminalPlot, NULL, *c, 0, 0, 0
    };
    
    if (mFlag == 0)
    {
        juliaSet(&plotParameters);
    }
    else
    {
        mandelbrotSet(&plotParameters);
    }
    
    free(c);
    
    printf("\n[MAIN]      Plot has been outputted\n");
}


int imageOutput(struct plotSettings *imagePlot, _Bool mFlag, char *fileName)
{
    int i, yP, xP; /* Index variables */
    
    int threadId;
    pthread_t threads[processorCount]; /* Processing threads */
    
    FILE *ppmFile; /* Output file pointer */
    
    void *ppmArray; /* Array of pixels */
    
    struct complex *c; /* Constant value for Julia set */
    
    struct blockInfo locBlock;
    struct blockInfo *block;
    
    block = &locBlock;
    block->blockCount = 1;
    block->remainderRows = 0;
    
    if ((ppmFile = fopen(fileName, "wb")) == NULL)
    {
        fprintf(stderr, "[ERROR]     Failed to create file %s\n", fileName);
        
        return 1;
    }
    
    /* Write PPM file header */
    if (imagePlot->colourScheme == 1 || imagePlot->colourScheme == 2) /* Black and white, PBM file */
    {
        fprintf(ppmFile, "P4 %d %d ", imagePlot->xPMax, imagePlot->yPMax);
        
        ppmArray = (unsigned char *) ppmArray;
    }
    else if (imagePlot->colourScheme == 3) /* Grayscale, PGM file */
    {
        fprintf(ppmFile, "P5 %d %d 255 ", imagePlot->xPMax, imagePlot->yPMax);
        
        ppmArray = (unsigned char *) ppmArray;
    }
    else /* 24-bit colour, PPM file */
    {
        fprintf(ppmFile, "P6 %d %d 255 ", imagePlot->xPMax, imagePlot->yPMax);
        
        ppmArray = (struct rgb *) ppmArray;
    }
        
    if ((c = malloc(sizeof(struct complex))) == NULL)
    {
        fprintf(stderr, "[ERROR]     Memory allocation failure\n");
        
        return 1;
    }
    
    if (ppmArrayMalloc(&ppmArray, imagePlot, block) == 1) /* To prevent memory overcommitment, the pixel array must be divided into blocks */
    {
        free(c);
        
        return 1;
    }
    
    /* Parameters for processing threads, which can only take one argument */
    struct plottingParameters plotParameters =
    {
        imagePlot, ppmArray, *c, 0, 1, 0
    };
    
    setPixelSize(imagePlot);
    imagePlot->yPMax = block->blockRows;
    
    printf("[MAIN]      Detected %d online processors, %d processing threads will be created\n", processorCount, processorCount);
    
    for (i = 0; i <= block->blockCount; ++i) /* For each chunk of pixel rows in the output image */
    {
        plotParameters.blockId = i;
        
        if (i == block->blockCount)
        {
            if (block->remainderRows != 0) /* If there are excess rows because the row count did not divide evenly into chunks */
            {
                printf("[MAIN]      Working on remaining image block...\n");
                
                imagePlot->yPMax = block->remainderRows;
            }
            else /* If the final block has been completed */
            {
                break;
            }
        }
        else
        {
            printf("[MAIN]      Working on image block %d...\n", i);
        }
        
        /* Create threads to significantly decrease execution time */
        for (threadId = 0; threadId < processorCount; ++threadId)
        {			
            printf("[MAIN]      Creating thread %d...\n", threadId);
            
            if (mFlag == 0)
            {
                if (pthread_create(&threads[threadId], NULL, juliaSet, &plotParameters) != 0)
                {
                    fprintf(stderr, "[ERROR]     Thread %d could not be created\n", threadId);
                    
                    free(c);
                    free(ppmArray);
                    
                    return 1;
                }
            }
            else
            {
                if (pthread_create(&threads[threadId], NULL, mandelbrotSet, &plotParameters) != 0)
                {
                    fprintf(stderr, "[ERROR]     Thread %d could not be created\n", threadId);
                    
                    free(c);
                    free(ppmArray);
                    
                    return 1;
                }
            }
        }
        
        /* Wait for threads to exit */
        for (threadId = 0; threadId < processorCount; ++threadId)
        {
            pthread_join(threads[threadId], NULL);
            
            printf("[MAIN]      Thread %d successfully exited\n", threadId);
        }
        
        printf("[MAIN]      Writing to %s...\n", fileName);
        
        /* Write array to PPM file */
        for (yP = 0; yP < imagePlot->yPMax; ++yP)
        {
            for (xP = 0; xP < imagePlot->xPMax; ++xP)
            {
                if (imagePlot->colourScheme == 1 || imagePlot->colourScheme == 2) /* Black and white */
                {
                    if (xP % 8 == 0)
                    {
                        fwrite((unsigned char *) ppmArray + yP * imagePlot->xPMax / 8 + xP / 8, sizeof(unsigned char), 1, ppmFile);
                    }
                }
                else if (imagePlot->colourScheme == 3) /* Grayscale */
                {
                    fwrite((unsigned char *) ppmArray + yP * imagePlot->xPMax + xP, sizeof(unsigned char), 1, ppmFile);
                }
                else /* 24-bit colour */
                {
                    fwrite((struct rgb *) ppmArray + yP * imagePlot->xPMax + xP, sizeof(struct rgb), 1, ppmFile);
                }
            }
        }
    }

    fclose(ppmFile);
    
    free(c);
    free(ppmArray);
    
    printf("[MAIN]      Plot has been outputted to %s\n", fileName);
    
    return 0;
}


int ppmArrayMalloc(void **ppmArray, struct plotSettings *imagePlot, struct blockInfo *block) /* To prevent memory overcommitment, the pixel array must be divided into blocks */
{
    int i; /* Index variable */
    
    float elementSize; /* Size of each pixel in bytes */
    size_t ppmArraySize; /* Size of pixel array in bytes */
    
    switch (imagePlot->colourScheme)
    {
        case 1: /* Black and white */
            elementSize = sizeof(unsigned char) / 8.0; /* Each pixel is 1 bit */
            
            break;
        case 2: /* White and black */
            elementSize = sizeof(unsigned char) / 8.0; /* Each pixel is 1 bit */

            break;
        case 3: /* Grayscale */
            elementSize = sizeof(unsigned char); /* Each pixel is 1 byte */
            
            break;
        default: /* 24-bit colour */
            elementSize = sizeof(struct rgb); /* Each pixel is 3 bytes */
            
            break;
    }
    
    ppmArraySize = (size_t) imagePlot->yPMax * (size_t) imagePlot->xPMax * (size_t) elementSize;
    
    block->blockRows = imagePlot->yPMax;
    
    printf("[MAIN]      Allocating memory...\n");
    
    for (i = 1; i < 6; ++i) /* Try to malloc for the pixel array, with each iteration decreasing the array size */
    {
        if (i == MALLOC_ESC_ITER) /* Set reasonable limit on amount of malloc attempts */
        {
            fprintf(stderr, "[ERROR]     Memory allocation failed\n");
            
            return 1;
        }
        
        if (i == 1)
        {
            fprintf(stderr, "[WARNING]   Memory allocation of 1 block (block: %d rows, remainder: %d rows) failed\n", block->blockRows, block->remainderRows);
        }
        else
        {
            fprintf(stderr, "[WARNING]   Memory allocation of %d blocks (block: %d rows, remainder: %d rows) failed\n", i, block->blockRows, block->remainderRows);
        }
        
        printf("[MAIN]      Retrying memory allocation with %d blocks...\n", i + 1);
        
        block->blockRows = imagePlot->yPMax / (i + 1);
        block->remainderRows = imagePlot->yPMax % (i + 1);
        
        ppmArraySize = (size_t) block->blockRows * (size_t) imagePlot->xPMax * (size_t) elementSize;
    }

    for (i = 6; (*ppmArray = malloc(ppmArraySize)) == NULL; ++i) /* Try to malloc for the pixel array, with each iteration decreasing the array size */
    {
        if (i == MALLOC_ESC_ITER) /* Set reasonable limit on amount of malloc attempts */
        {
            fprintf(stderr, "[ERROR]     Memory allocation failed\n");
            
            return 1;
        }
        
        if (i == 1)
        {
            fprintf(stderr, "[WARNING]   Memory allocation of 1 block (block: %d rows, remainder: %d rows) failed\n", block->blockRows, block->remainderRows);
        }
        else
        {
            fprintf(stderr, "[WARNING]   Memory allocation of %d blocks (block: %d rows, remainder: %d rows) failed\n", i, block->blockRows, block->remainderRows);
        }
        
        printf("[MAIN]      Retrying memory allocation with %d blocks...\n", i + 1);
        
        block->blockRows = imagePlot->yPMax / (i + 1);
        block->remainderRows = imagePlot->yPMax % (i + 1);
        
        ppmArraySize = (size_t) block->blockRows * (size_t) imagePlot->xPMax * (size_t) elementSize;
    }
    
    block->blockCount = i;
    
    if (block->blockCount == 1)
    {
        printf("[MAIN]      Memory successfully allocated as 1 block (block: %d rows, remainder: %d rows)\n", block->blockRows, block->remainderRows);
    }
    else
    {
        printf("[MAIN]      Memory successfully allocated as %d blocks (block: %d rows, %zu bytes, remainder: %d rows)\n", block->blockCount, block->blockRows, ppmArraySize, block->remainderRows);
    }
    
    ++block->blockCount;
    
    printf("[MAIN]      Reallocating memory to %d blocks for stability\n", block->blockCount); /* Securing the largest memory block is not the optimal solution - safer to downsize by one */
    
    block->blockRows = imagePlot->yPMax / block->blockCount;
    block->remainderRows = imagePlot->yPMax % block->blockCount;
    
    ppmArraySize = (size_t) block->blockRows * (size_t) imagePlot->xPMax * (size_t) elementSize;

    if ((*ppmArray = realloc(*ppmArray, ppmArraySize)) == NULL)
    {
        fprintf(stderr, "[ERROR]     Memory reallocation failed\n");
        
        free(*ppmArray);
        
        return 1;
    }
    
    printf("[MAIN]      Memory successfully reallocated as %d blocks (block: %d rows, %zu bytes, remainder: %d rows)\n", block->blockCount, block->blockRows, ppmArraySize, block->remainderRows);
    
    return 0;
}


void setPixelSize(struct plotSettings *imagePlot) /* How much one pixel represents on the axes */
{
    imagePlot->pixelWidth = (imagePlot->xCMax - imagePlot->xCMin) / imagePlot->xPMax;
    imagePlot->pixelHeight = (imagePlot->yCMax - imagePlot->yCMin) / imagePlot->yPMax;
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





int isFloat(char *floatStr) /* Check if number is floating point */
{	
    int i; /* Index variable */
    
    int decimalCount = 0; /* Number of decimal points in the string */
    
    for (i = 0; i < strlen(floatStr); ++i)
    {
        if (floatStr[i] == '.')
        {
            ++decimalCount;
        }
        else if (isdigit(floatStr[i]) == 0) /* All chars should be 0-9 or . */
        {
            return 1;
        }
        
        if (decimalCount > 1) /* Too many decimal points */
        {
            return 1;
        }
    }
    
    return 0;
}


void * mandelbrotSet(void *parameters)
{
    int xP, yP, n; /* Index variables */
    
    struct complex z, c; /* Function variables */
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
    
    imagePlot = plotParameters->imagePlot;
    
    for (yP = 0 + threadId; yP < imagePlot->yPMax; yP += processorCount) /* For each row in the block (offset by thread ID to ensure each thread gets a unique row) */
    {
        if (plotParameters->pFlag == 0 && yP == 0)
        {
            continue; /* Centers the plot vertically - noticeable with the terminal output */
        }
        
        c.im = imagePlot->yCMax - (plotParameters->blockId * imagePlot->yPMax + yP) * imagePlot->pixelHeight; /* Imaginary part of pixel */
        
        for (xP = 0; xP < imagePlot->xPMax; ++xP) /* For each column in the row */
        {
            c.re = imagePlot->xCMin + xP * imagePlot->pixelWidth; /* Real part of pixel */
            
            z.re = z.im = 0;
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
                    
                    if (n == imagePlot->nMax)
                    {
                        blackAndWhiteBitField |= 1 << (7 - bitCount); /* Set bitCount'th bit of the byte */
                    }
                    else
                    {
                        blackAndWhiteBitField &= ~(1 << (7 - bitCount)); /* Unset bitCount'th bit of the byte */
                    }
                }
                else if (imagePlot->colourScheme == 2) /* White and black */
                {
                    ++bitCount;
                
                    if (bitCount == 8)
                    {
                        *((unsigned char *) plotParameters->ppmArray + yP * imagePlot->xPMax / 8 + xP / 8) = blackAndWhiteBitField;
                        bitCount = 0;
                    }
                    
                    if (n == imagePlot->nMax)
                    {
                        blackAndWhiteBitField &= ~(1 << (7 - bitCount)); /* Unset bitCount'th bit of the byte */
                    }
                    else
                    {
                        blackAndWhiteBitField |= 1 << (7 - bitCount); /* Set bitCount'th bit of the byte */
                    }
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