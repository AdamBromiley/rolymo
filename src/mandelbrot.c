#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "groot/include/log.h"
#include "percy/include/parser.h"

#include "image.h"
#include "mandelbrot_parameters.h"
#include "parameters.h"


#define SEVERITY_STR_LEN_MAX 16

#define BIT_DEPTH_STR_LEN_MAX 16
#define COMPLEX_STR_LEN_MAX 32


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


/* Program name - argv[0] */
static char *programName;

/* Output files */
static char *OUTPUT_FILEPATH_DEFAULT = "var/mandelbrot.pnm";

static char *LOG_FILEPATH_DEFAULT = "var/mandelbrot.log";

/* Precision of floating points in output */
static const int DBL_PRINTF_PRECISION = 3;


enum PlotType getPlotType(int argc, char **argv, const struct option longOptions[], const char *getoptString);

int usage(void);
void plotParameters(struct PlotCTX *parameters, const char *image);
void programParameters(const char *log);

int uLongArgument(unsigned long *x, const char *argument, unsigned long min, unsigned long max, char optionID);
int uIntMaxArgument(uintmax_t *x, const char *argument, uintmax_t min, uintmax_t max, char optionID);
int doubleArgument(double *x, const char *argument, double min, double max, char optionID);
int complexArgument(complex *z, char *argument, complex min, complex max, char optionID);

int validateParameters(struct PlotCTX parameters);

int getoptErrorMessage(enum GetoptError optionError, char shortOption, const char *longOption);


/* Process command-line options */
int main(int argc, char **argv)
{
    const char *GETOPT_STRING = ":c:i:j:l:m:M:o:r:s:tv";

    /* Temporary variable for memory safety with uLongArgument() */
    unsigned long tempUL;
    int optionID;
    const struct option LONG_OPTIONS[] =
    {
        {"colour", required_argument, NULL, 'c'},     /* Colour scheme of PPM image */
        {"iterations", required_argument, NULL, 'i'}, /* Maximum iteration count of function */
        {"julia", required_argument, NULL, 'j'},      /* Plot a Julia set and specify constant */
        {"log", optional_argument, NULL, 'k'},        /* Output log to file (with optional path) */
        {"log-level", required_argument, NULL, 'l'},  /* Minimum log level to output */
        {"min", required_argument, NULL, 'm'},        /* Minimum/maximum X and Y coordinates */
        {"max", required_argument, NULL, 'M'},
        {"o", required_argument, NULL, 'o'},          /* Output image file name */
        {"width", required_argument, NULL, 'r'},      /* X and Y dimensions of plot */
        {"height", required_argument, NULL, 's'},
        {"t", no_argument, NULL, 't'},                /* Output plot to stdout */
        {"verbose", no_argument, NULL, 'v'},          /* Output log to stderr */
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };

    /* Plotting parameters */
    struct PlotCTX parameters;

    /* Image file path */
    char *outputFilepath = OUTPUT_FILEPATH_DEFAULT;

    /* Log parameters */
    char *logFilepath = NULL;
    bool vFlag = false;

    programName = argv[0];
    
    /* Do one getopt pass to get the plot type */
    parameters.type = getPlotType(argc, argv, LONG_OPTIONS, GETOPT_STRING);

    if (parameters.type == PLOT_NONE)
        return getoptErrorMessage(OPT_NONE, 0, NULL);

    /* Fill parameters struct with default values for the plot type */
    if (initialiseParameters(&parameters, parameters.type))
        return getoptErrorMessage(OPT_ERROR, 0, NULL);

    /* Parse options */
    opterr = 0;
    while ((optionID = getopt_long(argc, argv, GETOPT_STRING, LONG_OPTIONS, NULL)) != -1)
    {
        enum ParserErrorCode argError = PARSER_NONE;

        switch (optionID)
        {
            case 'c':
                argError = uLongArgument(&tempUL, optarg, COLOUR_SCHEME_MIN, COLOUR_SCHEME_MAX, optionID);
                parameters.colour.colour = tempUL;
                initialiseColourScheme(&(parameters.colour), parameters.colour.colour);
                break;
            case 'i':
                argError = uLongArgument(&(parameters.iterations), optarg, ITERATIONS_MIN, ITERATIONS_MAX, optionID);
                break;
            case 'j':
                argError = complexArgument(&(parameters.c), optarg, C_MIN, C_MAX, optionID);
                break;
            case 'k':
                if (!vFlag)
                    setLogVerbosity(false);

                if (optarg)
                    logFilepath = optarg;
                else
                    logFilepath = LOG_FILEPATH_DEFAULT;                
                break;
            case 'l':
                argError = uLongArgument(&tempUL, optarg, LOG_SEVERITY_MIN, LOG_SEVERITY_MAX, optionID);
                setLogLevel(tempUL);
                break;
            case 'm':
                argError = complexArgument(&(parameters.minimum), optarg, COMPLEX_MIN, COMPLEX_MAX, optionID);
                break;
            case 'M':
                argError = complexArgument(&(parameters.maximum), optarg, COMPLEX_MIN, COMPLEX_MAX, optionID);
                break;
            case 'o':
                outputFilepath = optarg;
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
                vFlag = true;
                setLogVerbosity(true);
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

    /* Open log file */
    if (logFilepath)
    {
        if (openLog(logFilepath))
            return getoptErrorMessage(OPT_NONE, 0, NULL);
    }

    /* If stdout output, change dimensions */
    if (parameters.output == OUTPUT_TERMINAL)
        initialiseTerminalOutputParameters(&parameters);

    /* Check and warn against some parameters */
    if (validateParameters(parameters))
        return getoptErrorMessage(OPT_NONE, 0, NULL);

    /* Output settings */
    programParameters(logFilepath);
    plotParameters(&parameters, outputFilepath);

    /* Open image file and write header */
    if (initialiseImage(&parameters, outputFilepath))
        return EXIT_FAILURE;
    
    /* Produce plot */
    if (imageOutput(&parameters))
        return EXIT_FAILURE;

    /* Close file */
    if (closeImage(&parameters))
        return EXIT_FAILURE;

    /* Close log file */
    if (closeLog())
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}


/* Do one getopt pass to get the plot type (default is Mandelbrot) */
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


/* `--help` output */
int usage(void)
{
    printf("Usage: %s [LOG PARAMETERS...] [OUTPUT PARAMETERS...] [-j CONSTANT] [PLOT PARAMETERS...]\n", programName);
    printf("       %s --help\n\n", programName);
    printf("A Mandelbrot and Julia set plotter.\n\n");
    printf("Mandatory arguments to long options are mandatory for short options too.\n");
    printf("Output parameters:\n");
    printf("  -c SCHEME, --colour=SCHEME    Specify colour palette to use\n");
    printf("                                [+] Default = %d\n", COLOUR_SCHEME_DEFAULT);
    printf("                                [+] SCHEME may be:\n");
    printf("                                    2  = Black and white\n");
    printf("                                    3  = White and black\n");
    printf("                                    4  = Greyscale\n");
    printf("                                    5  = Rainbow\n");
    printf("                                    6  = Vibrant rainbow\n");
    printf("                                    7  = Red and white\n");
    printf("                                    8  = Fire\n");
    printf("                                    9  = Red hot\n");
    printf("                                    10 = Matrix\n");
    printf("                                [+] Black and white schemes are 1-bit\n");
    printf("                                [+] Greyscale schemes are 8-bit\n");
    printf("                                [+] Coloured schemes are full 24-bit\n\n");
    printf("  -o FILE                       Output file name\n");
    printf("                                [+] Default = \'%s\'\n", OUTPUT_FILEPATH_DEFAULT);
    printf("  -r WIDTH,  --width=WIDTH      The width of the image file in pixels\n");
    printf("                                [+] If using a 1-bit colour scheme, WIDTH must be a multiple of %u to "
              "allow for\n                                    bit-width pixels\n", CHAR_BIT);
    printf("  -s HEIGHT, --height=HEIGHT    The height of the image file in pixels\n");
    printf("  -t                            Output to stdout using ASCII characters as shading\n");
    printf("Plot type:\n");
    printf("  -j CONSTANT                   Plot Julia set with specified constant parameter\n");
    printf("Plot parameters:\n");
    printf("  -m MIN,    --min=MIN          Minimum value to plot\n");
    printf("  -M MAX,    --max=MAX          Maximum value to plot\n");
    printf("  -i NMAX,   --iterations=NMAX  The maximum number of function iterations before a number is deemed to be "
              "within the set\n");
    printf("                                [+] Default = %u\n", ITERATIONS_DEFAULT);
    printf("                                [+] A larger maximum leads to a preciser plot but increases computation "
              "time\n\n");
    printf("  Default parameters:\n");
    printf("    Julia Set:\n");
    printf("      MIN    = %.*g + %.*gi\n", DBL_PRINTF_PRECISION, creal(JULIA_PARAMETERS_DEFAULT.minimum),
        DBL_PRINTF_PRECISION, cimag(JULIA_PARAMETERS_DEFAULT.minimum));
    printf("      MAX    = %.*g + %.*gi\n", DBL_PRINTF_PRECISION, creal(JULIA_PARAMETERS_DEFAULT.maximum),
        DBL_PRINTF_PRECISION, cimag(JULIA_PARAMETERS_DEFAULT.maximum));
    printf("      WIDTH  = %zu\n", JULIA_PARAMETERS_DEFAULT.width);
    printf("      HEIGHT = %zu\n\n", JULIA_PARAMETERS_DEFAULT.height);
    printf("    Mandelbrot set:\n");
    printf("      MIN    = %.*g + %.*gi\n", DBL_PRINTF_PRECISION, creal(MANDELBROT_PARAMETERS_DEFAULT.minimum),
        DBL_PRINTF_PRECISION, cimag(MANDELBROT_PARAMETERS_DEFAULT.minimum));
    printf("      MAX    = %.*g + %.*gi\n", DBL_PRINTF_PRECISION, creal(MANDELBROT_PARAMETERS_DEFAULT.maximum),
        DBL_PRINTF_PRECISION, cimag(MANDELBROT_PARAMETERS_DEFAULT.maximum));
    printf("      WIDTH  = %zu\n", MANDELBROT_PARAMETERS_DEFAULT.width);
    printf("      HEIGHT = %zu\n\n", MANDELBROT_PARAMETERS_DEFAULT.height);
    printf("Miscellaneous:\n");
    printf("  --log[=FILE]                  Output log to file, with optional file path argument\n");
    printf("                                [+] Default = \'%s\'\n", LOG_FILEPATH_DEFAULT);
    printf("                                [+] Option may be used with \'-v\'\n");
    printf("  -l LEVEL,  --log-level=LEVEL  Only log messages more severe than LEVEL\n");
    printf("                                [+] Default = INFO\n");
    printf("                                [+] LEVEL may be:\n");
    printf("                                    0  = NONE (log nothing)\n");
    printf("                                    1  = FATAL\n");
    printf("                                    2  = ERROR\n");
    printf("                                    3  = WARNING\n");
    printf("                                    4  = INFO\n");
    printf("                                    5  = DEBUG\n");
    printf("  -v,        --verbose          Redirect log to stderr\n");
    printf("             --help             Display this help message and exit\n\n");
    printf("Examples:\n");
    printf("  mandelbrot\n");
    printf("  mandelbrot -j \"0.1 - 0.2e-2i\" -o \"juliaset.pnm\"\n");
    printf("  mandelbrot -t\n");
    printf("  mandelbrot -i 200 --width=5500 --height=5000 --colour=9\n\n");

    return EXIT_SUCCESS;
}


/* Print program parameters to log */
void programParameters(const char *log)
{
    char level[SEVERITY_STR_LEN_MAX];

    /* Convert log severity level to string */
    getSeverityString(level, getLogLevel(), sizeof(level));

    logMessage(DEBUG, "Program settings:\n\
    Verbosity = %s\n\
    Log level = %s\n\
    Log file  = %s",
    (getLogVerbosity()) ? "VERBOSE" : "QUIET",
    level,
    (log) ? log : "-"
    );

    return;
}


/* Print plot parameters to log */
void plotParameters(struct PlotCTX *parameters, const char *image)
{
    char output[OUTPUT_STRING_LENGTH_MAX];
    char colour[COLOUR_STRING_LENGTH_MAX];
    char bitDepthString[BIT_DEPTH_STR_LEN_MAX];
    char plot[PLOT_STRING_LENGTH_MAX];
    char c[COMPLEX_STR_LEN_MAX];

    /* Get output type string from output type and bit depth enums */
    getOutputString(output, parameters, sizeof(output));

    /* Convert colour scheme enum to a string */
    getColourString(colour, parameters->colour.colour, sizeof(colour));

    /* Convert bit depth integer to string */
    if (parameters->colour.depth > 0)
    {
        snprintf(bitDepthString, sizeof(bitDepthString), "%d-bit", parameters->colour.depth);
    }
    else
    {
        strncpy(bitDepthString, "-", sizeof(bitDepthString));
        bitDepthString[sizeof(bitDepthString) - 1] = '\0';
    }

    /* Get plot type string from PlotType enum */
    getPlotString(plot, parameters->type, sizeof(plot));

    /* Only display constant value if a Julia set plot */
    if (parameters->type == PLOT_JULIA)
    {
        snprintf(c, sizeof(c), "%.*g + %.*gi",
            DBL_PRINTF_PRECISION, creal(parameters->c), DBL_PRINTF_PRECISION, cimag(parameters->c));
    }
    else
    {
        strncpy(c, "-", sizeof(c));
        c[sizeof(c) - 1] = '\0';
    }

    logMessage(INFO, "Image settings:\n\
    Output     = %s\n\
    Image file = %s\n\
    Dimensions = %zu px * %zu px\n\
    Colour     = {\n\
                     Scheme    = %s,\n\
                     Bit depth = %s,\n\
                 }",
    output,
    (image == NULL) ? "-" : image,
    parameters->width,
    parameters->height,
    colour,
    bitDepthString
    );

    logMessage(INFO, "Plot parameters:\n\
    Plot       = %s\n\
    Minimum    = %.*g + %.*gi\n\
    Maximum    = %.*g + %.*gi\n\
    Constant   = %s\n\
    Iterations = %lu\n",
    plot,
    DBL_PRINTF_PRECISION, creal(parameters->minimum), DBL_PRINTF_PRECISION, cimag(parameters->minimum),
    DBL_PRINTF_PRECISION, creal(parameters->maximum), DBL_PRINTF_PRECISION, cimag(parameters->maximum),
    c,
    parameters->iterations
    );

    return;
}


/* Wrapper for stringToULong() */
int uLongArgument(unsigned long *x, const char *argument, unsigned long min, unsigned long max, char optionID)
{
    char *endptr;
    const int BASE = 10;
    int argError;

    argError = stringToULong(x, argument, min, max, &endptr, BASE);

    if (argError == PARSER_ERANGE || argError == PARSER_EMIN || argError == PARSER_EMAX)
    {
        fprintf(stderr, "%s: -%c: Argument out of range, it must be between %lu and %lu\n", 
            programName, optionID, min, max);
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
            programName, optionID, min, max);
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
            programName, optionID, DBL_PRINTF_PRECISION, min, DBL_PRINTF_PRECISION, max);
        return PARSER_ERANGE;
    }
    else if (argError != PARSER_NONE)
    {
        return PARSER_ERROR;
    }

    return PARSER_NONE;
}


/* Wrapper for stringToComplex() */
int complexArgument(complex *z, char *argument, complex min, complex max, char optionID)
{
    char *endptr;
    int argError;

    argError = stringToComplex(z, argument, min, max, &endptr);

    if (argError == PARSER_ERANGE)
    {
        fprintf(stderr, "%s: -%c: Argument out of range, it must be between %.*g + %.*gi and %.*g + %.*gi\n", 
            programName, optionID, DBL_PRINTF_PRECISION, creal(min), DBL_PRINTF_PRECISION, cimag(min),
            DBL_PRINTF_PRECISION, creal(max), DBL_PRINTF_PRECISION, cimag(max));
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

    /* Check colour scheme */
    if (parameters.output != OUTPUT_TERMINAL && parameters.colour.depth == BIT_DEPTH_ASCII)
    {
        fprintf(stderr, "%s: Invalid colour scheme for output type\n", programName);
        return 1;
    }
    
    /* Check real and imaginary range */
    if (creal(parameters.maximum) <= creal(parameters.minimum))
    {
        fprintf(stderr, "%s: Invalid range - maximum real value is smaller than the minimum\n", programName);
        return 1;
    }
    else if (cimag(parameters.maximum) <= cimag(parameters.minimum))
    {
        fprintf(stderr, "%s: Invalid range - maximum imaginary value is smaller than the minimum\n", programName);
        return 1;
    }

    /* 
     * Minimum addressable data size is CHAR_BIT, therefore the 1-bit image
     * pixels are calculated in groups of CHAR_BIT size
     */
    /* TODO: Make applicable to all depths < CHAR_BIT */
    if (parameters.colour.depth == 1 && parameters.width % CHAR_BIT != 0)
    {
        parameters.width = parameters.width + CHAR_BIT - (parameters.width % CHAR_BIT);
        logMessage(WARNING, "For 1-bit pixel colour schemes, the width must be a multiple of %zu. Width set to %zu",
            CHAR_BIT, parameters.width);
    }

    /* Recommend the iteration count */
    if (parameters.iterations < ITERATIONS_RECOMMENDED_MIN)
    {
        logMessage(WARNING, "Maximum iteration count is low and may result in an imprecise plot, "
            "recommended range is %u to %u", ITERATIONS_RECOMMENDED_MIN, ITERATIONS_RECOMMENDED_MAX);
    }
    else if (parameters.iterations > ITERATIONS_RECOMMENDED_MAX)
    {
        logMessage(WARNING, "Maximum iteration count is high and may needlessly use resources without a "
            "noticeable precision increase, recommended range is %u to %u",
            ITERATIONS_RECOMMENDED_MIN, ITERATIONS_RECOMMENDED_MAX);
    }

    /* Recommend a suitable plot range */
    if (creal(parameters.minimum) < -(ESCAPE_RADIUS) || cimag(parameters.minimum) < -(ESCAPE_RADIUS)
           || creal(parameters.maximum) > ESCAPE_RADIUS || cimag(parameters.maximum) > ESCAPE_RADIUS)
    {
        logMessage(WARNING, "The sample set of complex numbers extends outside (%.*g, %.*g) to (%.*g, %.*g) "
            "- the set in which the Mandelbrot/Julia sets are guaranteed to be contained within",
            DBL_PRINTF_PRECISION, -(ESCAPE_RADIUS), DBL_PRINTF_PRECISION, -(ESCAPE_RADIUS),
            DBL_PRINTF_PRECISION, ESCAPE_RADIUS, DBL_PRINTF_PRECISION, ESCAPE_RADIUS);
    }

    return 0;
}


/* Convert custom getopt error code to message */
int getoptErrorMessage(enum GetoptError optionError, char shortOption, const char *longOption)
{
    switch (optionError)
    {
        case OPT_NONE:
            break;
        case OPT_ERROR:
            fprintf(stderr, "%s: Unknown error when reading command-line options\n", programName);
            break;
        case OPT_EOPT:
            if (shortOption == '\0')
                fprintf(stderr, "%s: Invalid option: \'%s\'\n", programName, longOption);
            else
                fprintf(stderr, "%s: Invalid option: \'-%c\'\n", programName, shortOption);
            break;
        case OPT_ENOARG:
            fprintf(stderr, "%s: -%c: Option argument required\n", programName, shortOption);
            break;
        case OPT_EARG:
            fprintf(stderr, "%s: -%c: Failed to parse argument\n", programName, shortOption);
            break;
        case OPT_EMANY:
            fprintf(stderr, "%s: -%c: Option can only appear once\n", programName, shortOption);
            break;
        case OPT_EARGC_LOW:
            fprintf(stderr, "%s: Too few arguments supplied\n", programName);
            break;
        case OPT_EARGC_HIGH:
            fprintf(stderr, "%s: Too many arguments supplied\n", programName);
            break;
        default:
            break;
    }

    fprintf(stderr, "Try \'%s --help\' for more information\n", programName);
    return EXIT_FAILURE;
}