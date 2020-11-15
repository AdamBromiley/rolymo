#include <complex.h>
#include <float.h>
#include <limits.h>
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
#include "connection_handler.h"
#include "ext_precision.h"
#include "getopt_error.h"
#include "image.h"
#include "mandelbrot_parameters.h"
#include "parameters.h"
#include "process_options.h"
#include "program_ctx.h"


#define LOG_LEVEL_STR_LEN_MAX 32
#define LOG_TIME_FORMAT_STR_LEN_MAX 32
#define OUTPUT_STR_LEN_MAX 64
#define COLOUR_STR_LEN_MAX 32
#define BIT_DEPTH_STR_LEN_MAX 32
#define PLOT_STR_LEN_MAX 32
#define COMPLEX_STR_LEN_MAX 32
#define PRECISION_STR_LEN_MAX 32


static LogLevel LOG_LEVEL_DEFAULT = INFO;


static void initialiseLog(void);

static int usage(void);
static void plotParameters(const ProgramCTX *ctx, const PlotCTX *p);
static void programParameters(const ProgramCTX *ctx);

static int validateParameters(PlotCTX *p);


/* Process command-line options */
int main(int argc, char **argv)
{
    ProgramCTX *ctx = createProgramCTX();
    PlotCTX *p = createPlotCTX();
    NetworkCTX *network = createNetworkCTX();

    int ret = 0;

    programName = argv[0];

    /* Setup logging library */
    initialiseLog();

    if (!ctx || !p || !network)
    {
        freeProgramCTX(ctx);
        freePlotCTX(p);
        freeNetworkCTX(network);
        getoptErrorMessage(OPT_ERROR, NULL);
        return EXIT_FAILURE;
    }

    /* Process argv with getopt */
    ret = processOptions(ctx, p, network, argc, argv);

    if (ret)
    {
        if (ret == 1)
            usage();

        freeProgramCTX(ctx);
        freePlotCTX(p);
        freeNetworkCTX(network);
        closeLog();
        return (ret == 1) ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    /* Check and warn against some parameters */
    if (validateParameters(p))
    {
        getoptErrorMessage(OPT_NONE, NULL);
        freeProgramCTX(ctx);
        freePlotCTX(p);
        freeNetworkCTX(network);
        closeLog();
        return EXIT_FAILURE;
    }

    /* Output settings */
    programParameters(ctx);

    if (initialiseNetworkConnection(network, p))
    {
        freePlotCTX(p);
        freeProgramCTX(ctx);
        closeLog();
        return EXIT_FAILURE;
    }

    plotParameters(ctx, p);

    /* Open image file and write header (if PNM) */
    if (p->output != OUTPUT_TERMINAL && network->mode != LAN_SLAVE)
    {
        if (initialiseImage(p, ctx->plotFilepath))
        {
            freePlotCTX(p);
            freeProgramCTX(ctx);
            closeLog();
            return EXIT_FAILURE;
        }
    }
    
    /* Produce plot */
    switch (network->mode)
    {
        case LAN_NONE:
            ret = imageOutput(p, ctx->mem, ctx->threads);
            break;
        case LAN_MASTER:
            ret = imageOutputMaster(p, network, ctx->mem);
            break;
        case LAN_SLAVE:
            ret = imageRowOutput(p, network, ctx->threads);
            break;
        default:
            ret = 1;
            break;
    }

    freeProgramCTX(ctx);

    if (ret)
    {
        freePlotCTX(p);
        closeLog();
        return EXIT_FAILURE;
    }

    /* Close file */
    if (p->output != OUTPUT_TERMINAL && network->mode != LAN_SLAVE && closeImage(p))
    {
        freePlotCTX(p);
        closeLog();
        return EXIT_FAILURE;
    }

    freePlotCTX(p);

    /* Close log file */
    if (closeLog())
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}


/* Initialise logging library */
static void initialiseLog(void)
{
    setLogLevel(LOG_LEVEL_DEFAULT);
    setLogVerbosity(true);
    setLogTimeFormat(LOG_TIME_RELATIVE);
    setLogReferenceTime();
}


/* `--help` output */
static int usage(void)
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
    printf("  -o FILE                       Output file name (default = \'%s\')\n", PLOT_FILEPATH_DEFAULT);
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
static void programParameters(const ProgramCTX *ctx)
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
               (ctx->logToFile) ? ctx->logFilepath : "-",
               timeFormat);
}


/* Print plot parameters to log */
static void plotParameters(const ProgramCTX *ctx, const PlotCTX *p)
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
               (p->output == OUTPUT_PNM) ? ctx->plotFilepath : "-",
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
}


/* Check user-supplied parameters */
static int validateParameters(PlotCTX *p)
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