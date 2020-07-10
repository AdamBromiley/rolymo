#include <complex.h>
#include <ctype.h>
#include <float.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <mpfr.h>
#include <mpc.h>

#include "libgroot/include/log.h"
#include "percy/include/parser.h"

#include "arg_ranges.h"
#include "array.h"
#include "ext_precision.h"
#include "image.h"
#include "mandelbrot_parameters.h"
#include "parameters.h"


#define LOG_LEVEL_STR_LEN_MAX 32
#define LOG_TIME_FORMAT_STR_LEN_MAX 16
#define BIT_DEPTH_STR_LEN_MAX 16
#define COMPLEX_STR_LEN_MAX 32
#define PRECISION_STR_LEN_MAX 16


typedef enum GetoptError
{
    OPT_NONE,
    OPT_ERROR,
    OPT_EOPT,
    OPT_ENOARG,
    OPT_EARG,
    OPT_EMANY,
    OPT_EARGC_LOW,
    OPT_EARGC_HIGH
} OptErr;


/* Program name - argv[0] */
static char *programName;

/* Option character for error messages */
static char opt;

/* Output files */
static char *OUTPUT_FILEPATH_DEFAULT = "var/mandelbrot.pnm";

static char *LOG_FILEPATH_DEFAULT = "var/mandelbrot.log";

/* Precision of floating points in output */
static const int DBL_PRINTF_PRECISION = 3;


ParseErr setPrecision(int argc, char **argv, const struct option opts[], const char *optstr);

PlotType getPlotType(int argc, char **argv, const struct option opts[], const char *optstr);
OutputType getOutputType(int argc, char **argv, const struct option opts[], const char *optstr);
ParseErr getMagnification(PlotCTX *p, int argc, char **argv, const struct option opts[], const char *optstr);

int usage(void);
void plotParameters(PlotCTX *p, const char *image);
void programParameters(const char *log);

ParseErr uLongArg(unsigned long *x, char *arg, unsigned long min, unsigned long max);
ParseErr uIntMaxArg(uintmax_t *x, char *arg, uintmax_t min, uintmax_t max);
ParseErr floatArg(double *x, char *arg, double min, double max);
ParseErr floatArgExt(long double *x, char *arg, long double min, long double max);
ParseErr complexArg(complex *z, char *arg, complex min, complex max);
ParseErr complexArgExt(long double complex *z, char *arg, long double complex min, long double complex max);
ParseErr complexArgMP(mpc_t z, char *arg, mpc_t min, mpc_t max);

ParseErr magArg(PlotCTX *p, char *arg, complex cMin, complex cMax, double mMin, double mMax);
ParseErr magArgExt(PlotCTX *p, char *arg, long double complex cMin, long double complex cMax, double mMin, double mMax);
ParseErr magArgMP(PlotCTX *p, char *arg, mpc_t cMin, mpc_t cMax, long double mMin, long double mMax);

int validateParameters(PlotCTX *p);

int getoptErrorMessage(OptErr error, char shortOpt, const char *longOpt);


/* Process command-line options */
int main(int argc, char **argv)
{
    /* Temporary variable for memory safety with uLongArg() */
    unsigned long tempUL;
    char *endptr;

    ParseErr argError;
    const char *GETOPT_STRING = ":Ac:i:j:l:m:M:o:r:s:tT:vx:Xz:";
    const struct option LONG_OPTIONS[] =
    {
        {"arbitrary", optional_argument, NULL, 'A'},  /* Use arbitrary precision */
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
        {"threads", required_argument, NULL, 'T'},    /* Specify thread count */
        {"verbose", no_argument, NULL, 'v'},          /* Output log to stderr */
        {"centre", required_argument, NULL, 'x'},     /* Centre coordinate of plot */
        {"extended", no_argument, NULL, 'X'},         /* Use extended precision */
        {"memory", required_argument, NULL, 'z'},     /* Maximum memory usage in MB */
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };

    /* Plotting parameters */
    PlotCTX *parameters;

    /* Image file path */
    char *outputFilepath = OUTPUT_FILEPATH_DEFAULT;

    /* Log parameters */
    char *logFilepath = NULL;
    bool vFlag = false;

    size_t memory = 0;
    unsigned int threadCount = 0;

    programName = argv[0];

    /* Initialise log */
    setLogVerbosity(true);
    setLogTimeFormat(LOG_TIME_RELATIVE);
    setLogReferenceTime();

    /* Do one getopt pass to set the precision mode */
    argError = setPrecision(argc, argv, LONG_OPTIONS, GETOPT_STRING);

    if (argError == PARSE_ERANGE)
        return getoptErrorMessage(OPT_NONE, 0, NULL);
    else if (argError != PARSE_SUCCESS)
        return getoptErrorMessage(OPT_EARG, opt, NULL);

    /* 
     * Do one getopt pass to get the plot type then create the parameters struct
     * and fill with default values
     */
    parameters = createPlotCTX(getPlotType(argc, argv, LONG_OPTIONS, GETOPT_STRING),
                               getOutputType(argc, argv, LONG_OPTIONS, GETOPT_STRING));

    if (!parameters)
        return getoptErrorMessage(OPT_ERROR, 0, NULL);

    /* Do one getopt pass to get the centrepoint & magnification */
    argError = getMagnification(parameters, argc, argv, LONG_OPTIONS, GETOPT_STRING);

    if (argError == PARSE_ERANGE)
        return getoptErrorMessage(OPT_NONE, 0, NULL);
    else if (argError != PARSE_SUCCESS)
        return getoptErrorMessage(OPT_EARG, opt, NULL);

    /* Parse options */
    opterr = 0;

    while ((opt = getopt_long(argc, argv, GETOPT_STRING, LONG_OPTIONS, NULL)) != -1)
    {
        argError = PARSE_SUCCESS;

        switch (opt)
        {
            case 'A':
                break;
            case 'c':
                /* No enum value is negative or extends beyond ULONG_MAX (defined in colour.h) */
                argError = uLongArg(&tempUL, optarg, 0UL, ULONG_MAX);
                parameters->colour.scheme = tempUL;

                /* Will return 1 if enum value of out range */
                if (initialiseColourScheme(&(parameters->colour), parameters->colour.scheme))
                {
                    argError = PARSE_ERANGE;
                    fprintf(stderr, "%s: -%c: Invalid colour scheme\n", programName, opt);
                }
    
                break;
            case 'i':
                argError = uLongArg(&(parameters->iterations), optarg, ITERATIONS_MIN, ITERATIONS_MAX);
                break;
            case 'j':
                switch (precision)
                {
                    case STD_PRECISION:
                        argError = complexArg(&(parameters->c.c), optarg, C_MIN, C_MAX);
                        break;
                    case EXT_PRECISION:
                        argError = complexArgExt(&(parameters->c.lc), optarg, C_MIN_EXT, C_MAX_EXT);
                        break;
                    case MUL_PRECISION: /* TODO: Set MPC min/max */
                        argError = complexArgMP(parameters->c.mpc, optarg, NULL, NULL);
                        break;
                    default:
                        argError = PARSE_EERR;
                        break;
                }

                break;
            case 'k':
                if (!vFlag)
                    setLogVerbosity(false);

                /* TODO: Change to strncpy() */
                logFilepath = (optarg) ? optarg : LOG_FILEPATH_DEFAULT;
               
                break;
            case 'l':
                argError = uLongArg(&tempUL, optarg, LOG_SEVERITY_MIN, LOG_SEVERITY_MAX);
                setLogLevel((enum LogLevel) tempUL);
                break;
            case 'm':
                switch (precision)
                {
                    case STD_PRECISION:
                        argError = complexArg(&(parameters->minimum.c), optarg, COMPLEX_MIN, COMPLEX_MAX);
                        break;
                    case EXT_PRECISION:
                        argError = complexArgExt(&(parameters->minimum.lc), optarg, COMPLEX_MIN_EXT, COMPLEX_MAX_EXT);
                        break;
                    case MUL_PRECISION: /* TODO: Set MPC min/max */
                        argError = complexArgMP(parameters->minimum.mpc, optarg, NULL, NULL);
                        break;
                    default:
                        argError = PARSE_EERR;
                        break;
                }

                break;
            case 'M':
                switch (precision)
                {
                    case STD_PRECISION:
                        argError = complexArg(&(parameters->maximum.c), optarg, COMPLEX_MIN, COMPLEX_MAX);
                        break;
                    case EXT_PRECISION:
                        argError = complexArgExt(&(parameters->maximum.lc), optarg, COMPLEX_MIN_EXT, COMPLEX_MAX_EXT);
                        break;
                    case MUL_PRECISION: /* TODO: Set MPC min/max */
                        argError = complexArgMP(parameters->maximum.mpc, optarg, NULL, NULL);
                        break;
                    default:
                        argError = PARSE_EERR;
                        break;
                }

                break;
            case 'o':
                /* TODO: Change to strncpy() */
                outputFilepath = optarg;
                break;
            case 'r':
                argError = uIntMaxArg(&(parameters->width), optarg, WIDTH_MIN, WIDTH_MAX);
                break;
            case 's':
                argError = uIntMaxArg(&(parameters->height), optarg, HEIGHT_MIN, HEIGHT_MAX);
                break;
            case 't':
                break;
            case 'T':
                argError = uLongArg(&tempUL, optarg, THREAD_COUNT_MIN, THREAD_COUNT_MAX);
                threadCount = (unsigned int) tempUL;
                break;
            case 'v':
                vFlag = true;
                setLogVerbosity(true);
                break;
            case 'x':
                break;
            case 'X':
                break;
            case 'z':
                argError = stringToMemory(&memory, optarg, MEMORY_MIN, MEMORY_MAX, &endptr, MEM_MB);

                if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
                {
                    fprintf(stderr, "%s: -%c: Argument out of range, it must be between %zu B and %zu B\n",
                            programName, opt, MEMORY_MIN, MEMORY_MAX);
                    argError = PARSE_ERANGE;
                }
                else if (argError != PARSE_SUCCESS)
                {
                    argError = PARSE_EERR;
                }

                break;
            case 'h':
                freePlotCTX(parameters);
                return usage();
            case '?':
                freePlotCTX(parameters);
                return getoptErrorMessage(OPT_EOPT, optopt, argv[optind - 1]);
            case ':':
                freePlotCTX(parameters);
                return getoptErrorMessage(OPT_ENOARG, optopt, NULL);
            default:
                freePlotCTX(parameters);
                return getoptErrorMessage(OPT_ERROR, 0, NULL);
        }

        if (argError != PARSE_SUCCESS)
        {
            freePlotCTX(parameters);

            if (argError == PARSE_ERANGE)
                return getoptErrorMessage(OPT_NONE, 0, NULL);
            else
                return getoptErrorMessage(OPT_EARG, opt, NULL);
        }
    }

    /* Open log file */
    if (logFilepath)
    {
        if (openLog(logFilepath))
        {
            freePlotCTX(parameters);
            return getoptErrorMessage(OPT_NONE, 0, NULL);
        }
    }

    /* Check and warn against some parameters */
    if (validateParameters(parameters))
    {
        freePlotCTX(parameters);
        return getoptErrorMessage(OPT_NONE, 0, NULL);
    }

    /* Output settings */
    programParameters(logFilepath);
    plotParameters(parameters, outputFilepath);

    /* Open image file and write header */
    if (initialiseImage(parameters, outputFilepath))
    {
        freePlotCTX(parameters);
        return EXIT_FAILURE;
    }
    
    /* Produce plot */
    if (imageOutput(parameters, memory, threadCount))
    {
        freePlotCTX(parameters);
        return EXIT_FAILURE;
    }

    /* Close file */
    if (closeImage(parameters))
    {
        freePlotCTX(parameters);
        return EXIT_FAILURE;
    }

    freePlotCTX(parameters);

    /* Close log file */
    if (closeLog())
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}


/* Do one getopt pass to set the precision (default is standard) */
ParseErr setPrecision(int argc, char **argv, const struct option opts[], const char *optstr)
{
    bool x = false, a = false;

    precision = STD_PRECISION;

    while ((opt = getopt_long(argc, argv, optstr, opts, NULL)) != -1)
    {
        ParseErr argError = PARSE_SUCCESS;
        unsigned long tempUL;

        switch (opt)
        {
            case 'X':
                x = true;
                precision = EXT_PRECISION;
                break;
            case 'A':
                a = true;
                precision = MUL_PRECISION;
                mpSignificandSize = MP_BITS_DEFAULT;

                if (optarg)
                {
                    argError = uLongArg(&tempUL, optarg, (unsigned long) MP_BITS_MIN, (unsigned long) MP_BITS_MAX);

                    if (argError != PARSE_SUCCESS)
                        return argError;

                    if (tempUL < MPFR_PREC_MIN || tempUL > MPFR_PREC_MAX)
                        return PARSE_ERANGE;

                    mpSignificandSize = (mpfr_prec_t) tempUL;
                }

                break;
            default:
                break;
        }

        if (x && a)
        {
            /* Return PARSE_ERANGE to hide an extra error message */
            fprintf(stderr, "%s: -%c: Option mutually exclusive with previous option\n", programName, opt);
            return PARSE_ERANGE;
        }
    }

    /* Reset the global optind variable */
    optind = 1;

    return PARSE_SUCCESS;
}


/* Do one getopt pass to get the plot type (default is Mandelbrot) */
PlotType getPlotType(int argc, char **argv, const struct option opts[], const char *optstr)
{
    PlotType plot = PLOT_MANDELBROT;

    while ((opt = getopt_long(argc, argv, optstr, opts, NULL)) != -1)
    {
        if (opt == 'j')
            plot = PLOT_JULIA;
    }

    /* Reset the global optind variable */
    optind = 1;

    return plot;
}


/* Do one getopt pass to get the output type (default is to image) */
OutputType getOutputType(int argc, char **argv, const struct option opts[], const char *optstr)
{
    OutputType output = OUTPUT_PNM;

    while ((opt = getopt_long(argc, argv, optstr, opts, NULL)) != -1)
    {
        if (opt == 't')
            output = OUTPUT_TERMINAL;
    }

    /* Reset the global optind variable */
    optind = 1;

    return output;
}


ParseErr getMagnification(PlotCTX *p, int argc, char **argv, const struct option opts[], const char *optstr)
{
    ParseErr argError = PARSE_SUCCESS;

    while ((opt = getopt_long(argc, argv, optstr, opts, NULL)) != -1)
    {
        if (opt == 'x')
        {
            switch (precision)
            {
                case STD_PRECISION:
                    argError = magArg(p, optarg, COMPLEX_MIN, COMPLEX_MAX, MAGNIFICATION_MIN, MAGNIFICATION_MAX);
                    break;
                case EXT_PRECISION:
                    argError = magArgExt(p, optarg, COMPLEX_MIN_EXT, COMPLEX_MAX_EXT, MAGNIFICATION_MIN, MAGNIFICATION_MAX);
                    break;
                case MUL_PRECISION: /* TODO: Sort out MPC min/max */
                    argError = magArgMP(p, optarg, NULL, NULL,
                                   MAGNIFICATION_MIN_EXT, MAGNIFICATION_MAX_EXT);
                    break;
                default:
                    argError = PARSE_EERR;
                    break;
            }

            if (argError != PARSE_SUCCESS)
                return argError;
        }
    }

    /* Reset the global optind variable */
    optind = 1;

    return PARSE_SUCCESS;
}


/* `--help` output */
int usage(void)
{
    char logLevel[LOG_LEVEL_STR_LEN_MAX];

    getLogLevelString(logLevel, getLogLevel(), sizeof(logLevel));

    printf("Usage: %s [LOG PARAMETERS...] [OUTPUT PARAMETERS...] [-j CONSTANT] [PLOT PARAMETERS...]\n", programName);
    printf("       %s --help\n\n", programName);
    printf("A Mandelbrot and Julia set plotter.\n\n");
    printf("Mandatory arguments to long options are mandatory for short options too.\n");
    printf("Output parameters:\n");
    printf("  -c SCHEME, --colour=SCHEME    Specify colour palette to use\n");
    printf("                                [+] Default = %d\n", COLOUR_SCHEME_DEFAULT);
    printf("                                [+] SCHEME may be:\n");
    printf("                                    1  = Black and white\n");
    printf("                                    2  = White and black\n");
    printf("                                    3  = Greyscale\n");
    printf("                                    4  = Rainbow\n");
    printf("                                    5  = Vibrant rainbow\n");
    printf("                                    6  = Red and white\n");
    printf("                                    7  = Fire\n");
    printf("                                    8  = Red hot\n");
    printf("                                    9  = Matrix\n");
    printf("                                [+] Black and white schemes are 1-bit\n");
    printf("                                [+] Greyscale schemes are 8-bit\n");
    printf("                                [+] Coloured schemes are full 24-bit\n\n");
    printf("  -o FILE                       Output file name\n");
    printf("                                [+] Default = \'%s\'\n", OUTPUT_FILEPATH_DEFAULT);
    printf("  -r WIDTH,  --width=WIDTH      The width of the image file in pixels\n");
    printf("                                [+] If using a 1-bit colour scheme, WIDTH must be a multiple of %u to "
           "allow for\n                                    bit-width pixels\n", (unsigned int) CHAR_BIT);
    printf("  -s HEIGHT, --height=HEIGHT    The height of the image file in pixels\n");
    printf("  -t                            Output to stdout using ASCII characters as shading\n");
    printf("Plot type:\n");
    printf("  -j CONSTANT                   Plot Julia set with specified constant parameter\n");
    printf("Plot parameters:\n");
    printf("  -m MIN,    --min=MIN          Minimum value to plot\n");
    printf("  -M MAX,    --max=MAX          Maximum value to plot\n");
    printf("  -i NMAX,   --iterations=NMAX  The maximum number of function iterations before a number is deemed to be "
           "within the set\n");
    printf("                                [+] A larger maximum leads to a preciser plot but increases computation "
           "time\n\n");
    printf("  Default parameters (standard-precision):\n");
    printf("    Julia Set:\n");
    printf("      MIN        = %.*g + %.*gi\n",
           DBL_PRINTF_PRECISION, creal(JULIA_PARAMETERS_DEFAULT.minimum.c),
           DBL_PRINTF_PRECISION, cimag(JULIA_PARAMETERS_DEFAULT.minimum.c));
    printf("      MAX        = %.*g + %.*gi\n",
           DBL_PRINTF_PRECISION, creal(JULIA_PARAMETERS_DEFAULT.maximum.c),
           DBL_PRINTF_PRECISION, cimag(JULIA_PARAMETERS_DEFAULT.maximum.c));
    printf("      ITERATIONS = %lu\n", JULIA_PARAMETERS_DEFAULT.iterations);
    printf("      WIDTH      = %zu\n", JULIA_PARAMETERS_DEFAULT.width);
    printf("      HEIGHT     = %zu\n\n", JULIA_PARAMETERS_DEFAULT.height);
    printf("    Mandelbrot set:\n");
    printf("      MIN        = %.*g + %.*gi\n",
           DBL_PRINTF_PRECISION, creal(MANDELBROT_PARAMETERS_DEFAULT.minimum.c),
           DBL_PRINTF_PRECISION, cimag(MANDELBROT_PARAMETERS_DEFAULT.minimum.c));
    printf("      MAX        = %.*g + %.*gi\n",
           DBL_PRINTF_PRECISION, creal(MANDELBROT_PARAMETERS_DEFAULT.maximum.c),
           DBL_PRINTF_PRECISION, cimag(MANDELBROT_PARAMETERS_DEFAULT.maximum.c));
    printf("      ITERATIONS = %lu\n", JULIA_PARAMETERS_DEFAULT.iterations);
    printf("      WIDTH      = %zu\n", MANDELBROT_PARAMETERS_DEFAULT.width);
    printf("      HEIGHT     = %zu\n\n", MANDELBROT_PARAMETERS_DEFAULT.height);
    printf("Optimisation:\n");
    printf("  -A [PREC], --arbitrary[=PREC] Enable arbitrary-precision mode, with optional number of bits of precision\n");
    printf("                                [+] Default = %zu bits\n", (size_t) MP_BITS_DEFAULT);
    printf("                                [+] MPFR floating-points will be used for calculations\n");
    printf("                                [+] This increases precision beyond -X but will be considerably slower\n");
    printf("  -T COUNT,  --threads=COUNT    Use COUNT number of processing threads\n");
    printf("                                [+] Default = Online processor count\n");
    printf("  -X,        --extended         Enable extended-precision (%zu bits, compared to the standard-precision "
           "%zu bits)\n", (size_t) LDBL_MANT_DIG, (size_t) DBL_MANT_DIG);
    printf("                                [+] The extended floating-point type will be used for calculations\n");
    printf("                                [+] This will increase precision at high zoom but may be slower\n");
    printf("  -z MEM,    --memory=MEM       Limit memory usage to MEM megabytes\n");
    printf("                                [+] Default = 80%% of free physical memory\n");
    printf("Log settings:\n");
    printf("             --log[=FILE]       Output log to file, with optional file path argument\n");
    printf("                                [+] Default = \'%s\'\n", LOG_FILEPATH_DEFAULT);
    printf("                                [+] Option may be used with \'-v\'\n");
    printf("  -l LEVEL,  --log-level=LEVEL  Only log messages more severe than LEVEL\n");
    printf("                                [+] Default = %s\n", logLevel);
    printf("                                [+] LEVEL may be:\n");
    printf("                                    0  = NONE (log nothing)\n");
    printf("                                    1  = FATAL\n");
    printf("                                    2  = ERROR\n");
    printf("                                    3  = WARNING\n");
    printf("                                    4  = INFO\n");
    printf("                                    5  = DEBUG\n");
    printf("  -v,        --verbose          Redirect log to stderr\n\n");
    printf("Miscellaneous:\n");
    printf("             --help             Display this help message and exit\n\n");
    printf("Examples:\n");
    printf("  %s\n", programName);
    printf("  %s -j \"0.1 - 0.2e-2i\" -o \"juliaset.pnm\"\n", programName);
    printf("  %s -t\n", programName);
    printf("  %s -i 200 --width=5500 --height=5000 --colour=9\n\n", programName);

    return EXIT_SUCCESS;
}

/* Print program parameters to log */
void programParameters(const char *log)
{
    char level[LOG_LEVEL_STR_LEN_MAX];
    char timeFormat[LOG_TIME_FORMAT_STR_LEN_MAX];

    /* Get log parameter strings */
    getLogLevelString(level, getLogLevel(), sizeof(level));
    getLogTimeFormatString(timeFormat, getLogTimeFormat(), sizeof(timeFormat));

    logMessage(DEBUG, "Program settings:\n"
                      "    Verbosity   = %s\n"
                      "    Log level   = %s\n"
                      "    Log file    = %s\n"
                      "    Time format = %s",
               (getLogVerbosity()) ? "VERBOSE" : "QUIET",
               level,
               (log) ? log : "-",
               timeFormat);

    return;
}


/* Print plot parameters to log */
void plotParameters(PlotCTX *p, const char *image)
{
    char output[OUTPUT_STR_LEN_MAX];
    char colour[COLOUR_STRING_LENGTH_MAX];
    char bitDepthString[BIT_DEPTH_STR_LEN_MAX];
    char plot[PLOT_STR_LEN_MAX];
    char minimum[COMPLEX_STR_LEN_MAX];
    char maximum[COMPLEX_STR_LEN_MAX];
    char c[COMPLEX_STR_LEN_MAX];
    char precisionString[PRECISION_STR_LEN_MAX];

    /* Get output type string from output type and bit depth enums */
    getOutputString(output, p, sizeof(output));

    /* Convert colour scheme enum to a string */
    getColourString(colour, p->colour.scheme, sizeof(colour));

    /* Convert bit depth integer to string */
    if (p->colour.depth > 0)
    {
        snprintf(bitDepthString, sizeof(bitDepthString), "%d-bit", p->colour.depth);
    }
    else
    {
        strncpy(bitDepthString, "-", sizeof(bitDepthString));
        bitDepthString[sizeof(bitDepthString) - 1] = '\0';
    }

    /* Get plot type string from PlotType enum */
    getPlotString(plot, p->type, sizeof(plot));

    /* Construct range strings */
    
    switch (precision)
    {
        case STD_PRECISION:
            snprintf(minimum, sizeof(minimum), "%.*g + %.*gi",
                     DBL_PRINTF_PRECISION, creal(p->minimum.c),
                     DBL_PRINTF_PRECISION, cimag(p->minimum.c));
            snprintf(maximum, sizeof(maximum), "%.*g + %.*gi",
                     DBL_PRINTF_PRECISION, creal(p->maximum.c),
                     DBL_PRINTF_PRECISION, cimag(p->maximum.c));
            break;
        case EXT_PRECISION:
            snprintf(minimum, sizeof(minimum), "%.*Lg + %.*Lgi",
                DBL_PRINTF_PRECISION, creall(p->minimum.lc), DBL_PRINTF_PRECISION, cimagl(p->minimum.lc));
            snprintf(maximum, sizeof(maximum), "%.*Lg + %.*Lgi",
                DBL_PRINTF_PRECISION, creall(p->maximum.lc), DBL_PRINTF_PRECISION, cimagl(p->maximum.lc));
            break;
        case MUL_PRECISION:
            /* TODO */
            break;
        default:
            return;
    }

    /* Only display constant value if a Julia set plot */
    if (p->type == PLOT_JULIA)
    {
        switch (precision)
        {
            case STD_PRECISION:
                snprintf(c, sizeof(c), "%.*g + %.*gi",
                    DBL_PRINTF_PRECISION, creal(p->c.c), DBL_PRINTF_PRECISION, cimag(p->c.c));
                break;
            case EXT_PRECISION:
                snprintf(c, sizeof(c), "%.*Lg + %.*Lgi",
                    DBL_PRINTF_PRECISION, creall(p->c.lc), DBL_PRINTF_PRECISION, cimagl(p->c.lc));
                break;
            case MUL_PRECISION:
                /* TODO */
                break;
            default:
                return;
        }
    }
    else
    {
        strncpy(c, "-", sizeof(c));
        c[sizeof(c) - 1] = '\0';
    }

    switch (precision)
    {
        case STD_PRECISION:
            strncpy(precisionString, "STANDARD", sizeof(precisionString));
            break;
        case EXT_PRECISION:
            strncpy(precisionString, "EXTENDED", sizeof(precisionString));
            break;
        case MUL_PRECISION:
            strncpy(precisionString, "ARBITRARY", sizeof(precisionString));
            break;
        default:
            return;
    }

    logMessage(INFO, "Image settings:\n"
                     "    Output     = %s\n"
                     "    Image file = %s\n"
                     "    Dimensions = %zu px * %zu px\n"
                     "    Colour     = %s (%s)",
               output,
               (image == NULL) ? "-" : image,
               p->width,
               p->height,
               colour,
               bitDepthString);

    logMessage(INFO, "Plot parameters:\n"
                     "    Plot       = %s\n"
                     "    Minimum    = %s\n"
                     "    Maximum    = %s\n"
                     "    Constant   = %s\n"
                     "    Iterations = %lu\n"
                     "    Precision  = %s",
               plot,
               minimum,
               maximum,
               c,
               p->iterations,
               precisionString);

    return;
}


/* Wrapper for stringToULong() */
ParseErr uLongArg(unsigned long *x, char *arg, unsigned long min, unsigned long max)
{
    const int BASE = 10;

    char *endptr;
    ParseErr argError = stringToULong(x, arg, min, max, &endptr, BASE);

    if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        fprintf(stderr, "%s: -%c: Argument out of range, it must be between %lu and %lu\n", 
            programName, opt, min, max);
        return PARSE_ERANGE;
    }
    else if (argError != PARSE_SUCCESS)
    {
        return PARSE_EERR;
    }

    return PARSE_SUCCESS;
}


/* Wrapper for stringToUIntMax() */
ParseErr uIntMaxArg(uintmax_t *x, char *arg, uintmax_t min, uintmax_t max)
{
    const int BASE = 10;

    char *endptr;
    ParseErr argError = stringToUIntMax(x, arg, min, max, &endptr, BASE);

    if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        fprintf(stderr, "%s: -%c: Argument out of range, it must be between %" PRIuMAX " and %" PRIuMAX "\n", 
            programName, opt, min, max);
        return PARSE_ERANGE;
    }
    else if (argError != PARSE_SUCCESS)
    {
        return PARSE_EERR;
    }

    return PARSE_SUCCESS;
}


/* Wrapper for stringToDouble() */
ParseErr floatArg(double *x, char *arg, double min, double max)
{
    char *endptr;
    ParseErr argError = stringToDouble(x, arg, min, max, &endptr);

    if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        fprintf(stderr, "%s: -%c: Argument out of range, it must be between %.*g and %.*g\n", 
            programName, opt, DBL_PRINTF_PRECISION, min, DBL_PRINTF_PRECISION, max);
        return PARSE_ERANGE;
    }
    else if (argError != PARSE_SUCCESS)
    {
        return PARSE_EERR;
    }

    return PARSE_SUCCESS;
}


/* Wrapper for stringToDoubleL() */
ParseErr floatArgExt(long double *x, char *arg, long double min, long double max)
{
    char *endptr;
    ParseErr argError = stringToDoubleL(x, arg, min, max, &endptr);

    if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        fprintf(stderr, "%s: -%c: Argument out of range, it must be between %.*Lg and %.*Lg\n", 
            programName, opt, DBL_PRINTF_PRECISION, min, DBL_PRINTF_PRECISION, max);
        return PARSE_ERANGE;
    }
    else if (argError != PARSE_SUCCESS)
    {
        return PARSE_EERR;
    }

    return PARSE_SUCCESS;
}


/* Wrapper for stringToComplex() */
ParseErr complexArg(complex *z, char *arg, complex min, complex max)
{
    char *endptr;
    ParseErr argError = stringToComplex(z, arg, min, max, &endptr);

    if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        fprintf(stderr, "%s: -%c: Argument out of range, it must be between %.*g + %.*gi and %.*g + %.*gi\n", 
            programName, opt, DBL_PRINTF_PRECISION, creal(min), DBL_PRINTF_PRECISION, cimag(min),
            DBL_PRINTF_PRECISION, creal(max), DBL_PRINTF_PRECISION, cimag(max));
        return PARSE_ERANGE;
    }
    else if (argError != PARSE_SUCCESS)
    {
        return PARSE_EERR;
    }

    return PARSE_SUCCESS;
}


/* Wrapper for stringToComplexL() */
ParseErr complexArgExt(long double complex *z, char *arg, long double complex min, long double complex max)
{
    char *endptr;
    ParseErr argError = stringToComplexL(z, arg, min, max, &endptr);

    if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        fprintf(stderr, "%s: -%c: Argument out of range, it must be between %.*Lg + %.*Lgi and %.*Lg + %.*Lgi\n", 
            programName, opt, DBL_PRINTF_PRECISION, creall(min), DBL_PRINTF_PRECISION, cimagl(min),
            DBL_PRINTF_PRECISION, creall(max), DBL_PRINTF_PRECISION, cimagl(max));
        return PARSE_ERANGE;
    }
    else if (argError != PARSE_SUCCESS)
    {
        return PARSE_EERR;
    }

    return PARSE_SUCCESS;
}


/* Wrapper for stringToComplexMPC() */
ParseErr complexArgMP(mpc_t z, char *arg, mpc_t min, mpc_t max)
{
    const int BASE = 0;

    char *endptr;
    ParseErr argError = stringToComplexMPC(z, arg, min, max, &endptr, BASE, mpSignificandSize, MP_COMPLEX_RND);

    if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        fprintf(stderr, "%s: -%c: Argument out of range", programName, opt);
        /*
        fprintf(stderr, "%s: -%c: Argument out of range, it must be between %.*Lg + %.*Lgi and %.*Lg + %.*Lgi\n", 
            programName, opt, DBL_PRINTF_PRECISION, creall(min), DBL_PRINTF_PRECISION, cimagl(min),
            DBL_PRINTF_PRECISION, creall(max), DBL_PRINTF_PRECISION, cimagl(max));*/
        return PARSE_ERANGE;
    }
    else if (argError != PARSE_SUCCESS)
    {
        return PARSE_EERR;
    }

    return PARSE_SUCCESS;
}


ParseErr magArg(PlotCTX *p, char *arg, complex cMin, complex cMax, double mMin, double mMax)
{
    complex rangeDefault, imageCentre;
    double magnification;

    char *endptr;
    ParseErr argError = stringToComplex(&imageCentre, arg, cMin, cMax, &endptr);

    if (argError == PARSE_SUCCESS)
    {
        /* Magnification not explicitly mentioned - default to 1.0 */
        magnification = 1.0;
    }
    else if (argError == PARSE_EEND)
    {
        /* Check for comma separator */
        while (isspace(*endptr))
            ++endptr;

        if (*endptr != ',')
            return PARSE_EFORM;

        ++endptr;

        /* Get magnification argument */
        argError = floatArg(&magnification, endptr, mMin, mMax);

        if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
        {
            fprintf(stderr, "%s: -%c: Magnification out of range, it must be between %.*g and %.*g\n", 
                programName, opt, DBL_PRINTF_PRECISION, mMin, DBL_PRINTF_PRECISION, mMax);
            return PARSE_ERANGE;
        }
        else if (argError != PARSE_SUCCESS)
        {
            return PARSE_EERR;
        }
    }
    else if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        fprintf(stderr, "%s: -%c: Argument out of range, it must be between %.*g + %.*gi and %.*g + %.*gi\n", 
            programName, opt, DBL_PRINTF_PRECISION, creal(cMin), DBL_PRINTF_PRECISION, cimag(cMin),
            DBL_PRINTF_PRECISION, creal(cMax), DBL_PRINTF_PRECISION, cimag(cMax));
        return PARSE_ERANGE;
    }
    else
    {
        return PARSE_EFORM;
    }

    /* Convert centrepoint and magnification to range */
    switch (p->type)
    {
        case PLOT_MANDELBROT:
            rangeDefault = MANDELBROT_PARAMETERS_DEFAULT.maximum.c - MANDELBROT_PARAMETERS_DEFAULT.minimum.c;
            break;
        case PLOT_JULIA:
            rangeDefault = JULIA_PARAMETERS_DEFAULT.maximum.c - JULIA_PARAMETERS_DEFAULT.minimum.c;
            break;
        default:
            return PARSE_EERR;
    }

    p->minimum.c = imageCentre - 0.5 * rangeDefault * pow(0.9, magnification - 1.0);
    p->maximum.c = imageCentre + 0.5 * rangeDefault * pow(0.9, magnification - 1.0);

    return PARSE_SUCCESS;
}


ParseErr magArgExt(PlotCTX *p, char *arg, long double complex cMin,
                                      long double complex cMax, double mMin, double mMax)
{
    long double complex rangeDefault, imageCentre;
    double magnification;

    char *endptr;
    ParseErr argError = stringToComplexL(&imageCentre, arg, cMin, cMax, &endptr);

    if (argError == PARSE_SUCCESS)
    {
        /* Magnification not explicitly mentioned - default to 1 */
        magnification = 1.0;
    }
    else if (argError == PARSE_EEND)
    {
        /* Check for comma separator */
        while (isspace(*endptr))
            ++endptr;

        if (*endptr != ',')
            return PARSE_EFORM;

        ++endptr;

        /* Get magnification argument */
        argError = floatArg(&magnification, endptr, mMin, mMax);

        if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
        {
            fprintf(stderr, "%s: -%c: Magnification out of range, it must be between %.*g and %.*g\n", 
                programName, opt, DBL_PRINTF_PRECISION, mMin, DBL_PRINTF_PRECISION, mMax);
            return PARSE_ERANGE;
        }
        else if (argError != PARSE_SUCCESS)
        {
            return PARSE_EERR;
        }
    }
    else if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        fprintf(stderr, "%s: -%c: Argument out of range, it must be between %.*Lg + %.*Lgi and %.*Lg + %.*Lgi\n", 
            programName, opt, DBL_PRINTF_PRECISION, creall(cMin), DBL_PRINTF_PRECISION, cimagl(cMin),
            DBL_PRINTF_PRECISION, creall(cMax), DBL_PRINTF_PRECISION, cimagl(cMax));
        return PARSE_ERANGE;
    }
    else
    {
        return PARSE_EFORM;
    }

    /* Convert centrepoint and magnification to range */
    switch (p->type)
    {
        case PLOT_MANDELBROT:
            rangeDefault = MANDELBROT_PARAMETERS_DEFAULT_EXT.maximum.lc - MANDELBROT_PARAMETERS_DEFAULT_EXT.minimum.lc;
            break;
        case PLOT_JULIA:
            rangeDefault = JULIA_PARAMETERS_DEFAULT_EXT.maximum.lc - JULIA_PARAMETERS_DEFAULT_EXT.minimum.lc;
            break;
        default:
            return PARSE_EERR;
    }

    p->minimum.lc = imageCentre - 0.5L * rangeDefault * powl(0.9L, magnification - 1.0L);
    p->maximum.lc = imageCentre + 0.5L * rangeDefault * powl(0.9L, magnification - 1.0L);

    return PARSE_SUCCESS;
}


ParseErr magArgMP(PlotCTX *p, char *arg, mpc_t cMin, mpc_t cMax, long double mMin, long double mMax)
{
    const int BASE = 0;

    mpc_t rangeDefault, imageCentre;
    long double magnification;

    mpfr_t tmp;

    char *endptr;
    ParseErr argError;
    
    mpc_init2(imageCentre, mpSignificandSize);
    argError = stringToComplexMPC(imageCentre, arg, cMin, cMax, &endptr, BASE, mpSignificandSize, MP_COMPLEX_RND);

    if (argError == PARSE_SUCCESS)
    {
        /* Magnification not explicitly mentioned - default to 1 */
        magnification = 1.0L;
    }
    else if (argError == PARSE_EEND)
    {
        /* Check for comma separator */
        while (isspace(*endptr))
            ++endptr;

        if (*endptr != ',')
        {
            mpc_clear(imageCentre);
            return PARSE_EFORM;
        }

        ++endptr;

        /* Get magnification argument */
        argError = floatArgExt(&magnification, endptr, mMin, mMax);

        if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
        {
            mpc_clear(imageCentre);
            fprintf(stderr, "%s: -%c: Magnification out of range, it must be between %.*Lg and %.*Lg\n", 
                programName, opt, DBL_PRINTF_PRECISION, mMin, DBL_PRINTF_PRECISION, mMax);
            return PARSE_ERANGE;
        }
        else if (argError != PARSE_SUCCESS)
        {
            mpc_clear(imageCentre);
            return PARSE_EERR;
        }
    }
    else if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        mpc_clear(imageCentre);
        fprintf(stderr, "%s: -%c: Argument out of range", programName, opt);
        /*fprintf(stderr, "%s: -%c: Argument out of range, it must be between %.*Lg + %.*Lgi and %.*Lg + %.*Lgi\n", 
            programName, opt, DBL_PRINTF_PRECISION, creall(cMin), DBL_PRINTF_PRECISION, cimagl(cMin),
            DBL_PRINTF_PRECISION, creall(cMax), DBL_PRINTF_PRECISION, cimagl(cMax));*/
        return PARSE_ERANGE;
    }
    else
    {
        mpc_clear(imageCentre);
        return PARSE_EFORM;
    }

    mpc_init2(rangeDefault, mpSignificandSize);

    /* Convert centrepoint and magnification to range */
    switch (p->type)
    {
        case PLOT_MANDELBROT:
            mpc_set_ldc(rangeDefault,
                MANDELBROT_PARAMETERS_DEFAULT_EXT.maximum.lc - MANDELBROT_PARAMETERS_DEFAULT_EXT.minimum.lc,
                MP_COMPLEX_RND);
            /*mpc_sub(rangeDefault, MANDELBROT_PARAMETERS_DEFAULT_EXT.maximum.mpc,
                MANDELBROT_PARAMETERS_DEFAULT_MP.minimum.mpc, MP_COMPLEX_RND);*/
            break;
        case PLOT_JULIA:
            mpc_sub(rangeDefault, JULIA_PARAMETERS_DEFAULT_MP.maximum.mpc,
                JULIA_PARAMETERS_DEFAULT_MP.minimum.mpc, MP_COMPLEX_RND);
            break;
        default:
            mpc_clear(imageCentre);
            mpc_clear(rangeDefault);
            return PARSE_EERR;
    }

    mpfr_init2(tmp, mpSignificandSize);
    mpfr_set_ld(tmp, 0.5L * powl(0.9L, magnification - 1.0L), MP_REAL_RND);

    mpc_mul_fr(rangeDefault, rangeDefault, tmp, MP_COMPLEX_RND);

    mpfr_clear(tmp);

    mpc_sub(p->minimum.mpc, imageCentre, rangeDefault, MP_COMPLEX_RND);
    mpc_add(p->maximum.mpc, imageCentre, rangeDefault, MP_COMPLEX_RND);

    mpc_clear(imageCentre);
    mpc_clear(rangeDefault);

    return PARSE_SUCCESS;
}


/* Check user-supplied parameters */
int validateParameters(PlotCTX *p)
{
    /* Check colour scheme */
    if (p->output != OUTPUT_TERMINAL && p->colour.depth == BIT_DEPTH_ASCII)
    {
        fprintf(stderr, "%s: Invalid colour scheme for output type\n", programName);
        return 1;
    }
    
    /* Check real and imaginary range */
    if (!precision)
    {
        if (creal(p->maximum.c) < creal(p->minimum.c))
        {
            fprintf(stderr, "%s: Invalid range - maximum real value is smaller than the minimum\n", programName);
            return 1;
        }
        else if (cimag(p->maximum.c) < cimag(p->minimum.c))
        {
            fprintf(stderr, "%s: Invalid range - maximum imaginary value is smaller than the minimum\n", programName);
            return 1;
        }
    }
    else
    {
        if (creall(p->maximum.lc) < creall(p->minimum.lc))
        {
            fprintf(stderr, "%s: Invalid range - maximum real value is smaller than the minimum\n", programName);
            return 1;
        }
        else if (cimagl(p->maximum.lc) < cimagl(p->minimum.lc))
        {
            fprintf(stderr, "%s: Invalid range - maximum imaginary value is smaller than the minimum\n", programName);
            return 1;
        }
    }

    /* 
     * Minimum addressable data size is CHAR_BIT, therefore the 1-bit image
     * pixels are calculated in groups of CHAR_BIT size
     */
    /* TODO: Make applicable to all depths < CHAR_BIT */
    if (p->colour.depth == 1 && p->width % CHAR_BIT != 0)
    {
        p->width = p->width + CHAR_BIT - (p->width % CHAR_BIT);
        logMessage(WARNING, "For 1-bit pixel colour schemes, the width must be a multiple of %zu. Width set to %zu",
            CHAR_BIT, p->width);
    }

    return 0;
}


/* Convert custom getopt error code to message */
int getoptErrorMessage(OptErr error, char shortOpt, const char *longOpt)
{
    switch (error)
    {
        case OPT_NONE:
            break;
        case OPT_ERROR:
            fprintf(stderr, "%s: Unknown error when reading command-line options\n", programName);
            break;
        case OPT_EOPT:
            if (shortOpt == '\0' && longOpt != NULL)
                fprintf(stderr, "%s: Invalid option: \'%s\'\n", programName, longOpt);
            else if (shortOpt != '\0')
                fprintf(stderr, "%s: Invalid option: \'-%c\'\n", programName, shortOpt);
            break;
        case OPT_ENOARG:
            fprintf(stderr, "%s: -%c: Option argument required\n", programName, shortOpt);
            break;
        case OPT_EARG:
            fprintf(stderr, "%s: -%c: Failed to parse argument\n", programName, shortOpt);
            break;
        case OPT_EMANY:
            fprintf(stderr, "%s: -%c: Option can only appear once\n", programName, shortOpt);
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