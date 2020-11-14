#include "process_options.h"

#include "arg_ranges.h"
#include "connection_handler.h"
#include "getopt_error.h"
#include "image.h"
#include "parameters.h"
#include "process_args.h"
#include "program_ctx.h"

#include "libgroot/include/log.h"
#include "percy/include/parser.h"

#include <arpa/inet.h>
#include <getopt.h>

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#ifdef MP_PREC
#include <mpfr.h>
#endif


static int validateOptions(int argc, char **argv);

static int parseGlobalOptions(ProgramCTX *ctx, int argc, char **argv);
static int parseNetworkOptions(NetworkCTX *network, int argc, char **argv);
static int parseDiscreteOptions(ProgramCTX *ctx, PlotCTX *p, int argc, char **argv);
static int parseContinuousOptions(PlotCTX *p, int argc, char **argv);
static int parsePrecisionMode(int argc, char **argv);
static PlotType parsePlotType(int argc, char **argv);
static OutputType parseOutputType(int argc, char **argv);
static int parseMagnification(PlotCTX *p, int argc, char **argv);


int processOptions(ProgramCTX *ctx, PlotCTX *p, NetworkCTX *network, int argc, char **argv)
{
    PlotType plot;
    OutputType output;

    if (initialiseProgramCTX(ctx))
        return -1;

    if (validateOptions(argc, argv))
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

    if (parseNetworkOptions(network, argc, argv))
        return -1;

    if (parsePrecisionMode(argc, argv))
        return -1;

    plot = parsePlotType(argc, argv);
    output = parseOutputType(argc, argv);

    if (output == OUTPUT_NONE)
        return -1;

    if (initialisePlotCTX(p, plot, output))
        return -1;

    if (network->mode == LAN_NONE || network->mode == LAN_MASTER)
    {
        if (parseContinuousOptions(p, argc, argv))
            return -1;

        if (parseDiscreteOptions(ctx, p, argc, argv))
            return -1;
    }

    return 0;
}


/* Scan argv for invalid command-line options */
static int validateOptions(int argc, char **argv)
{
    #ifdef MP_PREC
    const char *GETOPT_STRING = ":A::c:g:Gi:j:l:m:M:o:r:s:tT:vx:Xz:"; /* TODO: Add double colon after A and k? */
    #else
    const char *GETOPT_STRING = ":c:g:Gi:j:l:m:M:o:r:s:tT:vx:Xz:"; /* TODO: Add double colon after A and k? */
    #endif

    const struct option LONG_OPTIONS[] =
    {
        #ifdef MP_PREC
        {"multiple", optional_argument, NULL, 'A'},  /* Use multiple precision */
        #endif

        {"colour", required_argument, NULL, 'c'},     /* Colour scheme of PPM image */
        {"slave", required_argument, NULL, 'g'},      /* Initialise as a slave for distributed computation */
        {"master", no_argument, NULL, 'G'},           /* Initialise as a master for distributed computation */
        {"iterations", required_argument, NULL, 'i'}, /* Maximum iteration count of function */
        {"julia", required_argument, NULL, 'j'},      /* Plot a Julia set with specified constant */
        {"log", optional_argument, NULL, 'k'},        /* Output log to file (with optional path) */
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

    optind = 1;
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


/* Parse options common to every mode of operation */
static int parseGlobalOptions(ProgramCTX *ctx, int argc, char **argv)
{
    const char *GETOPT_STRING = ":l:T:vz:";

    const struct option LONG_OPTIONS[] =
    {
        {"log", optional_argument, NULL, 'k'},       /* Output log to file (with optional path) */
        {"log-level", required_argument, NULL, 'l'}, /* Minimum log level to output */
        {"threads", required_argument, NULL, 'T'},   /* Specify thread count */
        {"memory", required_argument, NULL, 'z'},    /* Maximum memory usage in MB */
        {"help", no_argument, NULL, 'h'},            /* Display help message and exit */
        {0, 0, 0, 0}
    };

    bool vFlag = false;

    optind = 1;
    while ((opt = getopt_long(argc, argv, GETOPT_STRING, LONG_OPTIONS, NULL)) != -1)
    {
        char *endptr;
        unsigned long tempUL;
        ParseErr argError = PARSE_SUCCESS;

        switch (opt)
        {
            case 'k': /* Output log to file (with optional path) */
                if (!vFlag)
                    setLogVerbosity(false);

                if (optarg)
                {
                    strncpy(ctx->logFilepath, optarg, sizeof(ctx->logFilepath));
                    ctx->logFilepath[sizeof(ctx->logFilepath) - 1] = '\0';
                }

                if (openLog(ctx->logFilepath))
                {
                    fprintf(stderr, "%s: -%c: Failed to open log file\n", programName, opt);
                    argError = PARSE_ERANGE;
                }

                ctx->logToFile = true;

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

    return 0;
}


/* Determine role in distributed network (if any) */
static int parseNetworkOptions(NetworkCTX *network, int argc, char **argv)
{
    const char *GETOPT_STRING = ":g:Gp:";

    const struct option LONG_OPTIONS[] =
    {
        {"slave", required_argument, NULL, 'g'}, /* Initialise as a slave for distributed computation */
        {"master", no_argument, NULL, 'G'},      /* Initialise as a master for distributed computation */
        {0, 0, 0, 0}
    };

    const uint16_t PORT_DEFAULT = 7939;

    char ipAddress[IP_ADDR_STR_LEN_MAX];
    uint16_t port = PORT_DEFAULT;

    network->mode = LAN_NONE;

    optind = 1;
    while ((opt = getopt_long(argc, argv, GETOPT_STRING, LONG_OPTIONS, NULL)) != -1)
    {
        switch (opt)
        {
            unsigned long tempUL;
            ParseErr argError;

            case 'g': /* Initialise as a slave for distributed computation */
                if (network->mode != LAN_NONE)
                {
                    fprintf(stderr, "%s: -%c: Option mutually exclusive with -%c\n", programName, opt, 'G');
                    getoptErrorMessage(OPT_NONE, NULL);
                    return -1;
                }

                if (validateIPAddress(optarg))
                {
                    getoptErrorMessage(OPT_EARG, NULL);
                    return -1;
                }

                strncpy(ipAddress, optarg, sizeof(ipAddress));
                ipAddress[sizeof(ipAddress) - 1] = '\0';

                if (inet_pton(AF_INET, ipAddress, &network->addr.sin_addr) != 1)
                {
                    getoptErrorMessage(OPT_ERROR, NULL);
		            return -1;
                }

                network->mode = LAN_SLAVE;
                break;
            case 'G': /* Initialise as a master for distributed computation */
                if (network->mode != LAN_NONE)
                {
                    fprintf(stderr, "%s: -%c: Option mutually exclusive with -%c\n", programName, opt, 'g');
                    getoptErrorMessage(OPT_NONE, NULL);
                    return -1;
                }

                network->addr.sin_addr.s_addr = htonl(INADDR_ANY);
                network->mode = LAN_MASTER;
                break;
            case 'p': /* Port number */
                argError = uLongArg(&tempUL, optarg, PORT_MIN, PORT_MAX);
                port = (uint16_t) tempUL;
    
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

                break;
            default:
                break;
        }
    }

    network->addr.sin_family = AF_INET;
    network->addr.sin_port = htons(port);

    return 0;
}


/* Get image parameters that are independent of the precision mode */
static int parseDiscreteOptions(ProgramCTX *ctx, PlotCTX *p, int argc, char **argv)
{
    const char *GETOPT_STRING = ":c:i:o:r:s:";

    const struct option LONG_OPTIONS[] =
    {
        {"colour", required_argument, NULL, 'c'},     /* Colour scheme of PPM image */
        {"iterations", required_argument, NULL, 'i'}, /* Maximum iteration count of function */
        {"width", required_argument, NULL, 'r'},      /* Width and height of image */
        {"height", required_argument, NULL, 's'},
        {0, 0, 0, 0}
    };

    optind = 1;
    while ((opt = getopt_long(argc, argv, GETOPT_STRING, LONG_OPTIONS, NULL)) != -1)
    {
        /* Temporary parsing variable for memory safety with uLongArg() */
        unsigned long tempUL;
        ParseErr argError = PARSE_SUCCESS;

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
                strncpy(ctx->plotFilepath, optarg, sizeof(ctx->plotFilepath));
                ctx->plotFilepath[sizeof(ctx->plotFilepath) - 1] = '\0';
                break;
            case 'r': /* Width of image */
                argError = uIntMaxArg(&p->width, optarg, WIDTH_MIN, WIDTH_MAX);
                break;
            case 's': /* Height of image */
                argError = uIntMaxArg(&p->height, optarg, HEIGHT_MIN, HEIGHT_MAX);
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
    const char *GETOPT_STRING = ":j:m:M:";

    const struct option LONG_OPTIONS[] =
    {
        {"julia", required_argument, NULL, 'j'}, /* Plot a Julia set and specify constant */
        {"min", required_argument, NULL, 'm'},   /* Range of complex numbers to plot */
        {"max", required_argument, NULL, 'M'},
        {0, 0, 0, 0}
    };

    #ifdef MP_PREC
    initialiseArgRangesMP();
    #endif

    if (parseMagnification(p, argc, argv))
        return -1;

    optind = 1;
    while ((opt = getopt_long(argc, argv, GETOPT_STRING, LONG_OPTIONS, NULL)) != -1)
    {
        ParseErr argError = PARSE_SUCCESS;

        switch (opt)
        {
            case 'j':
                switch (precision)
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
                switch (precision)
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
                switch (precision)
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


/* Do one getopt pass to set the precision (default is standard precision) */
static int parsePrecisionMode(int argc, char **argv)
{
    #ifdef MP_PREC
    const char *GETOPT_STRING = ":A::X";
    #else
    const char *GETOPT_STRING = ":X";
    #endif

    const struct option LONG_OPTIONS[] =
    {
        #ifdef MP_PREC
        {"multiple", optional_argument, NULL, 'A'}, /* Use multiple precision */
        #endif
        
        {"extended", no_argument, NULL, 'X'},       /* Use extended precision */
        {0, 0, 0, 0}
    };

    #ifdef MP_PREC
    bool xFlag = false, aFlag = false;
    #endif

    precision = STD_PRECISION;

    optind = 1;
    while ((opt = getopt_long(argc, argv, GETOPT_STRING, LONG_OPTIONS, NULL)) != -1)
    {
        #ifdef MP_PREC
        ParseErr argError = PARSE_SUCCESS;
        unsigned long tempUL;
        #endif

        switch (opt)
        {
            case 'X':

                #ifdef MP_PREC
                if (aFlag)
                {
                    fprintf(stderr, "%s: -%c: Option mutually exclusive with -%c\n", programName, opt, 'A');
                    getoptErrorMessage(OPT_NONE, 0, NULL);
                    return -1;
                }

                xFlag = true;
                #endif

                precision = EXT_PRECISION;
                break;

            #ifdef MP_PREC
            case 'A':
                if (xFlag)
                {
                    fprintf(stderr, "%s: -%c: Option mutually exclusive with -%c\n", programName, opt, 'X');
                    getoptErrorMessage(OPT_NONE, 0, NULL);
                    return -1;
                }

                aFlag = true;
                precision = MUL_PRECISION;
                mpSignificandSize = MP_BITS_DEFAULT;

                if (optarg)
                {
                    argError = uLongArg(&tempUL, optarg, (unsigned long) MP_BITS_MIN, (unsigned long) MP_BITS_MAX);

                    if (argError != PARSE_SUCCESS)
                    {
                        break;
                    }
                    else if (tempUL < MPFR_PREC_MIN || tempUL > MPFR_PREC_MAX)
                    {
                        mpfr_fprintf(stderr, "%s: -%c: Argument out of range, it must be between %Pu and %Pu\n",
                            programName, opt, MPFR_PREC_MIN, MPFR_PREC_MAX);
                        argError = PARSE_ERANGE;
                        break;
                    }

                    mpSignificandSize = (mpfr_prec_t) tempUL;
                }

                break;
            #endif

            default:
                break;
        }

        #ifdef MP_PREC
        if (argError == PARSE_ERANGE) /* Error message already outputted */
        {
            getoptErrorMessage(OPT_NONE, 0, NULL);
            return -1;
        }
        else if (argError != PARSE_SUCCESS) /* Error but no error message, yet */
        {
            getoptErrorMessage(OPT_EARG, opt, NULL);
            return -1;
        }
        #endif
    }

    return 0;
}


/* Do one getopt pass to get the plot type */
static PlotType parsePlotType(int argc, char **argv)
{
    const char *GETOPT_STRING = ":j:";

    const struct option LONG_OPTIONS[] =
    {
        {"julia", required_argument, NULL, 'j'}, /* Plot a Julia set with specified constant */
        {0, 0, 0, 0}
    };

    PlotType plot = PLOT_MANDELBROT;

    optind = 1;
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
    const char *GETOPT_STRING = ":o:t";

    const struct option LONG_OPTIONS[] =
    {
        {0, 0, 0, 0}
    };

    OutputType output = OUTPUT_PNM;
    bool oFlag = false, tFlag = false;

    optind = 1;
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
    const char *GETOPT_STRING = ":x:";

    const struct option LONG_OPTIONS[] =
    {
        {"centre", required_argument, NULL, 'x'}, /* Centre coordinate and magnification of plot */
        {0, 0, 0, 0}
    };

    optind = 1;
    while ((opt = getopt_long(argc, argv, GETOPT_STRING, LONG_OPTIONS, NULL)) != -1)
    {
        if (opt == 'x') /* Centre coordinate and magnification of plot */
        {
            ParseErr argError = PARSE_EERR;

            if (precision == STD_PRECISION)
            {
                argError = magArg(p, optarg, COMPLEX_MIN, COMPLEX_MAX, MAGNIFICATION_MIN, MAGNIFICATION_MAX);
            }
            else if (precision == EXT_PRECISION)
            {
                argError = magArgExt(p, optarg, COMPLEX_MIN_EXT, COMPLEX_MAX_EXT, MAGNIFICATION_MIN, MAGNIFICATION_MAX);
            }

            #ifdef MP_PREC
            else if (precision == MUL_PRECISION)
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