#ifndef MANDELBROT_H
#define MANDELBROT_H


#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/sysinfo.h>


#define FILENAME_MAX_LEN 255
#define DEFAULT_FILENAME "output"



#define MALLOC_ESC_ITER 15

#define COLOUR_SCALE_MULTIPLIER 20
#define CHAR_SCALE_MULTIPLIER 0.3

#define BUF_SIZE 1024


#endif