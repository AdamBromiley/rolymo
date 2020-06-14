#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

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


enum PlotType getPlotType(int argc, char **argv, const struct option longOptions[], const char *getoptString);

int usage(void);

int uLongArgument(unsigned long int *x, const char *argument, unsigned long int min, unsigned long int max,
                     char optionID);
int uIntMaxArgument(uintmax_t *x, const char *argument, uintmax_t min, uintmax_t max, char optionID);
int doubleArgument(double *x, const char *argument, double min, double max, char optionID);
int complexArgument(struct ComplexNumber *z, char *argument, struct ComplexNumber min, struct ComplexNumber max,
                       char optionID);

int validateParameters(struct PlotCTX parameters);

int getoptErrorMessage(enum GetoptError optionError, char shortOption, const char *longOption);


static const char *PROGRAM_NAME;
static const char *OUTPUT_FILE_PATH_DEFAULT = "./output.ppm";
static const char *LOG_FILE_PATH_DEFAULT = "./var/mandelbrot.log";
static const int DBL_PRINTF_PRECISION = 3;


int main(int argc, char **argv) /* Take in command-line options */
{
    const char *GETOPT_STRING = ":c:i:j:l:m:M:o:r:s:tv";

    int optionID, argError;
    const struct option longOptions[] =
    {
        {"colour", required_argument, NULL, 'c'}, /* Colour scheme of PPM image */
        {"iterations", required_argument, NULL, 'i'}, /* Maximum iteration count of calculation */
        {"julia", required_argument, NULL, 'j'}, /* Plot a Julia set and specify constant */
        {"log", optional_argument, NULL, 'k'}, /* Output log file */
        {"log-level", required_argument, NULL, 'l'}, /* Minimum log level to output */
        {"min", required_argument, NULL, 'm'}, /* Minimum/maximum X and Y coordinates */
        {"max", required_argument, NULL, 'M'},
        {"o", required_argument, NULL, 'o'}, /* Output PPM file name */
        {"width", required_argument, NULL, 'r'}, /* X and Y resolution of output PPM file */
        {"height", required_argument, NULL, 's'},
        {"t", no_argument, NULL, 't'}, /* Output to terminal */
        {"verbose", no_argument, NULL, 'v'}, /* Output log to stderr */
        {"help", no_argument, NULL, 'h'}, /* For guidance on command-line options */
        {0, 0, 0, 0}
    };

    struct PlotCTX parameters;
    const char *outputFilePath = OUTPUT_FILE_PATH_DEFAULT;
    const char *logFilePath = NULL;
    enum LogSeverity loggingLevel = INFO;
    enum Verbosity verbose = VERBOSE;

    PROGRAM_NAME = argv[0];

    parameters.type = getPlotType(argc, argv, longOptions, GETOPT_STRING);

    if (parameters.type == PLOT_NONE)
        return getoptErrorMessage(OPT_NONE, 0, NULL);

    if (initialiseParameters(&parameters, parameters.type))
        return getoptErrorMessage(OPT_ERROR, 0, NULL);
    
    opterr = 0;
    while ((optionID = getopt_long(argc, argv, GETOPT_STRING, longOptions, NULL)) != -1)
    {
        argError = PARSER_NONE;

        switch (optionID)
        {
            case 'c':
                /* Cast is safe assuming COLOUR_SCHEME_TYPE_MAX <= INT_MAX */
                argError = uLongArgument((unsigned long *) &(parameters.colour.colour), optarg, COLOUR_SCHEME_TYPE_MIN,
                             COLOUR_SCHEME_TYPE_MAX, optionID);
                initialiseColourScheme(&(parameters.colour), parameters.colour.colour);
                break;
            case 'i':
                argError = uLongArgument(&(parameters.iterations), optarg, ITERATIONS_MIN, ITERATIONS_MAX, optionID);
                break;
            case 'j':
                argError = complexArgument(&(parameters.c), optarg, C_MIN, C_MAX, optionID);
                break;
            case 'k':
                verbose = QUIET;
                if (optarg)
                    logFilePath = optarg;
                else
                    logFilePath = LOG_FILE_PATH_DEFAULT;                
                break;
            case 'l':
                /* Cast is safe assuming LOG_SEVERITY_MAX <= INT_MAX */
                argError = uLongArgument((unsigned long *) &loggingLevel, optarg, LOG_SEVERITY_MIN, LOG_SEVERITY_MAX, optionID);
                break;
            case 'm':
                argError = complexArgument(&(parameters.minimum), optarg, COMPLEX_MIN, COMPLEX_MAX, optionID);
                break;
            case 'M':
                argError = complexArgument(&(parameters.maximum), optarg, COMPLEX_MIN, COMPLEX_MAX, optionID);
                break;
            case 'o':
                outputFilePath = optarg;
                break;
            case 'r':
                argError = uIntMaxArgument(&(parameters.width), optarg, WIDTH_MIN, WIDTH_MAX, optionID);
                break;
            case 's':
                argError = uIntMaxArgument(&(parameters.height), optarg, HEIGHT_MIN, HEIGHT_MAX, optionID);
                break;
            case 't':
                parameters.output = OUTPUT_TERMINAL;
                break;
            case 'v':
                verbose = VERBOSE;
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
            return getoptErrorMessage(OPT_NONE, 0, NULL);
        else if (argError != PARSER_NONE)
            return getoptErrorMessage(OPT_EARG, optionID, NULL);
    }

    if (initialiseLog(verbose, loggingLevel, logFilePath))
        return EXIT_FAILURE;

    if (parameters.output != OUTPUT_TERMINAL)
    {
        parameters.file = fopen(outputFilePath, "w");

        if (!parameters.file)
        {
            fprintf(stderr, "%s: %s: Could not open file\n", PROGRAM_NAME, outputFilePath);
            return getoptErrorMessage(OPT_NONE, 0, NULL);
        }
    }
    else
    {
        initialiseTerminalOutputParameters(&parameters);
    }
    
    if (validateParameters(parameters))
        return getoptErrorMessage(OPT_NONE, 0, NULL);

    printf("   Program Settings:\n");
    printf("    VERBOSITY  = %d\n", verbose);
    printf("    LOG FILE   = %s\n", logFilePath);
    printf("   Plot Parameters:\n");
    printf("    PLOT TYPE  = %d\n", parameters.type);
    printf("    MIN        = %.*g + %.*gi\n", DBL_PRINTF_PRECISION, parameters.minimum.re, DBL_PRINTF_PRECISION, parameters.minimum.im);
    printf("    MAX        = %.*g + %.*gi\n", DBL_PRINTF_PRECISION, parameters.maximum.re, DBL_PRINTF_PRECISION, parameters.maximum.im);
    printf("    C          = %.*g + %.*gi\n", DBL_PRINTF_PRECISION, parameters.c.re, DBL_PRINTF_PRECISION, parameters.c.im);
    printf("    ITERATIONS = %lu\n", parameters.iterations);
    printf("    OUTPUT     = %d\n", parameters.output);
    printf("    FILE PATH  = %s\n", outputFilePath);
    printf("    WIDTH      = %zu\n", parameters.width);
    printf("    HEIGHT     = %zu\n", parameters.height);
    printf("    COLOUR     = {\n");
    printf("                  scheme:       %d,\n", parameters.colour.colour);
    printf("                  depth:        %d,\n", parameters.colour.depth);
    printf("                  function ptr: -}\n");
    printf("                 }\n");

    return EXIT_SUCCESS;
}


enum PlotType getPlotType(int argc, char **argv, const struct option longOptions[], const char *getoptString)
{
    const enum PlotType PLOT_TYPE_DEFAULT = PLOT_MANDELBROT;
    enum PlotType type = PLOT_NONE;

    int optionID;

    while ((optionID = getopt_long(argc, argv, getoptString, longOptions, NULL)) != -1)
    {
        if (optionID == 'j')
        {
            type = PLOT_JULIA;
            break;
        }
    }

    optind = 1;

    return (type == PLOT_NONE) ? PLOT_TYPE_DEFAULT : type;
}


int usage(void)
{
    printf("Usage: %s [-j -z CONSTANT] [-t|[[-o FILENAME] [-c SCHEME]]] [PLOT PARAMETERS...]\n", PROGRAM_NAME);
    printf("       %s --help\n\n", PROGRAM_NAME);
    printf("A Mandelbrot and Julia set plotter.\n\n");
    printf("Mandatory arguments to long options are mandatory for short options too.\n");
    printf("Plot type:\n");
    printf("  -j CONSTANT                   Julia set with specified constant parameter\n");
    printf("Output formatting:\n");
    printf("  -t                            Output to terminal using ASCII characters as shading\n");
    printf("  -o FILENAME                   Specify output file name\n");
    printf("                                [+] Default = \'%s\'\n", OUTPUT_FILE_PATH_DEFAULT);
    printf("  -c SCHEME, --colour=SCHEME    Specify colour palette to use\n");
    printf("                                [+] Default = 0\n");
    printf("                                [+] SCHEME may be:\n");
    printf("                                    0 = All hues\n");
    printf("                                    1 = Black and white - pixels in the set are black\n");
    printf("                                    2 = White and black - pixels in the set are white\n");
    printf("                                    3 = Grayscale\n");
    printf("                                    4 = Red and white\n");
    printf("                                    5 = Fire\n");
    printf("                                    6 = Vibrant - All hues but with maximum saturation\n");
    printf("                                    7 = Red hot\n");
    printf("                                    8 = Matrix\n");
    printf("                                [+] 1 and 2 have a bit depth of 1\n");
    printf("                                [+] 3 has a bit depth of 8 bits\n");
    printf("                                [+] Others are full colour with a bit depth of 24 bits\n\n");
    printf("Plot parameters:\n");
    printf("  -m MIN,    --min=MIN          Minimum value to plot\n");
    printf("  -M MAX,    --max=MAX          Maximum value to plot\n");
    printf("  -i NMAX,   --iterations=NMAX  The maximum number of function iterations before a number is deemed to be within the set\n");
    printf("                                [+] A larger maximum leads to a preciser plot but increases computation time\n");
    printf("                                [+] Default = %u\n", ITERATIONS_DEFAULT);
    printf("  -r WIDTH,  --width=WIDTH      The width of the PPM file in pixels\n");
    printf("                                [+] If colour scheme 1 or 2, WIDTH must be a multiple of 8 to allow for bit-width pixels\n");
    printf("  -s HEIGHT, --height=HEIGHT    The height of the PPM file in pixels\n\n");
    printf("  Default parameters:\n");
    printf("   Julia Set:\n");
    printf("    MIN    = %.*g + %.*gi\n", DBL_PRINTF_PRECISION, JULIA_PARAMETERS_DEFAULT.minimum.re, DBL_PRINTF_PRECISION, JULIA_PARAMETERS_DEFAULT.minimum.im);
    printf("    MAX    = %.*g + %.*gi\n", DBL_PRINTF_PRECISION, JULIA_PARAMETERS_DEFAULT.maximum.re, DBL_PRINTF_PRECISION, JULIA_PARAMETERS_DEFAULT.maximum.im);
    printf("    WIDTH  = %zu\n", JULIA_PARAMETERS_DEFAULT.width);
    printf("    HEIGHT = %zu\n\n", JULIA_PARAMETERS_DEFAULT.height);
    printf("   Mandelbrot set:\n");
    printf("    MIN    = %.*g + %.*gi\n", DBL_PRINTF_PRECISION, MANDELBROT_PARAMETERS_DEFAULT.minimum.re, DBL_PRINTF_PRECISION, MANDELBROT_PARAMETERS_DEFAULT.minimum.im);
    printf("    MAX    = %.*g + %.*gi\n", DBL_PRINTF_PRECISION, MANDELBROT_PARAMETERS_DEFAULT.maximum.re, DBL_PRINTF_PRECISION, MANDELBROT_PARAMETERS_DEFAULT.maximum.im);
    printf("    WIDTH  = %zu\n", MANDELBROT_PARAMETERS_DEFAULT.width);
    printf("    HEIGHT = %zu\n\n", MANDELBROT_PARAMETERS_DEFAULT.height);
    printf("Miscellaneous:\n");
    printf("             --help             Display this help message and exit\n\n\n");
    printf("Examples:\n");
    printf("  mandelbrot -m\n");
    printf("  mandelbrot -j -p -o \"juliaset.ppm\"\n");
    printf("  mandelbrot -m -t\n");
    printf("  mandelbrot -m -p --itermax=200 --width=5500 --height=5000 --colour=7\n\n");

    return EXIT_SUCCESS;
}


/* Wrapper for stringToULong() */
int uLongArgument(unsigned long int *x, const char *argument, unsigned long int min, unsigned long int max,
                     char optionID)
{
    char *endptr;
    const int BASE = 10;
    int argError;

    argError = stringToULong(x, argument, min, max, &endptr, BASE);

    if (argError == PARSER_ERANGE || argError == PARSER_EMIN || argError == PARSER_EMAX)
    {
        fprintf(stderr, "%s: -%c: Argument out of range, it must be between %lu and %lu\n", 
            PROGRAM_NAME, optionID, min, max);
        return PARSER_ERANGE;
    }
    else if (argError != PARSER_NONE)
    {
        return PARSER_ERROR;
    }

    return PARSER_NONE;
}


/* Wrapper for stringToUIntMax() */
int uIntMaxArgument(uintmax_t *x, const char *argument, uintmax_t min, uintmax_t max, char optionID)
{
    char *endptr;
    const int BASE = 10;
    int argError;

    argError = stringToUIntMax(x, argument, min, max, &endptr, BASE);

    if (argError == PARSER_ERANGE || argError == PARSER_EMIN || argError == PARSER_EMAX)
    {
        fprintf(stderr, "%s: -%c: Argument out of range, it must be between %" PRIuMAX " and %" PRIuMAX "\n", 
            PROGRAM_NAME, optionID, min, max);
        return PARSER_ERANGE;
    }
    else if (argError != PARSER_NONE)
    {
        return PARSER_ERROR;
    }

    return PARSER_NONE;
}


/* Wrapper for stringToDouble() */
int doubleArgument(double *x, const char *argument, double min, double max, char optionID)
{
    char *endptr;
    int argError;

    argError = stringToDouble(x, argument, min, max, &endptr);

    if (argError == PARSER_ERANGE || argError == PARSER_EMIN || argError == PARSER_EMAX)
    {
        fprintf(stderr, "%s: -%c: Argument out of range, it must be between %.*g and %.*g\n", 
            PROGRAM_NAME, optionID, DBL_PRINTF_PRECISION, min, DBL_PRINTF_PRECISION, max);
        return PARSER_ERANGE;
    }
    else if (argError != PARSER_NONE)
    {
        return PARSER_ERROR;
    }

    return PARSER_NONE;
}


/* Wrapper for stringToDouble() */
int complexArgument(struct ComplexNumber *z, char *argument, struct ComplexNumber min, struct ComplexNumber max,
                       char optionID)
{
    char *endptr;
    int argError;

    argError = stringToComplex(z, argument, min, max, &endptr);

    if (argError == PARSER_ERANGE)
    {
        fprintf(stderr, "%s: -%c: Argument out of range, it must be between %.*g + %.*gi and %.*g + %.*gi\n", 
            PROGRAM_NAME, optionID, DBL_PRINTF_PRECISION, min.re, DBL_PRINTF_PRECISION, min.im,
            DBL_PRINTF_PRECISION, max.re, DBL_PRINTF_PRECISION, max.im);
        return PARSER_ERANGE;
    }
    else if (argError != PARSER_NONE)
    {
        return PARSER_ERROR;
    }

    return PARSER_NONE;
}


/* Check user-supplied parameters */
int validateParameters(struct PlotCTX parameters)
{
    const unsigned int ITERATIONS_RECOMMENDED_MIN = 50;
    const unsigned int ITERATIONS_RECOMMENDED_MAX = 200;
    
    if (parameters.maximum.re <= parameters.minimum.re)
    {
        fprintf(stderr, "%s: Invalid range - maximum real value is smaller than the minimum\n", PROGRAM_NAME);
        return 1;
    }
    else if (parameters.maximum.im <= parameters.minimum.im)
    {
        fprintf(stderr, "%s: Invalid range - maximum imaginary value is smaller than the minimum\n", PROGRAM_NAME);
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
            if (shortOption == '\0')
                fprintf(stderr, "%s: Invalid option: \'%s\'\n", PROGRAM_NAME, longOption);
            else
                fprintf(stderr, "%s: Invalid option: \'-%c\'\n", PROGRAM_NAME, shortOption);
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