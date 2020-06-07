#ifndef MAIN_H
#define MAIN_H


#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>

#include <parser.h>
#include <parameters.h>


#define OPTSTRING ":jmto:x:X:y:Y:i:r:s:c:h"

#define DBL_PRECISION 2


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


int doubleArgument(double *x, const char *argument, double xMin, double xMax, char *programName, char optionID);
int getoptErrorMessage(enum GetoptError optionError, const char *programName, char shortOption, const char *longOption);


#endif