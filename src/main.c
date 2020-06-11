#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/sysinfo.h>

#include "log.h"
#include "parameters.h"
#include "parser.h"


enum GetoptError
{
    OPT_NONE,
    OPT_ERROR,
    OPT_EOPT,
    OPT_ENOARG,
    OPT_EARG,
    OPT_EMANY,
    OPT_EARGC_LOW,
    OPT_EARGC_HIGH
};


int usage(void);
int doubleArgument(double *x, const char *argument, double xMin, double xMax, char optionID);
int uLongArgument(unsigned long int *x, const char *argument, unsigned long int xMin, unsigned long int xMax,
                     char optionID);
int uIntMaxArgument(uintmax_t *x, const char *argument, uintmax_t xMin, uintmax_t xMax, char optionID);
int getoptErrorMessage(enum GetoptError optionError, char shortOption, const char *longOption);


static const char *PROGRAM_NAME;
static const char *OUTPUT_FILE_PATH_DEFAULT = "output.ppm";
static const int DBL_PRINTF_PRECISION = 2;


int main(int argc, char **argv) /* Take in command-line options */
{
    const char *GETOPT_STRING = ":jto:x:X:y:Y:i:r:s:c:h";

    int optionID, argError;
    const struct option longOptions[] =
    {
        {"j", no_argument, NULL, 'j'}, /* Plot a Julia set */
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

    struct PlotCTX parameters;
    char *outputFilePath = NULL;
    
    /* Get number of online processors */
    int processorCount = get_nprocs();

    PROGRAM_NAME = argv[0];

    if ((parameters.type = getPlotType(argc, argv, longOptions)) == PLOT_NONE)
    {
        return getoptErrorMessage(OPT_NONE, 0, NULL);
    }

    if (initialiseParameters(&parameters, parameters.type) != 0)
    {
        return getoptErrorMessage(OPT_ERROR, 0, NULL);
    }
    
    opterr = 0;
    
    while ((optionID = getopt_long(argc, argv, GETOPT_STRING, longOptions, NULL)) != -1)
    {
        argError = PARSER_NONE;

        switch (optionID)
        {
            case 'j':
                break;
            case 't':
                parameters.output = OUTPUT_TERMINAL;
                break;
            case 'o':
                outputFilePath = optarg;
                break;
            case 'x':
                argError = doubleArgument(&(parameters.minimum.re), optarg, COMPLEX_MIN.re, COMPLEX_MAX.re, optionID);
                break;
            case 'X':
                argError = doubleArgument(&(parameters.maximum.re), optarg, COMPLEX_MIN.re, COMPLEX_MAX.re, optionID);
                break;
            case 'y':
                argError = doubleArgument(&(parameters.minimum.im), optarg, COMPLEX_MIN.im, COMPLEX_MAX.im, optionID);
                break;
            case 'Y':
                argError = doubleArgument(&(parameters.maximum.im), optarg, COMPLEX_MIN.im, COMPLEX_MAX.im, optionID);
                break;
            case 'i':
                argError = uLongArgument(&(parameters.iterations), optarg, ITERATIONS_MIN, ITERATIONS_MAX, optionID);
                break;
            case 'r':
                argError = uIntMaxArgument(&(parameters.width), optarg, WIDTH_MIN, WIDTH_MAX, optionID);
                break;
            case 's':
                argError = uIntMaxArgument(&(parameters.height), optarg, HEIGHT_MIN, HEIGHT_MAX, optionID);
                break;
            case 'c':
                argError = uLongArgument(&(parameters.colour), optarg, COLOUR_SCHEME_TYPE_MIN,
                             COLOUR_SCHEME_TYPE_MAX, optionID);
                break;
            case 'h':
                return usage();
            case '?':
                return getoptErrorMessage(OPT_EOPT, optopt, argv[optind - 1]);
            case ':':
                return getoptErrorMessage(OPT_ENOARG, optopt, NULL);
            default:
                return getoptErrorMessage(OPT_ERROR, 0, NULL);
        }

        if (argError == PARSER_ERANGE)
        {
            return getoptErrorMessage(OPT_NONE, 0, NULL);
        }
        else if (argError != PARSER_NONE)
        {
            return getoptErrorMessage(OPT_EARG, optionID, NULL);
        }
    }

    if (parameters.output != OUTPUT_TERMINAL)
    {
        if (outputFilePath == NULL)
        {
            outputFilePath = OUTPUT_FILE_PATH_DEFAULT;
        }

        if ((parameters.file = fopen(outputFilePath, "w")) == NULL)
        {
            fprintf(stderr, "%s: %s: Could not open file\n", PROGRAM_NAME, outputFilePath);
            return getoptErrorMessage(OPT_NONE, 0, NULL);
        }
    }
    else
    {
        parameters.file = stdout;
    }
    
    
    if (validateParameters(parameters) != 0)
    {
        return getoptErrorMessage(OPT_NONE, 0, NULL);
    }
    

    return EXIT_SUCCESS;
}


enum PlotType getPlotType(int argc, char **argv, const struct option longOptions[])
{
    const enum PlotType PLOT_TYPE_DEFAULT = PLOT_MANDELBROT;
    enum PlotType type = PLOT_NONE;

    int optionID;

    while ((optionID = getopt_long(argc, argv, "j", longOptions, NULL)) != -1)
    {
        if (optionID == 'j')
        {
            type = PLOT_JULIA;
            break;
        }
    }

    optind = 1;

    if (type == PLOT_NONE)
    {
        type = PLOT_TYPE_DEFAULT;
    }

    return type;
}


int usage(void)
{
    printf("Usage: %s [-j] [-t|[-o FILENAME --colour=SCHEME]] [PLOT PARAMETERS...]\n", PROGRAM_NAME);
    printf("       %s --help\n\n", PROGRAM_NAME);
    printf("A Mandelbrot and Julia set plotter.\n\n");
    printf("Mandatory arguments to long options are mandatory for short options too.\n");
    printf("Plot type:\n");
    printf("  -j                           Julia set (c variable is prompted for during execution)\n");
    printf("Output formatting:\n");
    printf("  -t                           Output to terminal using ASCII characters as shading\n");
    printf("  -o FILENAME                  Specify output file name\n");
    printf("                               [+] Default = \'%s\'\n", OUTPUT_FILE_PATH_DEFAULT);
    printf("  -c SCHEME, --colour=SCHEME   Specify colour palette to use\n");
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
    printf("                               [+] Others are full colour with a bit depth of 24 bits\n\n");
    printf("Plot parameters:\n");
    printf("  -x XMIN,   --xmin=XMIN       Minimum x (real) value to plot\n");
    printf("  -X XMAX,   --xmax=XMAX       Maximum x (real) value to plot\n");
    printf("  -y YMIN,   --ymin=YMIN       Minimum y (imaginary) value to plot\n");
    printf("  -Y YMAX,   --ymax=YMAX       Maximum y (imaginary) value to plot\n");
    printf("  -i NMAX,   --itermax=NMAX    The maximum number of function iterations before a number is deemed to be within the set\n");
    printf("                               [+] A larger maximum leads to a preciser plot but increases computation time\n");
    printf("                               [+] Default = %d\n", DEFAULT_ITERMAX);
    printf("  -r WIDTH,  --width=WIDTH     The width of the PPM file in pixels\n");
    printf("                               [+] If colour scheme 1 or 2, WIDTH must be a multiple of 8 to allow for bit-width pixels\n");
    printf("  -s HEIGHT, --height=HEIGHT   The height of the PPM file in pixels\n\n");
    printf("  Default parameters:\n");
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
    printf("    HEIGHT = %d\n\n", DEFAULT_HEIGHT);
    printf("Miscellaneous:\n");
    printf("  -h,        --help            Display this help message and exit\n\n\n");
    printf("Examples:\n");
    printf("  mandelbrot -m\n");
    printf("  mandelbrot -j -p -o \"juliaset.ppm\"\n");
    printf("  mandelbrot -m -t\n");
    printf("  mandelbrot -m -p --itermax=200 --width=5500 --height=5000 --colour=7\n\n");

    return EXIT_SUCCESS;
}


/* Wrapper for stringToDouble() */
int doubleArgument(double *x, const char *argument, double xMin, double xMax, char optionID)
{
    int argError;

    argError = stringToDouble(argument, x, xMin, xMax);
                
    if (argError != PARSER_NONE)
    {
        if (argError == PARSER_ERANGE || argError == PARSER_EMIN || argError == PARSER_EMAX)
        {
            fprintf(stderr, "%s: -%c: Argument out of range, it must be between %.*g and %.*g\n", 
                PROGRAM_NAME, optionID, DBL_PRINTF_PRECISION, xMin, DBL_PRINTF_PRECISION, xMax);
            return PARSER_ERANGE;
        }

        return PARSER_ERROR;
    }

    return PARSER_NONE;
}


/* Wrapper for stringToULong() */
int uLongArgument(unsigned long int *x, const char *argument, unsigned long int xMin, unsigned long int xMax,
                     char optionID)
{
    const int BASE = 10;
    int argError;

    argError = stringToULong(argument, x, xMin, xMax, BASE);
                
    if (argError != PARSER_NONE)
    {
        if (argError == PARSER_ERANGE || argError == PARSER_EMIN || argError == PARSER_EMAX)
        {
            fprintf(stderr, "%s: -%c: Argument out of range, it must be between %lu and %lu\n", 
                PROGRAM_NAME, optionID, xMin, xMax);
            return PARSER_ERANGE;
        }

        return PARSER_ERROR;
    }

    return PARSER_NONE;
}


/* Wrapper for stringToUIntMax() */
int uIntMaxArgument(uintmax_t *x, const char *argument, uintmax_t xMin, uintmax_t xMax, char optionID)
{
    const int BASE = 10;
    int argError;

    argError = stringToUIntMax(argument, x, xMin, xMax, BASE);
                
    if (argError != PARSER_NONE)
    {
        if (argError == PARSER_ERANGE || argError == PARSER_EMIN || argError == PARSER_EMAX)
        {
            fprintf(stderr, "%s: -%c: Argument out of range, it must be between %" PRIuMAX " and %" PRIuMAX "\n", 
                PROGRAM_NAME, optionID, xMin, xMax);
            return PARSER_ERANGE;
        }

        return PARSER_ERROR;
    }

    return PARSER_NONE;
}


/* Convert error code to message when parsing command line arguments */
int getoptErrorMessage(enum GetoptError optionError, char shortOption, const char *longOption)
{
    switch (optionError)
    {
        case OPT_NONE:
            break;
        case OPT_ERROR:
            fprintf(stderr, "%s: Unknown error when reading command-line options\n", PROGRAM_NAME);
            break;
        case OPT_EOPT:
            if (shortOption == 0)
            {
                fprintf(stderr, "%s: Invalid option: \'%s\'\n", PROGRAM_NAME, longOption);
            }
            else
            {
                fprintf(stderr, "%s: Invalid option: \'-%c\'\n", PROGRAM_NAME, shortOption);
            }
            break;
        case OPT_ENOARG:
            fprintf(stderr, "%s: -%c: Option argument required\n", PROGRAM_NAME, shortOption);
            break;
        case OPT_EARG:
            fprintf(stderr, "%s: -%c: Failed to parse argument\n", PROGRAM_NAME, shortOption);
            break;
        case OPT_EMANY:
            fprintf(stderr, "%s: -%c: Option can only appear once\n", PROGRAM_NAME, shortOption);
            break;
        case OPT_EARGC_LOW:
            fprintf(stderr, "%s: Too few arguments supplied\n", PROGRAM_NAME);
            break;
        case OPT_EARGC_HIGH:
            fprintf(stderr, "%s: Too many arguments supplied\n", PROGRAM_NAME);
            break;
        default:
            break;
    }

    fprintf(stderr, "Try \'%s --help\' for more information\n", PROGRAM_NAME);
    return EXIT_FAILURE;
}


/* Check user-supplied parameters */
int validateParameters(struct PlotCTX parameters)
{
    const unsigned int ITERATIONS_RECOMMENDED_MIN = 50;
    const unsigned int ITERATIONS_RECOMMENDED_MAX = 200;
    
    if (parameters.maximum.re <= parameters.minimum.re)
    {
        fprintf(stderr, "%s: Invalid real number range\n", PROGRAM_NAME);
        return 1;
    }
    else if (parameters.maximum.im <= parameters.minimum.im)
    {
        fprintf(stderr, "%s: Invalid imaginary number range\n", PROGRAM_NAME);
        return 1;
    }

    if (parameters.colour.depth == BIT_DEPTH_1 && parameters.width % CHAR_BIT != 0)
    {
        parameters.width = parameters.width + CHAR_BIT - (parameters.width % CHAR_BIT);
        logMessage(WARNING, "For 1-bit pixel colour schemes, the width must be a multiple of %zu. Width set to %zu",
            CHAR_BIT, parameters.width);
    }

    if (parameters.iterations < ITERATIONS_RECOMMENDED_MIN)
    {
        logMessage(WARNING, "Maximum iteration count is low and may argError in an imprecise plot, "
            "recommended range is %u to %u", ITERATIONS_RECOMMENDED_MIN, ITERATIONS_RECOMMENDED_MAX);
    }
    else if (parameters.iterations > ITERATIONS_RECOMMENDED_MAX)
    {
        logMessage(WARNING, "Maximum iteration count is high and may needlessly use resources without a "
            "noticeable precision increase, recommended range is %u to %u",
            ITERATIONS_RECOMMENDED_MIN, ITERATIONS_RECOMMENDED_MAX);
    }

    if (parameters.minimum.re < -(ESCAPE_RADIUS) || parameters.minimum.im < -(ESCAPE_RADIUS) ||
           parameters.maximum.re > ESCAPE_RADIUS || parameters.maximum.im > ESCAPE_RADIUS)
    {
        logMessage(WARNING, "The sample set of complex numbers extends outside (%.*g, %.*g) to (%.*g, %.*g) "
            "- the set in which the Mandelbrot/Julia sets are guaranteed to be contained within",
            DBL_PRINTF_PRECISION, -(ESCAPE_RADIUS), DBL_PRINTF_PRECISION, -(ESCAPE_RADIUS),
            DBL_PRINTF_PRECISION, ESCAPE_RADIUS, DBL_PRINTF_PRECISION, ESCAPE_RADIUS);
    }

    return 0;
}