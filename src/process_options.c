#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <arpa/inet.h>
#include <getopt.h>
#include <netinet/in.h>

#include "libgroot/include/log.h"
#include "percy/include/parser.h"

#include "process_options.h"

#include "arg_ranges.h"
#include "getopt_error.h"
#include "image.h"
#include "network_ctx.h"
#include "parameters.h"
#include "process_args.h"
#include "program_ctx.h"

#ifdef MP_PREC
#include <mpfr.h>
#endif


const uint16_t PORT_DEFAULT = 7939;


#ifdef MP_PREC
static const char *GETOPT_STRING = ":Ac:g:G:i:j:l:m:M:o:p:r:s:tT:vx:Xz:";
#else
static const char *GETOPT_STRING = ":c:g:G:i:j:l:m:M:o:p:r:s:tT:vx:Xz:";
#endif

static const struct option LONG_OPTIONS[] =
{
    #ifdef MP_PREC
    {"multiple", no_argument, NULL, 'A'},         /* Use multiple precision */
    {"precision", required_argument, NULL, 'P'},  /* Specify number of bits to use for the MP significand */
    #endif

    {"colour", required_argument, NULL, 'c'},     /* Colour scheme of PPM image */
    {"worker", required_argument, NULL, 'g'},     /* Initialise as a worker for distributed computation */
    {"master", required_argument, NULL, 'G'},     /* Initialise as a master for distributed computation */
    {"iterations", required_argument, NULL, 'i'}, /* Maximum iteration count of function */
    {"julia", required_argument, NULL, 'j'},      /* Plot a Julia set with specified constant */
    {"log", no_argument, NULL, 'k'},              /* Output log to file */
    {"log-file", required_argument, NULL, 'K'},   /* Specify filepath of log */
    {"log-level", required_argument, NULL, 'l'},  /* Minimum log level to output */
    {"min", required_argument, NULL, 'm'},        /* Range of complex numbers to plot */
    {"max", required_argument, NULL, 'M'},
    {"width", required_argument, NULL, 'r'},      /* Width and height of image */
    {"height", required_argument, NULL, 's'},
    {"threads", required_argument, NULL, 'T'},    /* Specify thread count */
    {"centre", required_argument, NULL, 'x'},     /* Centre coordinate and magnification of plot */
    {"extended", no_argument, NULL, 'X'},         /* Use extended precision */
    {"memory", required_argument, NULL, 'z'},     /* Maximum memory usage in MB */
    {"help", no_argument, NULL, 'h'},             /* Display help message and exit */
    {0, 0, 0, 0}
};


static int parsePrecisionMode(PrecisionMode *precision, int argc, char **argv);
static int parseGlobalOptions(ProgramCTX *ctx, int argc, char **argv);
static NetworkCTX * parseNetworkOptions(int argc, char **argv);
static int parseDiscreteOptions(PlotCTX *p, int argc, char **argv);
static int parseContinuousOptions(PlotCTX *p, int argc, char **argv);
static PlotType parsePlotType(int argc, char **argv);
static OutputType parseOutputType(int argc, char **argv);
static int parseMagnification(PlotCTX *p, int argc, char **argv);


/* Scan argv for invalid command-line options */
int validateOptions(int argc, char **argv)
{
    optind = 0;
    while ((opt = getopt_long(argc, argv, GETOPT_STRING, LONG_OPTIONS, NULL)) != -1)
    {
        if (opt == '?')
        {
            /* If invalid option found */
            opt = optopt;
            getoptErrorMessage(OPT_EOPT, argv[optind - 1]);
            return -1;
        }
        else if (opt == ':')
        {
            /* If missing option argument */
            opt = optopt;
            getoptErrorMessage(OPT_ENOARG, NULL);
            return -1;
        }
    }

    return 0;
}


int processProgramOptions(ProgramCTX *ctx, NetworkCTX **network, int argc, char **argv)
{
    if (!ctx || !network)
        return -1;

    if (initialiseProgramCTX(ctx))
        return -1;

    switch (parseGlobalOptions(ctx, argc, argv))
    {
        case 0:
            break;
        case 1:
            return 1;
        default:
            return -1;
    }

    *network = parseNetworkOptions(argc, argv);

    if (!(*network))
        return -1;

    return 0;
}


PlotCTX * processPlotOptions(int argc, char **argv)
{
    PlotCTX *p;
    PrecisionMode precision;

    PlotType plot = parsePlotType(argc, argv);
    OutputType output = parseOutputType(argc, argv);

    if (output == OUTPUT_NONE)
        return NULL;

    if (parsePrecisionMode(&precision, argc, argv))
        return NULL;

    p = createPlotCTX(precision);

    if (initialisePlotCTX(p, plot, output))
        return NULL;

    if (parseContinuousOptions(p, argc, argv) || parseDiscreteOptions(p, argc, argv))
        return NULL;

    return p;
}


/* Do one getopt pass to set the precision (default is standard precision) */
static int parsePrecisionMode(PrecisionMode *precision, int argc, char **argv)
{
    #ifdef MP_PREC
    unsigned long tempPrecision = 0;
    bool AFlag = false, PFlag = false, XFlag = false;
    #endif

    if (!precision)
        return -1;

    *precision = STD_PRECISION;

    optind = 0;
    while ((opt = getopt_long(argc, argv, GETOPT_STRING, LONG_OPTIONS, NULL)) != -1)
    {
        #ifdef MP_PREC
        ParseErr argError = PARSE_SUCCESS;
        #endif

        switch (opt)
        {
            #ifdef MP_PREC
            case 'A': /* Use multiple precision */
                AFlag = true;
                if (XFlag)
                {
                    fprintf(stderr, "%s: -%c: Option mutually exclusive with -%c\n", programName, opt, 'X');
                    getoptErrorMessage(OPT_NONE, NULL);
                    return -1;
                }

                *precision = MUL_PRECISION;
                break;
            case 'P': /* Specify number of bits to use for the MP significand */
                PFlag = true;
                if (XFlag)
                {
                    fprintf(stderr, "%s: -%c: Option mutually exclusive with -%c\n", programName, opt, 'X');
                    getoptErrorMessage(OPT_NONE, NULL);
                    return -1;
                }

                argError = uLongArg(&tempPrecision, optarg, (unsigned long) MP_BITS_MIN, (unsigned long) MP_BITS_MAX);

                if (argError != PARSE_SUCCESS)
                {
                    break;
                }
                if (tempPrecision < MPFR_PREC_MIN || tempPrecision > MPFR_PREC_MAX)
                {
                    mpfr_fprintf(stderr, "%s: -%c: Argument out of range, it must be between %Pu and %Pu\n",
                                 programName, opt, MPFR_PREC_MIN, MPFR_PREC_MAX);
                    argError = PARSE_ERANGE;
                    break;
                }

                break;
            #endif

            case 'X': /* Use extended precision */

                #ifdef MP_PREC
                XFlag = true;
                if (AFlag || PFlag)
                {
                    fprintf(stderr, "%s: -%c: Option mutually exclusive with -%c\n",
                            programName, opt, (AFlag) ? 'A' : 'P');
                    getoptErrorMessage(OPT_NONE, NULL);
                    return -1;
                }
                #endif

                *precision = EXT_PRECISION;
                break;
            default:
                break;
        }

        #ifdef MP_PREC
        if (argError == PARSE_ERANGE) /* Error message already outputted */
        {
            getoptErrorMessage(OPT_NONE, NULL);
            return -1;
        }
        else if (argError != PARSE_SUCCESS) /* Error but no error message, yet */
        {
            getoptErrorMessage(OPT_EARG, NULL);
            return -1;
        }
        #endif
    }

    #ifdef MP_PREC
    if (PFlag && !AFlag)
    {
        fprintf(stderr, "%s: -%c: Option must be used in conjunction with -%c\n", programName, 'P', 'A');
        getoptErrorMessage(OPT_NONE, NULL);
        return -1;
    }
    else if (PFlag)
    {
        mpSignificandSize = (mpfr_prec_t) tempPrecision;
    }
    #endif

    return 0;
}


/* Parse options common to every mode of operation */
static int parseGlobalOptions(ProgramCTX *ctx, int argc, char **argv)
{
    char tmpLogFilepath[sizeof(ctx->logFilepath)];
    bool KFlag = false, vFlag = false;

    optind = 0;
    while ((opt = getopt_long(argc, argv, GETOPT_STRING, LONG_OPTIONS, NULL)) != -1)
    {
        ParseErr argError = PARSE_SUCCESS;
        unsigned long tempUL = 0;

        switch (opt)
        {
            char *endptr;

            case 'k': /* Output log to file */
                ctx->logToFile = true;
                if (!vFlag)
                    setLogVerbosity(false);
                break;
            case 'K': /* Specify filepath of log */
                KFlag = true;
                ctx->logToFile = true;
                strncpy(tmpLogFilepath, optarg, sizeof(tmpLogFilepath));
                tmpLogFilepath[sizeof(tmpLogFilepath) - 1] = '\0';

                if (!vFlag)
                    setLogVerbosity(false);
                break;
            case 'l': /* Minimum log level to output */
                argError = uLongArg(&tempUL, optarg, LOG_LEVEL_MIN, LOG_LEVEL_MAX);
                setLogLevel((LogLevel) tempUL);
                break;
            case 'T': /* Specify thread count */
                argError = uLongArg(&tempUL, optarg, THREAD_COUNT_MIN, THREAD_COUNT_MAX);
                ctx->threads = (unsigned int) tempUL;
                break;
            case 'v': /* Output log to stderr */
                vFlag = true;
                setLogVerbosity(true);
                break;
            case 'z': /* Maximum memory usage in MB */
                argError = stringToMemory(&ctx->mem, optarg, MEMORY_MIN, MEMORY_MAX, &endptr, MEM_MB);

                if (argError == PARSE_ERANGE || argError == PARSE_EMIN || argError == PARSE_EMAX)
                {
                    fprintf(stderr, "%s: -%c: Argument out of range, it must be between %zu B and %zu B\n",
                            programName, opt, MEMORY_MIN, MEMORY_MAX);
                    argError = PARSE_ERANGE;
                }

                break;
            case 'h': /* Display help message and exit */
                return 1;
            case ':':
                opt = optopt;
                getoptErrorMessage(OPT_ENOARG, NULL);
                return -1;
            default:
                break;
        }

        if (argError == PARSE_ERANGE) /* Error message already outputted */
        {
            getoptErrorMessage(OPT_NONE, NULL);
            return -1;
        }
        else if (argError != PARSE_SUCCESS) /* Error but no error message, yet */
        {
            getoptErrorMessage(OPT_EARG, NULL);
            return -1;
        }
    }

    if (KFlag)
    {
        strncpy(ctx->logFilepath, tmpLogFilepath, sizeof(ctx->logFilepath));
        ctx->logFilepath[sizeof(ctx->logFilepath) - 1] = '\0';
    }

    if (ctx->logToFile && openLog(ctx->logFilepath))
    {
        fprintf(stderr, "%s: -%c: Failed to open log file\n", programName, opt);
        getoptErrorMessage(OPT_NONE, NULL);
        return -1;
    }

    return 0;
}


/* Determine role in distributed network (if any) and allocate network object */
static NetworkCTX * parseNetworkOptions(int argc, char **argv)
{
    int numberOfWorkers = 0;
    char ipAddress[IP_ADDR_STR_LEN_MAX];

    NetworkCTX *network = NULL;
    LANStatus mode = LAN_NONE;

    struct sockaddr_in addr =
    {
        .sin_family = AF_INET,
        .sin_port = htons(PORT_DEFAULT),
    };

    optind = 0;
    while ((opt = getopt_long(argc, argv, GETOPT_STRING, LONG_OPTIONS, NULL)) != -1)
    {
        ParseErr argError = PARSE_SUCCESS;
        unsigned long tempUL = 0;

        switch (opt)
        {
            case 'g': /* Initialise as a worker for distributed computation */
                if (mode != LAN_NONE)
                {
                    fprintf(stderr, "%s: -%c: Option mutually exclusive with -%c\n", programName, opt, 'G');
                    getoptErrorMessage(OPT_NONE, NULL);
                    return NULL;
                }

                if (validateIPAddress(optarg))
                {
                    getoptErrorMessage(OPT_EARG, NULL);
                    return NULL;
                }

                strncpy(ipAddress, optarg, sizeof(ipAddress));
                ipAddress[sizeof(ipAddress) - 1] = '\0';

                if (inet_pton(AF_INET, ipAddress, &addr.sin_addr) != 1)
                {
                    getoptErrorMessage(OPT_ERROR, NULL);
		            return NULL;
                }

                mode = LAN_WORKER;
                break;
            case 'G': /* Initialise as a master for distributed computation */
                if (mode != LAN_NONE)
                {
                    fprintf(stderr, "%s: -%c: Option mutually exclusive with -%c\n", programName, opt, 'g');
                    getoptErrorMessage(OPT_NONE, NULL);
                    return NULL;
                }

                argError = uLongArg(&tempUL, optarg, (unsigned int) WORKERS_MIN, (unsigned int) WORKERS_MAX);                    
                numberOfWorkers = (int) tempUL;

                addr.sin_addr.s_addr = htonl(INADDR_ANY);
                mode = LAN_MASTER;
                break;
            case 'p': /* Port number */
                argError = uLongArg(&tempUL, optarg, PORT_MIN, PORT_MAX);
                addr.sin_port = htons((uint16_t) tempUL);
                break;
            default:
                break;
        }

        if (argError == PARSE_ERANGE) /* Error message already outputted */
        {
            getoptErrorMessage(OPT_NONE, NULL);
            return NULL;
        }
        else if (argError != PARSE_SUCCESS) /* Error but no error message, yet */
        {
            getoptErrorMessage(OPT_EARG, NULL);
            return NULL;
        }
    }

    network = createNetworkCTX(mode, numberOfWorkers, &addr);
    if (!network)
        return NULL;

    return network;
}


/* Get image parameters that are independent of the precision mode */
static int parseDiscreteOptions(PlotCTX *p, int argc, char **argv)
{
    optind = 0;
    while ((opt = getopt_long(argc, argv, GETOPT_STRING, LONG_OPTIONS, NULL)) != -1)
    {
        ParseErr argError = PARSE_SUCCESS;
        unsigned long tempUL = 0;
        uintmax_t tempUIntMax = 0;

        switch (opt)
        {
            case 'c': /* Colour scheme of PPM image */
                /* No enum value is negative or extends beyond ULONG_MAX (defined in colour.h) */
                argError = uLongArg(&tempUL, optarg, 0UL, ULONG_MAX);
                p->colour.scheme = tempUL;

                /* Will return 1 if enum value of out range */
                if (initialiseColourScheme(&p->colour, p->colour.scheme))
                {
                    fprintf(stderr, "%s: -%c: Invalid colour scheme\n", programName, opt);
                    argError = PARSE_ERANGE;
                }
    
                break;
            case 'i': /* Maximum iteration count of function */
                argError = uLongArg(&p->iterations, optarg, ITERATIONS_MIN, ITERATIONS_MAX);
                break;
            case 'o': /* Output image filename */
                strncpy(p->plotFilepath, optarg, sizeof(p->plotFilepath));
                p->plotFilepath[sizeof(p->plotFilepath) - 1] = '\0';
                break;
            case 'r': /* Width of image */
                argError = uIntMaxArg(&tempUIntMax, optarg, WIDTH_MIN, WIDTH_MAX);
                p->width = (size_t) tempUIntMax;
                break;
            case 's': /* Height of image */
                argError = uIntMaxArg(&tempUIntMax, optarg, HEIGHT_MIN, HEIGHT_MAX);
                p->height = (size_t) tempUIntMax;
                break;
            default:
                break;
        }

        if (argError == PARSE_ERANGE) /* Error message already outputted */
        {
            getoptErrorMessage(OPT_NONE, NULL);
            return -1;
        }
        else if (argError != PARSE_SUCCESS) /* Error but no error message, yet */
        {
            getoptErrorMessage(OPT_EARG, NULL);
            return -1;
        }
    }

    return 0;
}


/* Get image parameters that are dependent on the precision mode */
static int parseContinuousOptions(PlotCTX *p, int argc, char **argv)
{
    #ifdef MP_PREC
    initialiseArgRangesMP();
    #endif

    if (parseMagnification(p, argc, argv))
        return -1;

    optind = 0;
    while ((opt = getopt_long(argc, argv, GETOPT_STRING, LONG_OPTIONS, NULL)) != -1)
    {
        ParseErr argError = PARSE_SUCCESS;

        switch (opt)
        {
            case 'j':
                switch (p->precision)
                {
                    case STD_PRECISION:
                        argError = complexArg(&(p->c.c), optarg, C_MIN, C_MAX);
                        break;
                    case EXT_PRECISION:
                        argError = complexArgExt(&(p->c.lc), optarg, C_MIN_EXT, C_MAX_EXT);
                        break;
                    
                    #ifdef MP_PREC
                    case MUL_PRECISION:
                        argError = complexArgMP(p->c.mpc, optarg, C_MIN_MP, C_MAX_MP);
                        break;
                    #endif

                    default:
                        argError = PARSE_EERR;
                        break;
                }

                break;
            case 'm':
                switch (p->precision)
                {
                    case STD_PRECISION:
                        argError = complexArg(&(p->minimum.c), optarg, COMPLEX_MIN, COMPLEX_MAX);
                        break;
                    case EXT_PRECISION:
                        argError = complexArgExt(&(p->minimum.lc), optarg, COMPLEX_MIN_EXT, COMPLEX_MAX_EXT);
                        break;

                    #ifdef MP_PREC
                    case MUL_PRECISION:
                        argError = complexArgMP(p->minimum.mpc, optarg, NULL, NULL);
                        break;
                    #endif

                    default:
                        argError = PARSE_EERR;
                        break;
                }

                break;
            case 'M':
                switch (p->precision)
                {
                    case STD_PRECISION:
                        argError = complexArg(&(p->maximum.c), optarg, COMPLEX_MIN, COMPLEX_MAX);
                        break;
                    case EXT_PRECISION:
                        argError = complexArgExt(&(p->maximum.lc), optarg, COMPLEX_MIN_EXT, COMPLEX_MAX_EXT);
                        break;
                    
                    #ifdef MP_PREC
                    case MUL_PRECISION:
                        argError = complexArgMP(p->maximum.mpc, optarg, NULL, NULL);
                        break;
                    #endif

                    default:
                        argError = PARSE_EERR;
                        break;
                }

                break;
            default:
                break;
        }

        if (argError == PARSE_ERANGE) /* Error message already outputted */
        {
            getoptErrorMessage(OPT_NONE, NULL);

            #ifdef MP_PREC
            freeArgRangesMP();
            #endif

            return -1;
        }
        else if (argError != PARSE_SUCCESS) /* Error but no error message, yet */
        {
            getoptErrorMessage(OPT_EARG, NULL);

            #ifdef MP_PREC
            freeArgRangesMP();
            #endif

            return -1;
        }
    }

    #ifdef MP_PREC
    freeArgRangesMP();
    #endif

    return 0;
}


/* Do one getopt pass to get the plot type */
static PlotType parsePlotType(int argc, char **argv)
{
    PlotType plot = PLOT_MANDELBROT;

    optind = 0;
    while ((opt = getopt_long(argc, argv, GETOPT_STRING, LONG_OPTIONS, NULL)) != -1)
    {
        /* Plot a Julia set with specified constant */
        if (opt == 'j')
            plot = PLOT_JULIA;
    }

    return plot;
}


/* Do one getopt pass to get the plot type (default is Mandelbrot) */
static OutputType parseOutputType(int argc, char **argv)
{
    OutputType output = OUTPUT_PNM;
    bool oFlag = false, tFlag = false;

    optind = 0;
    while ((opt = getopt_long(argc, argv, GETOPT_STRING, LONG_OPTIONS, NULL)) != -1)
    {
        if (opt == 'o') /* Output image filename */
        {
            if (tFlag)
            {
                fprintf(stderr, "%s: -%c: Option mutually exclusive with -%c\n", programName, opt, 't');
                getoptErrorMessage(OPT_NONE, NULL);
                return OUTPUT_NONE;
            }

            oFlag = true;
        }
        else if (opt == 't') /* Output plot to stdout */
        {
            if (oFlag)
            {
                fprintf(stderr, "%s: -%c: Option mutually exclusive with -%c\n", programName, opt, 'o');
                getoptErrorMessage(OPT_NONE, NULL);
                return OUTPUT_NONE;
            }

            tFlag = true;
            output = OUTPUT_TERMINAL;
        }
    }

    return output;
}


/* Do one getopt pass to set the image centre and magnification amount */
static int parseMagnification(PlotCTX *p, int argc, char **argv)
{
    optind = 0;
    while ((opt = getopt_long(argc, argv, GETOPT_STRING, LONG_OPTIONS, NULL)) != -1)
    {
        if (opt == 'x') /* Centre coordinate and magnification of plot */
        {
            ParseErr argError = PARSE_EERR;

            if (p->precision == STD_PRECISION)
            {
                argError = magArg(p, optarg, COMPLEX_MIN, COMPLEX_MAX, MAGNIFICATION_MIN, MAGNIFICATION_MAX);
            }
            else if (p->precision == EXT_PRECISION)
            {
                argError = magArgExt(p, optarg, COMPLEX_MIN_EXT, COMPLEX_MAX_EXT, MAGNIFICATION_MIN, MAGNIFICATION_MAX);
            }

            #ifdef MP_PREC
            else if (p->precision == MUL_PRECISION)
            {
                argError = magArgMP(p, optarg, NULL, NULL, MAGNIFICATION_MIN_EXT, MAGNIFICATION_MAX_EXT);
            }
            #endif

            if (argError == PARSE_ERANGE) /* Error message already outputted */
            {
                getoptErrorMessage(OPT_NONE, NULL);
                return -1;
            }
            else if (argError != PARSE_SUCCESS) /* Error but no error message, yet */
            {
                getoptErrorMessage(OPT_EARG, NULL);
                return -1;
            }
        }
    }

    return 0;
}