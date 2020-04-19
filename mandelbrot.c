/* 
 * Orig. Author:  Adam Bromiley <adam@bromiley.co.uk>
 * Created:       5 December 2019
 * Modified:      19 April 2020
 *
 * Name:          mandelbrot.c
 * Description:   A Mandelbrot/Julia set plotter
 *
 * Compilation:   `gcc mandelbrot.c -lm -pthread -o mandelbrot`
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


#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/sysinfo.h>


#define FILENAME_MAX_LEN 255
#define DEFAULT_FILENAME "output"

#define MAX_WIDTH 50000
#define MAX_HEIGHT 50000
#define DEFAULT_WIDTH 550
#define DEFAULT_HEIGHT 500
#define DEFAULT_PX_SCALE 200.0
#define TERMINAL_WIDTH 80
#define TERMINAL_HEIGHT 46

#define DEFAULT_XMAX 0.75
#define DEFAULT_XMIN -2.0
#define DEFAULT_YMAX 1.25
#define DEFAULT_YMIN -1.25

#define ESCAPE_RADIUS 2.0
#define MAX_ITERMAX 200
#define MIN_ITERMAX 50
#define DEFAULT_ITERMAX 100

#define MALLOC_ESC_ITER 15

#define COLOUR_SCALE_MULTIPLIER 20
#define CHAR_SCALE_MULTIPLIER 0.3

#define BUF_SIZE 1024


static int processorCount; /* Number of processors */

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /* Mutex so threads can get unique IDs */


struct plotSettings
{
	_Bool pFlag; /* Output type: Terminal output (0) and PPM file (1) */
	double xCMin, xCMax, yCMin, yCMax; /* Minimum/maximum X and Y coordinates */
	int xPMax, yPMax; /* Dimensions of PPM file in pixels or terminal output in columns/lines */
	double pixelWidth, pixelHeight; /* The X/Y dimensions of each pixel */
	int nMax; /* Maximum iterations before being considered to have escaped - larger nMax = more precise plot but longer execution */
	int colourScheme; /* Colour scheme of the PPM image */
};

struct blockInfo /* Outline of PPM file data blocks to be wrote to */
{
	int blockCount, blockRows, remainderRows;
};

struct complex /* Complex number */
{
	double re, im;
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

struct rgb /* RGB colour */
{
	unsigned char r, g, b;
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


int main(int argc, char **argv) /* Take in command-line options */
{
	int optionId; /* Command-line option identifier */
	
	/* Short command-line option parameter variables */
	_Bool jFlag = 0, mFlag = 0; /* Flags to display Julia/Mandelbrot sets */
	_Bool pFlag = 0, tFlag = 0, xFlag = 0, XFlag = 0, yFlag = 0, YFlag = 0, rFlag = 0, sFlag = 0, cFlag = 0; /* Flags indicating chosen CLI options */
	char *fileName = NULL; /* Output file name */
	
	/* Default plot settings for Mandelbrot image output */
	struct plotSettings imagePlot =
	{
		.pFlag = 1,
		.xCMin = DEFAULT_XMIN, .xCMax = DEFAULT_XMAX, .yCMin = DEFAULT_YMIN, .yCMax = DEFAULT_YMAX, 
		.xPMax = DEFAULT_WIDTH, .yPMax = DEFAULT_HEIGHT,
		.nMax = DEFAULT_ITERMAX,
		.colourScheme = 0
	};
	
	/* Command-line options */
	struct option longOptions[] =
	{
		{"j", no_argument, NULL, 'j'}, /* Plot a Julia set */
		{"m", no_argument, NULL, 'm'}, /* Plot the Mandlebrot set */
		{"p", no_argument, NULL, 'p'}, /* Output a PPM file */
		{"t", no_argument, NULL, 't'}, /* Output to terminal */
		{"o", required_argument, NULL, 'o'}, /* Output PPM file name */
		{"xmin", required_argument, NULL, 'x'}, /* Minimum/maximum X and Y coordinates */
		{"xmax", required_argument, NULL, 'X'},
		{"ymin", required_argument, NULL, 'y'},
		{"ymax", required_argument, NULL, 'Y'},
		{"itermax", required_argument, NULL, 'i'}, /* Maximum iteration count of calculation */
		{"width", required_argument, NULL, 'r'}, /* X and Y resolution of output PPM file */
		{"height", required_argument, NULL, 's'},
		{"colour", required_argument, NULL, 'c'}, /* Colour scheme of PPM image */
		{"help", no_argument, NULL, 'h'}, /* For guidance on command-line options */
		{0, 0, 0, 0}
	};
	
	opterr = 0; /* Removes default stderr message of getopt_long() */
	
	processorCount = get_nprocs(); /* Get number of online processors */
	
	while ((optionId = getopt_long(argc, argv, ":jmpto:x:X:y:Y:i:r:s:c:h", longOptions, NULL)) != -1) /* Iterate over all the options */
	{
		switch (optionId)
		{
			case 'j':
				jFlag = 1;
				
				break;
			case 'm':
				mFlag = 1;
				
				break;
			case 'p':
				pFlag = 1;
				
				break;
			case 't':
				tFlag = 1;
				imagePlot.pFlag = 0;
				
				break;
			case 'o':
				if (strlen(optarg) > FILENAME_MAX_LEN) /* Maximum length of file name */
				{
					fprintf(stderr, "[ERROR]     File name is too long (must be 255 characters, or shorter)\n");
					
					return exitError(fileName);
				}

				if (strchr(optarg, '/') != NULL) /* Invalid Linux file name */
				{
					fprintf(stderr, "[ERROR]     File name cannot contain '/'\n");
					
					return exitError(fileName);
				}
				
				if((fileName = malloc((strlen(optarg) + 1) * sizeof(char))) == NULL)
				{
					fprintf(stderr, "[ERROR]     Memory allocation failure\n");
					
					return exitError(fileName);
				}
				
				strcpy(fileName, optarg);

				break;
			case 'x':
				imagePlot.xCMin = stringToDouble(optarg);
				
				if (errno == EINVAL)
				{
					fprintf(stderr, "[ERROR]     Invalid argument for --xmin\n");
				
					return exitError(fileName);
				}
				else if (errno == ERANGE)
				{
					fprintf(stderr, "[ERROR]     --xmin argument out of range\n");
					
					return exitError(fileName);
				}
				
				xFlag = 1;
				
				break;
			case 'X':
				imagePlot.xCMax = stringToDouble(optarg);
				
				if (errno == EINVAL)
				{
					fprintf(stderr, "[ERROR]     Invalid argument for --xmax\n");
				
					return exitError(fileName);
				}
				else if (errno == ERANGE)
				{
					fprintf(stderr, "[ERROR]     --xmax argument out of range\n");
					
					return exitError(fileName);
				}
				
				XFlag = 1;
				
				break;
			case 'y':
				imagePlot.yCMin = stringToDouble(optarg);
				
				if (errno == EINVAL)
				{
					fprintf(stderr, "[ERROR]     Invalid argument for --ymin");
				
					return exitError(fileName);
				}
				else if (errno == ERANGE)
				{
					fprintf(stderr, "[ERROR]     --ymin argument out of range\n");
					
					return exitError(fileName);
				}
				
				yFlag = 1;
				
				break;
			case 'Y':
				imagePlot.yCMax = stringToDouble(optarg);
				
				if (errno == EINVAL)
				{
					fprintf(stderr, "[ERROR]     Invalid argument for --ymax\n");
				
					return exitError(fileName);
				}
				else if (errno == ERANGE)
				{
					fprintf(stderr, "[ERROR]     --ymax argument out of range\n");
					
					return exitError(fileName);
				}
				
				YFlag = 1;
				
				break;
			case 'i':
				imagePlot.nMax = stringToDouble(optarg);
				
				if (errno == EINVAL)
				{
					fprintf(stderr, "[ERROR]     Invalid argument for --itermax\n");
				
					return exitError(fileName);
				}
				else if (errno == ERANGE || imagePlot.nMax <= 0)
				{
					fprintf(stderr, "[ERROR]     --itermax argument out of range\n");
					
					return exitError(fileName);
				}
				
				break;
			case 'r':
				imagePlot.xPMax = stringToDouble(optarg);
				
				if (errno == EINVAL)
				{
					fprintf(stderr, "[ERROR]     Invalid argument for --width\n");
				
					return exitError(fileName);
				}
				else if (errno == ERANGE || imagePlot.xPMax <= 0)
				{
					fprintf(stderr, "[ERROR]     --width argument out of range\n");
					
					return exitError(fileName);
				}
				
				rFlag = 1;
				
				break;
			case 's':
				imagePlot.yPMax = stringToDouble(optarg);
				
				if (errno == EINVAL)
				{
					fprintf(stderr, "[ERROR]     Invalid argument for --height\n");
				
					return exitError(fileName);
				}
				else if (errno == ERANGE || imagePlot.yPMax <= 0)
				{
					fprintf(stderr, "[ERROR]     --height argument out of range\n");
					
					return exitError(fileName);
				}
				
				sFlag = 1;
				
				break;
			case 'c':
				imagePlot.colourScheme = stringToDouble(optarg);
				
				if (errno == EINVAL)
				{
					fprintf(stderr, "[ERROR]     Invalid argument for --colour\n");
				
					return exitError(fileName);
				}
				else if (errno == ERANGE || imagePlot.colourScheme < 0 || imagePlot.colourScheme > 8)
				{
					fprintf(stderr, "[ERROR]     --colour argument out of range\n");
					
					return exitError(fileName);
				}
				
				cFlag = 1;
				
				break;
			case 'h':
				if (argc != 2)
				{
					fprintf(stderr, "[ERROR]     --help is mutually exclusive with all other options\n");
					
					return exitError(fileName);
				}
				
				printf("\nUsage: mandelbrot [-j|-m] [-t|-p [-o FILENAME] [--colour=SCHEME]] [PLOT PARAMETERS...]\n");
				printf("       mandelbrot --help\n\n");
				printf("Mandatory arguments to long options are mandatory for short options too.\n");
				printf("\nPlot type:\n");
				printf("  -j                           Julia set (c variable is prompted for during execution)\n");
				printf("  -m                           Mandelbrot set\n");
				printf("\nOutput formatting:\n");
				printf("  -t                           Output to terminal using ASCII characters as shading\n");
				printf("  -p                           Output to PPM image file (default)\n");
				printf("  -o FILENAME                  With -p: Specify output file name\n");
				printf("                               [+] Default = \"%s\"\n", DEFAULT_FILENAME);
				printf("  -c SCHEME, --colour=SCHEME   With -p: Specify colour palette to use\n");
				printf("                               [+] Default = 0\n");
				printf("                               [+] SCHEME may be:\n");
				printf("                                   0 = All hues\n");
				printf("                                   1 = Black and white - pixels in the set are black\n");
				printf("                                   2 = White and black - pixels in the set are white\n");
				printf("                                   3 = Grayscale\n");
				printf("                                   4 = Red and white\n");
				printf("                                   5 = Fire\n");
				printf("                                   6 = Vibrant - All hues but with maximum saturation\n");
				printf("                                   7 = Red hot\n");
				printf("                                   8 = Matrix\n");
				printf("                               [+] 1 and 2 have a bit depth of 1\n");
				printf("                               [+] 3 has a bit depth of 8 bits\n");
				printf("                               [+] Others are full colour with a bit depth of 24 bits\n");
				printf("\nPlot parameters:\n");
				printf("  -x XMIN,   --xmin=XMIN       Minimum x (real) value to plot\n");
				printf("  -X XMAX,   --xmax=XMAX       Maximum x (real) value to plot\n");
				printf("  -y YMIN,   --ymin=YMIN       Minimum y (imaginary) value to plot\n");
				printf("  -Y YMAX,   --ymax=YMAX       Maximum y (imaginary) value to plot\n");
				printf("  -i NMAX,   --itermax=NMAX    The maximum number of function iterations before a number is deemed to be within the set\n");
				printf("                               [+] A larger maximum leads to a preciser plot but increases computation time\n");
				printf("                               [+] Default = %d\n", DEFAULT_ITERMAX);
				printf("  -r WIDTH,  --width=WIDTH     The width of the PPM file in pixels\n");
				printf("                               [+] If colour scheme 1 or 2, WIDTH must be a multiple of 8 to allow for bit-width pixels\n");
				printf("  -s HEIGHT, --height=HEIGHT   The height of the PPM file in pixels\n");
				printf("\n  Default parameters:\n");
				printf("   Julia Set:\n");
				printf("    XMIN   = %.2f\n", -ESCAPE_RADIUS);
				printf("    XMAX   = %.2f\n", ESCAPE_RADIUS);
				printf("    YMIN   = %.2f\n", -ESCAPE_RADIUS);
				printf("    YMAX   = %.2f\n", ESCAPE_RADIUS);
				printf("    WIDTH  = %.2f\n", (ESCAPE_RADIUS - (-ESCAPE_RADIUS)) * DEFAULT_PX_SCALE);
				printf("    HEIGHT = %.2f\n", (ESCAPE_RADIUS - (-ESCAPE_RADIUS)) * DEFAULT_PX_SCALE);
				printf("   Mandelbrot set:\n");
				printf("    XMIN   = %f\n", DEFAULT_XMIN);
				printf("    XMAX   = %f\n", DEFAULT_XMAX);
				printf("    YMIN   = %f\n", DEFAULT_YMIN);
				printf("    YMAX   = %f\n", DEFAULT_YMAX);
				printf("    WIDTH  = %d\n", DEFAULT_WIDTH);
				printf("    HEIGHT = %d\n", DEFAULT_HEIGHT);
				printf("\nMiscellaneous:\n");
				printf("  -h,        --help            Display this help message and exit\n");
				printf("\n\nExamples:\n");
				printf("  mandelbrot -m\n");
				printf("  mandelbrot -j -p -o \"juliaset.ppm\"\n");
				printf("  mandelbrot -m -t\n");
				printf("  mandelbrot -m -p --itermax=200 --width=5500 --height=5000 --colour=7\n");
				putchar('\n');
				
				return EXIT_SUCCESS;
			case '?': /* Invalid option */
				fprintf(stderr, "[ERROR]     Invalid option: -%c\n", optopt);
				
				return exitError(fileName);
			case ':': /* Missing argument to option */
				fprintf(stderr, "[ERROR]     The option -%c requires an argument\n", optopt);
				
				return exitError(fileName);
			default:
				fprintf(stderr, "[ERROR]     Unknown error when reading command-line options\n");
				
				return exitError(fileName);
		}
	}

	if (argc == 1) /* If no command-line options */
	{
		printf("Usage: mandelbrot [-j|-m] [-t|-p [-o FILENAME] [--colour=SCHEME]] [PLOT PARAMETERS...]\n");
		printf("       mandelbrot --help\n\n");
		
		return EXIT_SUCCESS;
	}
	
	if ((imagePlot.colourScheme == 1 || imagePlot.colourScheme == 2) && imagePlot.xPMax % 8 != 0)
	{
		fprintf(stderr, "[ERROR]     Width must be a multiple of 8 for a B&W image because each pixel is a bit wide\n");
		
		return exitError(fileName);
	}
	
	if (imagePlot.xCMax <= imagePlot.xCMin)
	{
		fprintf(stderr, "[ERROR]     Invalid x-coordinate range\n");
		
		return exitError(fileName);
	}
	else if (imagePlot.yCMax <= imagePlot.yCMin)
	{
		fprintf(stderr, "[ERROR]     Invalid y-coordinate range\n");
		
		return exitError(fileName);
	}
	
	if (fileName != NULL && imagePlot.pFlag == 0)
	{
		fprintf(stderr, "[ERROR]     Terminal output does not generate a file\n");
		
		return exitError(fileName);
	}
	else if (pFlag == 1 && tFlag == 1)
	{
		fprintf(stderr, "[ERROR]     Multiple output types selected\n");
		
		return exitError(fileName);
	}
	else if (jFlag == mFlag)
	{
		if (jFlag == 1)
		{
			fprintf(stderr, "[ERROR]     Multiple plot types selected\n");
		}
		else
		{
			fprintf(stderr, "[ERROR]     No plot type selected\n");
		}
		
		return exitError(fileName);
	}
	else if (fileName == NULL && imagePlot.pFlag == 1)
	{
		fprintf(stderr, "[WARNING]   No output file name specified... defaulted to 'output.ppm'\n");
		
		if ((fileName = malloc((strlen(DEFAULT_FILENAME) + 1) * sizeof(char))) == NULL)
		{
			fprintf(stderr, "[ERROR]     Memory allocation failure\n");
			
			return exitError(fileName);
		}
		
		strcpy(fileName, DEFAULT_FILENAME);
	}

	if (imagePlot.nMax < MIN_ITERMAX)
	{
		fprintf(stderr, "[WARNING]   Maximum iteration count is low and may result in an unsatisfactory plot. Recommended range is between %d and %d\n", MIN_ITERMAX, MAX_ITERMAX);
	}
	else if (imagePlot.nMax > MAX_ITERMAX)
	{
		fprintf(stderr, "[WARNING]   Maximum iteration count is high and may not noticeably increase the quality of the plot. Recommended range is between %d and %d\n", MIN_ITERMAX, MAX_ITERMAX);
	}
	
	if (ESCAPE_RADIUS > 2)
	{
		fprintf(stderr, "[WARNING]   Escape radius is greater than 2. A number, z, is guaranteed to escape if |z| > 2 so there will be unecessary calculations\n");
	}
	else if (ESCAPE_RADIUS < 2)
	{
		fprintf(stderr, "[WARNING]   Escape radius is less than 2. This will result in some points incorrectly being classified as within the set. Recommended value is 2\n");
	}
	
	if (imagePlot.xPMax > MAX_WIDTH)
	{
		fprintf(stderr, "[WARNING]   Image width is very high; the program may be killed unexpectedly by the kernel. Recommend range is below %dpx\n", MAX_WIDTH);
	}
	else if (imagePlot.yPMax > MAX_HEIGHT)
	{
		fprintf(stderr, "[WARNING]   Image height is very high; the program may be killed unexpectedly by the kernel. Recommended range is below %dpx\n", MAX_HEIGHT);
	}
	
	if (imagePlot.xCMax > 2 || imagePlot.xCMin < -2 || imagePlot.yCMax > 2 || imagePlot.yCMin < -2)
	{
		fprintf(stderr, "[WARNING]   The coordinate range extends outside [(-2, -2), (2, 2)] (where Mandelbrot/Julia sets are contained). This will lead to redundant points being calulated\n");
	}
	
	if ((rFlag == 1 || sFlag == 1) && tFlag == 1)
	{
		fprintf(stderr, "[WARNING]   Resolution of the terminal output defaults back to %d by %d\n", TERMINAL_WIDTH, TERMINAL_HEIGHT);
	}
	
	if (tFlag == 1 && cFlag == 1)
	{
		fprintf(stderr, "[WARNING]   --colour has no effect on terminal output\n");
	}
	
	if (jFlag == 1) /* Set defaults for Julia set plot */
	{
		if (xFlag == 0)
		{
			imagePlot.xCMin = -ESCAPE_RADIUS;
		}
		
		if (XFlag == 0)
		{
			imagePlot.xCMax = ESCAPE_RADIUS;
		}
		
		if (yFlag == 0)
		{
			imagePlot.yCMin = -ESCAPE_RADIUS;
		}
		
		if (YFlag == 0)
		{
			imagePlot.yCMax = ESCAPE_RADIUS;
		}
		
		if (rFlag == 0)
		{
			imagePlot.xPMax = (imagePlot.xCMax - imagePlot.xCMin) * DEFAULT_PX_SCALE;
		}
		
		if (sFlag == 0)
		{
			imagePlot.yPMax = (imagePlot.yCMax - imagePlot.yCMin) * DEFAULT_PX_SCALE;
		}
	}
	
	if (imagePlot.pFlag == 0)
	{	
		struct plotSettings *terminalPlot;
		terminalPlot = &imagePlot;
		
		terminalOutput(terminalPlot, mFlag);
	}
	else
	{
		if (imageOutput(&imagePlot, mFlag, fileName) == 1)
		{
			return exitError(fileName);
		}
	}
	
	return exitSuccess(fileName);
}


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
	
	initialiseTerminalPlot(terminalPlot);
	setPixelSize(terminalPlot);
	
	if (mFlag == 0)
	{		
		do /* Input constant value of Julia set */
		{
			printf("\nInput the constant value, c, of the Julia set in the form a + bi: ");
		}
		while (complexParser(c) == 1);
	}
	
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


void initialiseTerminalPlot(struct plotSettings *terminalPlot)
{
	terminalPlot->xPMax = TERMINAL_WIDTH;
	terminalPlot->yPMax = TERMINAL_HEIGHT;
	
	setPixelSize(terminalPlot);
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
	
	if (mFlag == 0)
	{		
		do /* Input constant value of Julia set */
		{
			printf("\nInput the constant value, c, of the Julia set in the form a + bi: ");
		} while (complexParser(c) == 1);
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
	long long int ppmArraySize; /* Size of pixel array in bytes */
	
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
	
	ppmArraySize = imagePlot->yPMax * imagePlot->xPMax * elementSize;
	
	block->blockRows = imagePlot->yPMax;
	
	printf("[MAIN]      Allocating memory...\n");
	
	for (i = 1; (*ppmArray = malloc(ppmArraySize)) == NULL; ++i) /* Try to malloc for the pixel array, with each iteration decreasing the array size */
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
		
		ppmArraySize = block->blockRows * imagePlot->xPMax * elementSize;
	}
	
	block->blockCount = i;
	
	if (block->blockCount == 1)
	{
		printf("[MAIN]      Memory successfully allocated as 1 block (block: %d rows, remainder: %d rows)\n", block->blockRows, block->remainderRows);
	}
	else
	{
		printf("[MAIN]      Memory successfully allocated as %d blocks (block: %d rows, %lld bytes, remainder: %d rows)\n", block->blockCount, block->blockRows, ppmArraySize, block->remainderRows);
	}
	
	++block->blockCount;
	
	printf("[MAIN]      Reallocating memory to %d blocks for stability\n", block->blockCount); /* Securing the largest memory block is not the optimal solution - safer to downsize by one */
	
	block->blockRows = imagePlot->yPMax / block->blockCount;
	block->remainderRows = imagePlot->yPMax % block->blockCount;
	
	ppmArraySize = block->blockRows * imagePlot->xPMax * elementSize;
	
	if ((*ppmArray = realloc(*ppmArray, ppmArraySize)) == NULL)
	{
		fprintf(stderr, "[ERROR]     Memory reallocation failed\n");
		
		free(*ppmArray);
		
		return 1;
	}
	
	printf("[MAIN]      Memory successfully reallocated as %d blocks (block: %d rows, %lld bytes, remainder: %d rows)\n", block->blockCount, block->blockRows, ppmArraySize, block->remainderRows);
	
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


int complexParser(struct complex *c) /* Validate input of a complex number in the form a + bi */
{
	int i; /* Index variable */
	
	char buffer[BUF_SIZE]; /* Input buffer */
	int inputLength;
	
	int plusCount = 0, minusCount = 0, iCount = 0, decimalCount = 0, charCount = 0; /* Count of format-identifying characters */
	
	int formatType; /* Format of complex number */
	
	char *nPtr; /* Newline character pointer */

	_Bool aNegFlag = 0, bNegFlag = 0; /* Negative flags for real and imaginary parts */

	char *rePart, *imPart; /* Complex number */
	
	if (validateInput(buffer, sizeof(buffer)) == 1) /* Validate input length */
	{
		return 1;
	}
	
	if ((nPtr = strchr(buffer, '\n')) != NULL) /* Strip trailing newline character */
	{
		*nPtr = 0;
	}
	
	for (i = 0; buffer[i] != 0; ++i) /* Count format-identifying characters */
	{
		switch (buffer[i])
		{
			case '+':
				++plusCount;
				break;
			case '-':
				++minusCount;
				break;
			case 'i':
				++iCount;
				break;
			case '.':
				++decimalCount;
				break;
		}
	}
	
	inputLength = strlen(buffer);
	for (i = 0; i < inputLength; ++i) /* Strip whitespace from string */
	{
		if (buffer[i] == ' ')
		{
			buffer[i] = 0;
			
			continue;
		}
		else
		{
			if (i != charCount)
			{
				buffer[charCount] = buffer[i]; /* Shift left to fill in null bytes */
				buffer[i] = 0;
			}
			
			++charCount;
		}
	}
	
	if (plusCount > 1 || minusCount > 2 || iCount > 1 || decimalCount > 2) /* Quick check to remove erroneous inputs */
	{
		fprintf(stderr, "[ERROR]     Too many symbols (+/-/i/.) in input\n");
		
		return 1;
	}
	
	if ((minusCount == 2 || iCount == 0) && plusCount != 0)
	{
		fprintf(stderr, "[ERROR]     Incompatible number of symbols (+/-/i/.)\n");
		
		return 1;
	}
	
	inputLength = strlen(buffer);

	if ((rePart = malloc(inputLength * sizeof(char))) == NULL)
	{
		fprintf(stderr, "[ERROR]     Memory allocation failure\n");
		
		return 1;
	}

	if ((imPart = malloc(inputLength * sizeof(char))) == NULL)
	{
		fprintf(stderr, "[ERROR]     Memory allocation failure\n");
		
		free(rePart);

		return 1;
	}
	
	/* Read comments for identified input formats with each check */
	/* "a" and "b" are arbitrary strings and "+/-/i" are special characters */
	
	if (buffer[inputLength - 1] == '+' || buffer[inputLength - 1] == '-') /* "a+", "a-" */
	{
		fprintf(stderr, "[ERROR]     Terminating '+/-'\n");
		
		free(rePart);
		free(imPart);

		return 1;
	}
	else if (buffer[0] == '+') /* "+a" */
	{
		fprintf(stderr, "[ERROR]     Leading '+'\n");
		
		free(rePart);
		free(imPart);

		return 1;
	}
	else if (buffer[0] == '-')
	{
		if (buffer[1] == '-' || buffer[1] == '+') /* "--a", "--i", "-+i", "--ai", "-+ai" */
		{
			fprintf(stderr, "[ERROR]     Leading '--/-+'\n");
			
			free(rePart);
			free(imPart);

			return 1;
		}
	}
	else if (buffer[0] == 'i')
	{
		if (inputLength != 1) /* "ia" */
		{
			fprintf(stderr, "[ERROR]     Preceding 'i'\n");
			
			free(rePart);
			free(imPart);

			return 1;
		}
		else
		{
			strcpy(imPart, "1");
			formatType = 3; /* IDENTIFIED: "i" */
		}
	}
	
	if (buffer[inputLength - 1] != 'i' && buffer[0] == '-' && minusCount == 1) /* IDENTIFIED: "-a" */
	{
		aNegFlag = 1;
		formatType = 2;
	}
	else if (iCount == 0 && minusCount == 0) /* IDENTIFIED: "a" */
	{
		formatType = 2;
	}
	else if (iCount == 0) /* "a-b", "a--b" */
	{
		fprintf(stderr, "[ERROR]     Invalid use of '-' operator\n");
		
		free(rePart);
		free(imPart);

		return 1;
	}
	else if (minusCount > 1 && buffer[0] != '-') /* "a--i", "a--bi", "a-b-i" */
	{
		fprintf(stderr, "[ERROR]     Invalid use of '-' operator\n");
		
		free(rePart);
		free(imPart);

		return 1;
	}
	else if (minusCount == 1 && plusCount == 1 && buffer[0] != '-') /* "a+-i", "a-+i", "a+-bi", "a+b-i" */
	{
		fprintf(stderr, "[ERROR]     Invalid use of '+/-' operators\n");
		
		free(rePart);
		free(imPart);

		return 1;
	}
	else /* All invalid sequences have been removed */
	{
		char *iPtr = strchr(buffer, 'i');
		
		if (buffer[0] == '-') /* "-bi", "-a-i", "-a+i", "-a-bi", "-a+bi", "-i" */
		{
			if (buffer[inputLength - 1] == 'i' && plusCount == 0 && minusCount == 1) /* "-bi", "-i" */
			{
				bNegFlag = 1;
				formatType = 3;
				
				if (inputLength == 2) /* IDENTIFIED: "-i" */
				{
					strcpy(imPart, "1");
				}
				/* else IDENTIFIED: "-bi" */
			}
			else /* "-a-i", "-a+i", "-a-bi", "-a+bi" */
			{
				aNegFlag = 1;
				
				if (minusCount > 1) /* "-a-i", "-a-bi" */
				{
					bNegFlag = 1;
				
					if (*(iPtr - 1) == '-') /* IDENTIFIED: "-a-i" */
					{
						strcpy(imPart, "1");
						formatType = 2;
					}
					else /* IDENTIFIED: "-a-bi" */
					{
						formatType = 1;
					}
				}
				else /* "-a+i", "-a+bi" */
				{
					if (*(iPtr - 1) == '+') /* IDENTIFIED: "-a+i" */
					{
						strcpy(imPart, "1");
						formatType = 2;
					}
					else /* IDENTIFIED: "-a+bi" */
					{
						formatType = 1;
					}
				}
			}
		}
		else /* "bi", "a-i", "a-bi", "a+i" */
		{
			if (plusCount == 0 && minusCount == 0) /* IDENTIFIED: "bi" */
			{
				formatType = 3;
			}
			else if (minusCount == 1)
			{
				bNegFlag = 1;
				
				if (*(iPtr - 1) == '-') /* IDENTIFIED: "a-i" */
				{
					strcpy(imPart, "1");
					formatType = 2;
				}
				else /* IDENTIFIED: "a-bi" */
				{
					formatType = 1;
				}
			}
			else
			{
				if (*(iPtr - 1) == '+') /* IDENTIFIED: "a+i" */
				{
					strcpy(imPart, "1");
					formatType = 2;
				}
				else /* IDENTIFIED: "a+bi" */
				{
					formatType = 1;
				}
			}
		}
	}
	
	for (i = 0; i < inputLength; ++i) /* Strip operators */
	{
		if (buffer[i] == '+' || buffer[i] == 'i' || buffer[i] == '-')
		{
			buffer[i] = ' '; /* Replace with whitespace for sscanf() */
		}
	}
	
	switch (formatType)
	{
		case 1: /* Valid format will be "a b" */
			if (sscanf(buffer, "%s%s", rePart, imPart) != 2)
			{
				fprintf(stderr, "[ERROR]     Invalid format\n");
				
				free(rePart);
				free(imPart);

				return 1;
			}
				
			break;
		case 2: /* Valid format will be "a" */
			if (sscanf(buffer, "%s", rePart) != 1)
			{
				fprintf(stderr, "[ERROR]     Invalid format\n");
				
				free(rePart);
				free(imPart);

				return 1;
			}
			
			break;
		case 3: /* Valid format will be " " (if just i) or "b" */
			strcpy(rePart, "0");
			
			if (strcmp(imPart, "0") == 0) /* "b" */
			{
				if (sscanf(buffer, "%s", imPart) != 1)
				{
					fprintf(stderr, "[ERROR]     Invalid format\n");
					
					free(rePart);
					free(imPart);

					return 1;
				}
			}
			
			break;
	}
	
	if ((isFloat(rePart) || isFloat(imPart)) == 1)
	{
		fprintf(stderr, "[ERROR]     Numerical part(s) is not a real number\n");
		
		free(rePart);
		free(imPart);

		return 1;
	}
	
	c->re = stringToDouble(rePart);
	if (errno == EINVAL)
	{
		fprintf(stderr, "[ERROR]     Malformed input\n");
		
		free(rePart);
		free(imPart);

		return 1;
	}
	else if (errno == ERANGE)
	{
		fprintf(stderr, "[ERROR]     Number out of range\n");
		
		free(rePart);
		free(imPart);

		return 1;
	}

	free(rePart);
	
	c->im = stringToDouble(imPart);
	if (errno == EINVAL)
	{
		fprintf(stderr, "[ERROR]     Malformed input\n");
		
		free(imPart);

		return 1;
	}
	else if (errno == ERANGE)
	{
		fprintf(stderr, "[ERROR]     Number out of range\n");
		
		free(imPart);

		return 1;
	}

	free(imPart);
	
	if (sqrt(pow(c->re, 2) + pow(c->im, 2)) > ESCAPE_RADIUS)
	{
		fprintf(stderr, "[ERROR]     Number outside of escape radius (c must satisy |c| < %f)\n", ESCAPE_RADIUS);
		
		return 1;
	}
	
	if (aNegFlag == 1)
	{
		c->re = -(c->re);
	}

	if (bNegFlag == 1)
	{
		c->im = -(c->im);
	}

	return 0;
}


int validateInput(char *buffer, int bufferSize) /* Validation of input buffer */
{
	char c;

	if (fgets(buffer, bufferSize, stdin) == NULL) /* If fgets() errors, such as a null input */
	{	
		fprintf(stderr, "[ERROR]     Malformed input\n");
		
		clearerr(stdin);
		
		return 1;
	}

	if (strchr(buffer, '\n') == NULL)
	{
		if (strlen(buffer) < bufferSize - 1) /* CTRL+D has been inputted or similar */
		{
			fprintf(stderr, "[ERROR]     Malformed input\n");
		}
		else /* If input exceeds length of buffer then buffer would not include a newline character */
		{
			while ((c = getchar()) != '\n' && c != EOF) ;
		
			fprintf(stderr, "[ERROR]     Too many characters inputted\n");
		}
	
		clearerr(stdin);
		
		return 1;
	}

	return 0;
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


double stringToDouble(char *floatStr) /* strtod() but with error handling */
{
	char *endptr;
	
	double x; /* Converted number */
	
	errno = 0;

	x = strtod(floatStr, &endptr);
	
	if (errno == ERANGE) /* Out of range */
	{
		return 1;
	}
	else if (endptr == floatStr) /* If endptr still points to the beginning of the string */
	{		
		errno = EINVAL;
		
		return 1;
	}	

	return x;
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