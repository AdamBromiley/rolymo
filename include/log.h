#ifndef LOG_H
#define LOG_H


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>


#define DEFAULT_VERBOSITY 0
#define DEFAULT_LOGGING_LEVEL INFO

#define TIME_STRING_LENGTH_MAX 32
#define SEVERITY_STRING_LENGTH_MAX 10
#define LOG_MESSAGE_LENGTH_MAX 256
#define LOG_ENTRY_LENGTH_MAX 512


enum LogSeverity
{
        FATAL,
        ERROR,
        WARNING,
        INFO,
        DEBUG
};


extern _Bool verbose;
extern enum LogSeverity loggingLevel;


void getTime(char *buffer, size_t bufferSize);
void getSeverityString(char *buffer, enum LogSeverity messageLevel, size_t bufferSize);
void logMessage(enum LogSeverity messageLevel, const char *formatString, ...);

int initialiseLog(const char *logFilePath, const char *programName);
int closeLog(void);


#endif