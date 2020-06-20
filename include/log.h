#ifndef LOG_H
#define LOG_H


#include <stddef.h>


#define VERBOSITY_STRING_LENGTH_MAX 16
#define SEVERITY_STRING_LENGTH_MAX 8


enum Verbosity
{
        QUIET,
        VERBOSE
};

enum LogSeverity
{
        LOG_NONE,
        FATAL,
        ERROR,
        WARNING,
        INFO,
        DEBUG
};


const enum LogSeverity LOG_SEVERITY_MIN;
const enum LogSeverity LOG_SEVERITY_MAX;


void logMessage(enum LogSeverity messageLevel, const char *formatString, ...);
int initialiseLog(enum Verbosity mode, enum LogSeverity level, const char *filePath);
int closeLog(void);

void getVerbosityString(char *dest, enum Verbosity verbosity, size_t n);
void getSeverityString(char *dest, enum LogSeverity severity, size_t n);


#endif