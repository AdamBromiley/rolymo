#include <main.h>


int main(int argc, char **argv) /* Take in command-line options */
{
    const char *programName = argv[0];

    int optionID;

    int result;

    enum PlotType plotType = PLOT_NONE;
    struct PlotCTX parameters;

    char *outputFilePath;
    FILE *outputFile;
    
    #include <sys/sysinfo.h>

/* Get number of online processors */
    processorCount = get_nprocs();

    /* Command-line options */
    struct option longOptions[] =
    {
        {"j", no_argument, NULL, 'j'}, /* Plot a Julia set */
        {"m", no_argument, NULL, 'm'}, /* Plot the Mandlebrot set */
        {"t", no_argument, &parameters.output, OUTPUT_TERMINAL}, /* Output to terminal */
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
    
    opterr = 0;
    
    while ((optionID = getopt_long(argc, argv, OPTSTRING, longOptions, NULL)) != -1)
    {
        /* Set default parameters for plot type */
        if (optind == 1)
        {
            switch (optionID)
            {
                case 'j':
                    plotType = PLOT_JULIA;
                    break;
                case 'm':
                    plotType = PLOT_MANDELBROT;
                    break;
                default:
                    fprintf(stderr, "%s: First option must specify plot type\n", programName);
                    return getoptErrorMessage(OPT_NONE, programName, 0, NULL);
            }

            if (initialiseParameters(&parameters, plotType) != 0)
            {
                return getoptErrorMessage(OPT_ERROR, programName, 0, NULL);
            }

            continue;
        }

        switch (optionID)
        {
            case 0:
                break;
            case 'j':
                return getoptErrorMessage(OPT_EMANY, programName, optionID, NULL);
            case 'm':
                return getoptErrorMessage(OPT_EMANY, programName, optionID, NULL);
            case 't':
                break;
            case 'o':
                outputFilePath = optarg;
                break;
            case 'x':
                result = doubleArgument(&(parameters.minimum.re), optarg, COMPLEX_MIN.re, COMPLEX_MAX.re, programName, optionID);

                if (result == PARSER_ERANGE)
                {
                    return getoptErrorMessage(OPT_NONE, programName, optionID, NULL);
                }
                else if (result != PARSER_NONE)
                {
                    return getoptErrorMessage(OPT_EARG, programName, optionID, NULL);
                }

                break;
            case 'X':
                result = doubleArgument(&(parameters.maximum.re), optarg, COMPLEX_MIN.re, COMPLEX_MAX.re, programName, optionID);

                if (result == PARSER_ERANGE)
                {
                    return getoptErrorMessage(OPT_NONE, programName, optionID, NULL);
                }
                else if (result != PARSER_NONE)
                {
                    return getoptErrorMessage(OPT_EARG, programName, optionID, NULL);
                }

                break;
            case 'y':
                result = doubleArgument(&(parameters.minimum.im), optarg, COMPLEX_MIN.im, COMPLEX_MAX.im, programName, optionID);

                if (result == PARSER_ERANGE)
                {
                    return getoptErrorMessage(OPT_NONE, programName, optionID, NULL);
                }
                else if (result != PARSER_NONE)
                {
                    return getoptErrorMessage(OPT_EARG, programName, optionID, NULL);
                }

                break;
            case 'Y':
                result = doubleArgument(&(parameters.maximum.im), optarg, COMPLEX_MIN.im, COMPLEX_MAX.im, programName, optionID);

                if (result == PARSER_ERANGE)
                {
                    return getoptErrorMessage(OPT_NONE, programName, optionID, NULL);
                }
                else if (result != PARSER_NONE)
                {
                    return getoptErrorMessage(OPT_EARG, programName, optionID, NULL);
                }

                break;
            case 'i':
                result = uLongArgument(&(parameters.iterations), optarg, ITERATIONS_MIN, ITERATIONS_MAX, programName, optionID);

                if (result == PARSER_ERANGE)
                {
                    return getoptErrorMessage(OPT_NONE, programName, optionID, NULL);
                }
                else if (result != PARSER_NONE)
                {
                    return getoptErrorMessage(OPT_EARG, programName, optionID, NULL);
                }

                break;
            case 'r':
                result = uIntMaxArgument(&(parameters.width), optarg, WIDTH_MIN, WIDTH_MAX, programName, optionID);

                if (result == PARSER_ERANGE)
                {
                    return getoptErrorMessage(OPT_NONE, programName, optionID, NULL);
                }
                else if (result != PARSER_NONE)
                {
                    return getoptErrorMessage(OPT_EARG, programName, optionID, NULL);
                }

                break;
            case 's':
                result = uIntMaxArgument(&(parameters.height), optarg, HEIGHT_MIN, HEIGHT_MAX, programName, optionID);

                if (result == PARSER_ERANGE)
                {
                    return getoptErrorMessage(OPT_NONE, programName, optionID, NULL);
                }
                else if (result != PARSER_NONE)
                {
                    return getoptErrorMessage(OPT_EARG, programName, optionID, NULL);
                }

                break;
            case 'c':
                result = uLongArgument(&(parameters.colour), optarg, COLOUR_MIN, COLOUR_MAX, programName, optionID);

                if (result == PARSER_ERANGE)
                {
                    return getoptErrorMessage(OPT_NONE, programName, optionID, NULL);
                }
                else if (result != PARSER_NONE)
                {
                    return getoptErrorMessage(OPT_EARG, programName, optionID, NULL);
                }

                break;
            case 'h':
                return usage(programName);
            case '?':
                return getoptErrorMessage(OPT_EOPT, programName, optionID, NULL);
            case ':':
                return getoptErrorMessage(OPT_ENOARG, programName, optionID, NULL);
            default:
                return getoptErrorMessage(OPT_ERROR, programName, optionID, NULL);
        }
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


int usage(const char *programName)
{
    printf("Usage: %s [-j|-m] [-t|-p [-o FILENAME] [--colour=SCHEME]] [PLOT PARAMETERS...]\n", programName);
    printf("       %s --help\n\n", programName);
    printf("A Mandelbrot and Julia set plotter.\n\n");
    printf("Mandatory arguments to long options are mandatory for short options too.\n");
    printf("Plot type:\n");
    printf("  -j                           Julia set (c variable is prompted for during execution)\n");
    printf("  -m                           Mandelbrot set\n\n");
    printf("Output formatting:\n");
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
int doubleArgument(double *x, const char *argument, double xMin, double xMax, char *programName, char optionID)
{
    int result;

    result = stringToDouble(argument, x, xMin, xMax);
                
    if (result != PARSER_NONE)
    {
        if (result == PARSER_ERANGE || result == PARSER_EMIN || result == PARSER_EMAX)
        {
            fprintf(stderr, "%s: -%c: Argument out of range, it must be between %.*g and %.*g\n", 
                programName, optionID, DBL_PRECISION, xMin, DBL_PRECISION, xMax);
            return PARSER_ERANGE;
        }

        return PARSER_ERROR;
    }

    return PARSER_NONE;
}


/* Wrapper for stringToULong() */
int uLongArgument(unsigned long int *x, const char *argument, unsigned long int xMin, unsigned long int xMax,
                     char *programName, char optionID)
{
    const int BASE = 10;
    int result;

    result = stringToULong(argument, x, xMin, xMax, BASE);
                
    if (result != PARSER_NONE)
    {
        if (result == PARSER_ERANGE || result == PARSER_EMIN || result == PARSER_EMAX)
        {
            fprintf(stderr, "%s: -%c: Argument out of range, it must be between %lu and %lu\n", 
                programName, optionID, xMin, xMax);
            return PARSER_ERANGE;
        }

        return PARSER_ERROR;
    }

    return PARSER_NONE;
}


/* Wrapper for stringToUIntMax() */
int uIntMaxArgument(uintmax_t *x, const char *argument, uintmax_t xMin, uintmax_t xMax,
                       char *programName, char optionID)
{
    const int BASE = 10;
    int result;

    result = stringToUIntMax(argument, x, xMin, xMax, BASE);
                
    if (result != PARSER_NONE)
    {
        if (result == PARSER_ERANGE || result == PARSER_EMIN || result == PARSER_EMAX)
        {
            fprintf(stderr, "%s: -%c: Argument out of range, it must be between %" PRIuMAX " and %" PRIuMAX "\n", 
                programName, optionID, xMin, xMax);
            return PARSER_ERANGE;
        }

        return PARSER_ERROR;
    }

    return PARSER_NONE;
}


/* Convert error code to message when parsing command line arguments */
int getoptErrorMessage(enum GetoptError optionError, const char *programName, char shortOption, const char *longOption)
{
    switch (optionError)
    {
        case OPT_NONE:
            break;
        case OPT_ERROR:
            fprintf(stderr, "%s: Unknown error when reading command-line options\n", programName);
            break;
        case OPT_EOPT:
            if (shortOption == 0)
            {
                fprintf(stderr, "%s: Invalid option: \'%s\'\n", programName, longOption);
            }
            else
            {
                fprintf(stderr, "%s: Invalid option: \'-%c\'\n", programName, shortOption);
            }
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