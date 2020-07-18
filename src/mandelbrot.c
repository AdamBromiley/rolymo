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

#ifdef MP_PREC
#include <mpfr.h>
#include <mpc.h>
#endif

#include "libgroot/include/log.h"
#include "percy/include/parser.h"

#include "arg_ranges.h"
#include "array.h"
#include "ext_precision.h"
#include "image.h"
#include "mandelbrot_parameters.h"
#include "parameters.h"


#define FILEPATH_LEN_MAX 4096
#define OUTPUT_FILEPATH_DEFAULT "var/mandelbrot.pnm"
#define LOG_FILEPATH_DEFAULT "var/mandelbrot.log"

#define LOG_LEVEL_STR_LEN_MAX 32
#define LOG_TIME_FORMAT_STR_LEN_MAX 16
#define OUTPUT_STR_LEN_MAX 64
#define COLOUR_STR_LEN_MAX 32
#define BIT_DEPTH_STR_LEN_MAX 32
#define PLOT_STR_LEN_MAX 32
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

static LogLevel LOG_LEVEL_DEFAULT = INFO;

/* Precision of floating points in output */
static const int FLT_PRINTF_PREC = 3;


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

#ifdef MP_PREC
ParseErr complexArgMP(mpc_t z, char *arg, mpc_t min, mpc_t max);
#endif

ParseErr magArg(PlotCTX *p, char *arg, complex cMin, complex cMax, double mMin, double mMax);
ParseErr magArgExt(PlotCTX *p, char *arg, long double complex cMin, long double complex cMax, double mMin, double mMax);

#ifdef MP_PREC
ParseErr magArgMP(PlotCTX *p, char *arg, mpc_t cMin, mpc_t cMax, long double mMin, long double mMax);
#endif

int validateParameters(PlotCTX *p);

int getoptErrorMessage(OptErr error, char shortOpt, const char *longOpt);


/* Process command-line options */
int main(int argc, char **argv)
{
    const struct option LONG_OPTIONS[] =
    {
        #ifdef MP_PREC
        {"multiple", optional_argument, NULL, 'A'},  /* Use multiple precision */
        #endif

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

    #ifdef MP_PREC
    const char *GETOPT_STRING = ":Ac:i:j:l:m:M:o:r:s:tT:vx:Xz:";
    #else
    const char *GETOPT_STRING = ":c:i:j:l:m:M:o:r:s:tT:vx:Xz:";
    #endif

    /* Plotting parameters */
    PlotCTX *parameters;

    ParseErr argError;

    /* Image file path */
    char outputFilepath[FILEPATH_LEN_MAX] = OUTPUT_FILEPATH_DEFAULT;
    bool oFlag = false;

    /* Log parameters */
    char logFilepath[FILEPATH_LEN_MAX] = LOG_FILEPATH_DEFAULT;
    bool lFlag = false, vFlag = false;

    size_t memory = 0;
    unsigned int threadCount = 0;

    programName = argv[0];

    /* Initialise log */
    setLogLevel(LOG_LEVEL_DEFAULT);
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
    #ifdef MP_PREC
    initialiseArgRangesMP();
    #endif

    opterr = 0;

    while ((opt = getopt_long(argc, argv, GETOPT_STRING, LONG_OPTIONS, NULL)) != -1)
    {
        /* Temporary parsing variable for memory safety with uLongArg() */
        unsigned long tempUL;
        char *endptr;

        argError = PARSE_SUCCESS;

        switch (opt)
        {
            #ifdef MP_PREC
            case 'A':
                break;
            #endif

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
                    
                    #ifdef MP_PREC
                    case MUL_PRECISION:
                        argError = complexArgMP(parameters->c.mpc, optarg, C_MIN_MP, C_MAX_MP);
                        break;
                    #endif

                    default:
                        argError = PARSE_EERR;
                        break;
                }

                break;
            case 'k':
                if (!vFlag)
                    setLogVerbosity(false);

                lFlag = true;

                if (optarg)
                {
                    strncpy(logFilepath, optarg, sizeof(logFilepath));
                    logFilepath[sizeof(logFilepath) - 1] = '\0';
                }
               
                break;
            case 'l':
                argError = uLongArg(&tempUL, optarg, LOG_LEVEL_MIN, LOG_LEVEL_MAX);
                setLogLevel((LogLevel) tempUL);
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

                    #ifdef MP_PREC
                    case MUL_PRECISION:
                        argError = complexArgMP(parameters->minimum.mpc, optarg, NULL, NULL);
                        break;
                    #endif

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
                    
                    #ifdef MP_PREC
                    case MUL_PRECISION:
                        argError = complexArgMP(parameters->maximum.mpc, optarg, NULL, NULL);
                        break;
                    #endif

                    default:
                        argError = PARSE_EERR;
                        break;
                }

                break;
            case 'o':
                oFlag = true;
                strncpy(outputFilepath, optarg, sizeof(outputFilepath));
                outputFilepath[sizeof(outputFilepath) - 1] = '\0';
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

                #ifdef MP_PREC
                freeArgRangesMP();
                #endif

                return usage();
            case '?':
                freePlotCTX(parameters);

                #ifdef MP_PREC
                freeArgRangesMP();
                #endif

                return getoptErrorMessage(OPT_EOPT, optopt, argv[optind - 1]);
            case ':':
                freePlotCTX(parameters);

                #ifdef MP_PREC
                freeArgRangesMP();
                #endif

                return getoptErrorMessage(OPT_ENOARG, optopt, NULL);
            default:
                freePlotCTX(parameters);

                #ifdef MP_PREC
                freeArgRangesMP();
                #endif

                return getoptErrorMessage(OPT_ERROR, 0, NULL);
        }

        if (argError != PARSE_SUCCESS)
        {
            freePlotCTX(parameters);

            #ifdef MP_PREC
            freeArgRangesMP();
            #endif

            if (argError == PARSE_ERANGE)
                return getoptErrorMessage(OPT_NONE, 0, NULL);
            else
                return getoptErrorMessage(OPT_EARG, opt, NULL);
        }
    }

    #ifdef MP_PREC
    freeArgRangesMP();
    #endif

    /* Open log file */
    if (lFlag)
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

    /* Open image file and write header (if PNM) */
    if (parameters->output != OUTPUT_TERMINAL || oFlag == true)
    {
        plotParameters(parameters, outputFilepath);

        if (initialiseImage(parameters, outputFilepath))
        {
            freePlotCTX(parameters);
            return EXIT_FAILURE;
        }
    }
    else
    {
        plotParameters(parameters, NULL);
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
    #ifdef MP_PREC
    bool x = false, a = false;
    #endif

    precision = STD_PRECISION;

    while ((opt = getopt_long(argc, argv, optstr, opts, NULL)) != -1)
    {
        #ifdef MP_PREC
        ParseErr argError = PARSE_SUCCESS;
        unsigned long tempUL;
        #endif

        switch (opt)
        {
            case 'X':

                #ifdef MP_PREC
                x = true;
                #endif

                precision = EXT_PRECISION;
                break;
            
            #ifdef MP_PREC
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
            #endif

            default:
                break;
        }

        #ifdef MP_PREC
        if (x && a)
        {
            /* Return PARSE_ERANGE to hide an extra error message */
            fprintf(stderr, "%s: -%c: Option mutually exclusive with previous option\n", programName, opt);
            return PARSE_ERANGE;
        }
        #endif
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

                #ifdef MP_PREC
                case MUL_PRECISION:
                    argError = magArgMP(p, optarg, NULL, NULL, MAGNIFICATION_MIN_EXT, MAGNIFICATION_MAX_EXT);
                    break;
                #endif

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
    char colourScheme[COLOUR_STR_LEN_MAX];
    char logLevel[LOG_LEVEL_STR_LEN_MAX];

    printf("Usage: %s [LOG PARAMETERS...] [OUTPUT PARAMETERS...] [-j CONSTANT] [PLOT PARAMETERS...]\n", programName);
    printf("       %s --help\n\n", programName);
    printf("A Mandelbrot and Julia set plotter.\n\n");
    printf("Mandatory arguments to long options are mandatory for short options too.\n");
    printf("Output parameters:\n");

    getColourString(colourScheme, COLOUR_SCHEME_DEFAULT, sizeof(colourScheme));

    printf("  -c SCHEME, --colour=SCHEME    Specify colour palette to use (default = %s)\n", colourScheme);
    printf("                                  SCHEME may be:\n");

    /* Output all valid colour schemes */
    for (unsigned int i = (unsigned int) COLOUR_SCHEME_MIN; i <= (unsigned int) COLOUR_SCHEME_MAX; ++i)
    {
        if (getColourString(colourScheme, (ColourSchemeType) i, sizeof(colourScheme)))
            continue;

        printf("                                    %-2u = %s\n", i, colourScheme);
    }

    printf("                                  Black and white schemes are 1-bit\n");
    printf("                                  Greyscale schemes are 8-bit\n");
    printf("                                  Coloured schemes are full 24-bit\n\n");
    printf("  -o FILE                       Output file name (default = \'%s\')\n", OUTPUT_FILEPATH_DEFAULT);
    printf("  -r WIDTH,  --width=WIDTH      The width of the image file in pixels\n");
    printf("                                  If using a 1-bit colour scheme, WIDTH must be a multiple of %u to allow "
                                             "for\n"
           "                                  bit-width pixels\n",
                                              (unsigned int) CHAR_BIT);
    printf("  -s HEIGHT, --height=HEIGHT    The height of the image file in pixels\n");
    printf("  -t                            Output to stdout (or, with -o, text file) using ASCII characters as "
                                           "shading\n");
    printf("Plot type:\n");
    printf("  -j CONSTANT                   Plot Julia set with specified constant parameter\n");
    printf("Plot parameters:\n");
    printf("  -m MIN,    --min=MIN          Minimum value to plot\n");
    printf("  -M MAX,    --max=MAX          Maximum value to plot\n");
    printf("  -i NMAX,   --iterations=NMAX  The maximum number of function iterations before a number is deemed to be "
           "within the set\n");
    printf("                                  A larger maximum leads to a preciser plot but increases computation "
                                             "time\n\n");
    printf("  Default parameters (standard-precision):\n");
    printf("    Julia Set:\n");
    printf("      MIN        = %.*g + %.*gi\n",
           FLT_PRINTF_PREC, creal(JULIA_PARAMETERS_DEFAULT.minimum.c),
           FLT_PRINTF_PREC, cimag(JULIA_PARAMETERS_DEFAULT.minimum.c));
    printf("      MAX        = %.*g + %.*gi\n",
           FLT_PRINTF_PREC, creal(JULIA_PARAMETERS_DEFAULT.maximum.c),
           FLT_PRINTF_PREC, cimag(JULIA_PARAMETERS_DEFAULT.maximum.c));
    printf("      ITERATIONS = %lu\n", JULIA_PARAMETERS_DEFAULT.iterations);
    printf("      WIDTH      = %zu\n", JULIA_PARAMETERS_DEFAULT.width);
    printf("      HEIGHT     = %zu\n\n", JULIA_PARAMETERS_DEFAULT.height);
    printf("    Mandelbrot set:\n");
    printf("      MIN        = %.*g + %.*gi\n",
           FLT_PRINTF_PREC, creal(MANDELBROT_PARAMETERS_DEFAULT.minimum.c),
           FLT_PRINTF_PREC, cimag(MANDELBROT_PARAMETERS_DEFAULT.minimum.c));
    printf("      MAX        = %.*g + %.*gi\n",
           FLT_PRINTF_PREC, creal(MANDELBROT_PARAMETERS_DEFAULT.maximum.c),
           FLT_PRINTF_PREC, cimag(MANDELBROT_PARAMETERS_DEFAULT.maximum.c));
    printf("      ITERATIONS = %lu\n", JULIA_PARAMETERS_DEFAULT.iterations);
    printf("      WIDTH      = %zu\n", MANDELBROT_PARAMETERS_DEFAULT.width);
    printf("      HEIGHT     = %zu\n\n", MANDELBROT_PARAMETERS_DEFAULT.height);
    printf("Optimisation:\n");

    #ifdef MP_PREC
    printf("  -A [PREC], --multiple[=PREC] Enable multiple-precision mode\n");
    printf("                                  Specify optional number of precision bits (default = %zu bits)\n",
           (size_t) MP_BITS_DEFAULT);
    printf("                                  MPFR floating-points will be used for calculations\n");
    printf("                                  Precision better than \'-X\', but will be considerably slower\n");
    #endif

    printf("  -T COUNT,  --threads=COUNT    Use COUNT number of processing threads (default = processor count)\n");
    printf("  -X,        --extended         Extend precision (%zu bits, compared to standard-precision %zu bits)\n",
           (size_t) LDBL_MANT_DIG, (size_t) DBL_MANT_DIG);
    printf("                                  The extended floating-point type will be used for calculations\n");
    printf("                                  This will increase precision at high zoom but may be slower\n");
    printf("  -z MEM,    --memory=MEM       Limit memory usage to MEM megabytes (default = 80%% of free RAM)\n");
    printf("Log settings:\n");
    printf("             --log[=FILE]       Output log to file, with optional file path argument\n");
    printf("                                  Optionally, specify path other than default (\'%s\')\n",
           LOG_FILEPATH_DEFAULT);
    printf("                                  Option may be used with \'-v\'\n");

    /* Get default logging level */
    if (getLogLevelString(logLevel, LOG_LEVEL_DEFAULT, sizeof(logLevel)))
    {
        strncpy(logLevel, "Invalid logging level", sizeof(logLevel));
        logLevel[sizeof(logLevel) - 1] = '\0';
    }

    printf("  -l LEVEL,  --log-level=LEVEL  Only log messages more severe than LEVEL (default = %s)\n", logLevel);
    printf("                                  LEVEL may be:\n");

    /* Output all valid logging levels */
    for (unsigned int i = (unsigned int) LOG_LEVEL_MIN; i <= (unsigned int) LOG_LEVEL_MAX; ++i)
    {
        if (getLogLevelString(logLevel, i, sizeof(logLevel)))
            continue;

        printf("                                    %d  = %s%s\n",
               (unsigned int) i, logLevel, i == LOG_NONE ? " (log nothing)" : "");
    }

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

    if (getLogLevelString(level, getLogLevel(), sizeof(level)))
    {
        strncpy(level, "Invalid logging level", sizeof(level));
        level[sizeof(level) - 1]= '\0';
    }

    if (getLogTimeFormatString(timeFormat, getLogTimeFormat(), sizeof(timeFormat)))
    {
        strncpy(timeFormat, "Invalid time format", sizeof(timeFormat));
        timeFormat[sizeof(timeFormat) - 1] = '\0';
    }

    logMessage(DEBUG, "Program settings:\n"
                      "    Verbosity   = %s\n"
                      "    Log level   = %s\n"
                      "    Log file    = %s\n"
                      "    Time format = %s",
               (getLogVerbosity()) ? "VERBOSE" : "QUIET",
               level,
               (log) ? log : "None",
               timeFormat);

    return;
}


/* Print plot parameters to log */
void plotParameters(PlotCTX *p, const char *fp)
{
    char outputStr[OUTPUT_STR_LEN_MAX];
    char colourStr[COLOUR_STR_LEN_MAX];
    char depthStr[BIT_DEPTH_STR_LEN_MAX];
    char typeStr[PLOT_STR_LEN_MAX];
    char minStr[COMPLEX_STR_LEN_MAX];
    char maxStr[COMPLEX_STR_LEN_MAX];
    char cStr[COMPLEX_STR_LEN_MAX];
    char precisionStr[PRECISION_STR_LEN_MAX];

    /* Get output type string from output type and bit depth enums */
    if (getOutputString(outputStr, p, sizeof(outputStr)))
    {
        strncpy(outputStr, "Unknown output mode", sizeof(outputStr));
        outputStr[sizeof(outputStr) - 1] = '\0';
    }

    /* Convert colour scheme enum to a string */
    if (getColourString(colourStr, p->colour.scheme, sizeof(colourStr)))
    {
        strncpy(colourStr, "Unknown colour scheme", sizeof(colourStr));
        colourStr[sizeof(colourStr) - 1] = '\0';
    }

    /* Convert bit depth integer to string */
    if (p->colour.depth > 0)
    {
        snprintf(depthStr, sizeof(depthStr), "(%d-bit)", p->colour.depth);
    }
    else if (p->colour.depth == 0)
    {
        strncpy(depthStr, "", sizeof(depthStr));
        depthStr[sizeof(depthStr) - 1] = '\0';
    }
    else
    {
        strncpy(depthStr, "(Invalid bit depth)", sizeof(depthStr));
        depthStr[sizeof(depthStr) - 1] = '\0';
    }

    /* Get plot type string from PlotType enum */
    if (getPlotString(typeStr, p->type, sizeof(typeStr)))
    {
        strncpy(typeStr, "Unknown plot type", sizeof(typeStr));
        typeStr[sizeof(typeStr) - 1] = '\0';
    }

    /* Construct range strings */
    switch (precision)
    {
        case STD_PRECISION:
            snprintf(minStr, sizeof(minStr), "%.*g + %.*gi",
                     FLT_PRINTF_PREC, creal(p->minimum.c),
                     FLT_PRINTF_PREC, cimag(p->minimum.c));
            snprintf(maxStr, sizeof(maxStr), "%.*g + %.*gi",
                     FLT_PRINTF_PREC, creal(p->maximum.c),
                     FLT_PRINTF_PREC, cimag(p->maximum.c));
            break;
        case EXT_PRECISION:
            snprintf(minStr, sizeof(minStr), "%.*Lg + %.*Lgi",
                     FLT_PRINTF_PREC, creall(p->minimum.lc),
                     FLT_PRINTF_PREC, cimagl(p->minimum.lc));
            snprintf(maxStr, sizeof(maxStr), "%.*Lg + %.*Lgi",
                     FLT_PRINTF_PREC, creall(p->maximum.lc),
                     FLT_PRINTF_PREC, cimagl(p->maximum.lc));
            break;
        
        #ifdef MP_PREC
        case MUL_PRECISION:
            mpfr_snprintf(minStr, sizeof(minStr), "%.*Rg + %.*Rgi",
                          FLT_PRINTF_PREC, mpc_realref(p->minimum.mpc),
                          FLT_PRINTF_PREC, mpc_imagref(p->minimum.mpc));
            mpfr_snprintf(maxStr, sizeof(maxStr), "%.*Rg + %.*Rgi",
                          FLT_PRINTF_PREC, mpc_realref(p->maximum.mpc),
                          FLT_PRINTF_PREC, mpc_imagref(p->maximum.mpc));
            break;
        #endif

        default:
            strncpy(minStr, "Invalid precision mode", sizeof(minStr));
            minStr[sizeof(minStr) - 1] = '\0';
            strncpy(maxStr, "Invalid precision mode", sizeof(maxStr));
            maxStr[sizeof(maxStr) - 1] = '\0';
            return;
    }

    /* Only display constant value if a Julia set plot */
    if (p->type == PLOT_JULIA)
    {
        switch (precision)
        {
            case STD_PRECISION:
                snprintf(cStr, sizeof(cStr), "%.*g + %.*gi",
                         FLT_PRINTF_PREC, creal(p->c.c),
                         FLT_PRINTF_PREC, cimag(p->c.c));
                break;
            case EXT_PRECISION:
                snprintf(cStr, sizeof(cStr), "%.*Lg + %.*Lgi",
                         FLT_PRINTF_PREC, creall(p->c.lc),
                         FLT_PRINTF_PREC, cimagl(p->c.lc));
                break;
            
            #ifdef MP_PREC
            case MUL_PRECISION:
                mpfr_snprintf(cStr, sizeof(cStr), "%.*Rg + %.*Rgi",
                              FLT_PRINTF_PREC, mpc_realref(p->c.mpc),
                              FLT_PRINTF_PREC, mpc_imagref(p->c.mpc));
                break;
            #endif

            default:
                strncpy(cStr, "Invalid precision mode", sizeof(cStr));
                cStr[sizeof(cStr) - 1] = '\0';
        }
    }
    else
    {
        strncpy(cStr, "N/A", sizeof(cStr));
        cStr[sizeof(cStr) - 1] = '\0';
    }

    if (getPrecisionString(precisionStr, precision, sizeof(precisionStr)))
    {
        strncpy(precisionStr, "Invalid precision mode", sizeof(precisionStr));
        precisionStr[sizeof(precisionStr) - 1] = '\0';
    }

    logMessage(INFO, "Image settings:\n"
                     "    Output      = %s\n"
                     "    Output file = %s\n"
                     "    Dimensions  = %zu px * %zu px\n"
                     "    Colour      = %s %s",
               outputStr,
               (!fp) ? "-" : fp,
               p->width,
               p->height,
               colourStr,
               depthStr);

    logMessage(INFO, "Plot parameters:\n"
                     "    Plot        = %s\n"
                     "    Minimum     = %s\n"
                     "    Maximum     = %s\n"
                     "    Constant    = %s\n"
                     "    Iterations  = %lu\n"
                     "    Precision   = %s",
               typeStr,
               minStr,
               maxStr,
               cStr,
               p->iterations,
               precisionStr);

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
            programName, opt, FLT_PRINTF_PREC, min, FLT_PRINTF_PREC, max);
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
            programName, opt, FLT_PRINTF_PREC, min, FLT_PRINTF_PREC, max);
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
            programName, opt, FLT_PRINTF_PREC, creal(min), FLT_PRINTF_PREC, cimag(min),
            FLT_PRINTF_PREC, creal(max), FLT_PRINTF_PREC, cimag(max));
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
            programName, opt, FLT_PRINTF_PREC, creall(min), FLT_PRINTF_PREC, cimagl(min),
            FLT_PRINTF_PREC, creall(max), FLT_PRINTF_PREC, cimagl(max));
        return PARSE_ERANGE;
    }
    else if (argError != PARSE_SUCCESS)
    {
        return PARSE_EERR;
    }

    return PARSE_SUCCESS;
}


#ifdef MP_PREC
/* Wrapper for stringToComplexMPC() */
ParseErr complexArgMP(mpc_t z, char *arg, mpc_t min, mpc_t max)
{
    const int BASE = 0;

    char *endptr;
    ParseErr argError = stringToComplexMPC(z, arg, min, max, &endptr, BASE, mpSignificandSize, MP_COMPLEX_RND);

    if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
    {
        mpfr_fprintf(stderr, "%s: -%c: Argument out of range, it must be between %.*Rg + %.*Rgi and %.*Rg + %.*Rgi",
                     programName, opt,
                     FLT_PRINTF_PREC, mpc_realref(min), FLT_PRINTF_PREC, mpc_imagref(min),
                     FLT_PRINTF_PREC, mpc_realref(max), FLT_PRINTF_PREC, mpc_imagref(max));
        return PARSE_ERANGE;
    }
    else if (argError != PARSE_SUCCESS)
    {
        return PARSE_EERR;
    }

    return PARSE_SUCCESS;
}
#endif


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
                programName, opt, FLT_PRINTF_PREC, mMin, FLT_PRINTF_PREC, mMax);
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
            programName, opt, FLT_PRINTF_PREC, creal(cMin), FLT_PRINTF_PREC, cimag(cMin),
            FLT_PRINTF_PREC, creal(cMax), FLT_PRINTF_PREC, cimag(cMax));
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
                programName, opt, FLT_PRINTF_PREC, mMin, FLT_PRINTF_PREC, mMax);
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
            programName, opt, FLT_PRINTF_PREC, creall(cMin), FLT_PRINTF_PREC, cimagl(cMin),
            FLT_PRINTF_PREC, creall(cMax), FLT_PRINTF_PREC, cimagl(cMax));
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


#ifdef MP_PREC
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
                programName, opt, FLT_PRINTF_PREC, mMin, FLT_PRINTF_PREC, mMax);
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
        mpfr_fprintf(stderr, "%s: -%c: Argument out of range, it must be between %.*Rg + %.*Rgi and %.*Rg + %.*Rgi",
                     programName, opt,
                     FLT_PRINTF_PREC, mpc_realref(cMin), FLT_PRINTF_PREC, mpc_imagref(cMin),
                     FLT_PRINTF_PREC, mpc_realref(cMax), FLT_PRINTF_PREC, mpc_imagref(cMax));
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
#endif


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
     * Minimum addressable data size is CHAR_BIT, therefore bit depths below
     * CHAR_BIT must have pixels calculated in groups of CHAR_BIT size
     */
    if (p->colour.depth < CHAR_BIT && p->width % CHAR_BIT != 0)
    {
        p->width = p->width + CHAR_BIT - (p->width % CHAR_BIT);
        logMessage(WARNING, "For %u-bit pixel colour schemes, the width must be a multiple of %u. Width set to %zu",
                   (unsigned int) p->colour.depth, (unsigned int) CHAR_BIT, p->width);
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