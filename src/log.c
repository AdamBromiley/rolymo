#include "log.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


#define TIME_STRING_LENGTH_MAX 32
#define LOG_MESSAGE_LENGTH_MAX 256
#define LOG_ENTRY_LENGTH_MAX 512

#define RFC3339_TIME_FORMAT_STRING "%Y-%m-%d %H:%M:%S"


/* Log severity range for user parameter checking */
const enum LogSeverity LOG_SEVERITY_MIN = LOG_NONE;
const enum LogSeverity LOG_SEVERITY_MAX = DEBUG;


/* Logging modes */
static enum Verbosity verbose = QUIET;
static enum LogSeverity loggingLevel = INFO;

/* Log file */
static FILE *logFile = NULL;


static void getTime(char *dest, size_t n);


/* Write to log */
void logMessage(enum LogSeverity messageLevel, const char *formatString, ...)
{
    va_list formatArguments;

    char timeString[TIME_STRING_LENGTH_MAX + 1];
    char severityString[SEVERITY_STRING_LENGTH_MAX + 1];
    char message[LOG_MESSAGE_LENGTH_MAX + 1];
    char logEntry[LOG_ENTRY_LENGTH_MAX + 1];
    
    /* Ignore if there's nowhere to log to */
    if (!logFile && verbose == QUIET)
        return;

    /* Ignore if message not severe enough for the chosen logging level */
    if (loggingLevel < messageLevel || loggingLevel == LOG_NONE)
        return;

    /* Get date and time in RFC 3339 format - YYYY-MM-DD hh:mm:ss */
    getTime(timeString, sizeof(timeString));

    /* Convert severity level to a string */
    getSeverityString(severityString, messageLevel, sizeof(severityString));

    /* Read all arguments to logMessage() after the format string */
    va_start(formatArguments, formatString);
    vsnprintf(message, sizeof(message), formatString, formatArguments);
    va_end(formatArguments);

    /* Construct log message */
    snprintf(logEntry, sizeof(logEntry), "[%s] %-*s %s\n", timeString, SEVERITY_STRING_LENGTH_MAX, severityString, message);

    /* Write to log */
    if (logFile)
        fprintf(logFile, "%s", logEntry);

    if (verbose == VERBOSE)
        fprintf(stderr, "%s", logEntry);

    return;
}


/* Open log file */
int initialiseLog(enum Verbosity mode, enum LogSeverity level, const char *filePath)
{
    verbose = mode;
    loggingLevel = level;

    if (loggingLevel == LOG_NONE)
        return 0;

    if (filePath)
    {
        logFile = fopen(filePath, "a");

        if (!logFile)
        {
            logMessage(ERROR, "Log file could not be opened");
            return 1;
        }
    }

    logMessage(DEBUG, "Log file initialised");
    return 0;
}


/* Close log file */
int closeLog(void)
{
    if (logFile)
    {
        logMessage(DEBUG, "Closing log file");

        if (fclose(logFile))
        {
            logFile = NULL;
            logMessage(ERROR, "Log file could not be closed");
            return 1;
        }

        logFile = NULL;
        logMessage(DEBUG, "Log file closed");
    }
    
    return 0;
}


/* Convert verbosity enum to a string */
void getVerbosityString(char *dest, enum Verbosity verbosity, size_t n)
{
    const char *verbosityString;

    switch (verbosity)
    {
        case QUIET:
            verbosityString = "QUIET";
            break;
        case VERBOSE:
            verbosityString = "VERBOSE";
            break;
        default:
            verbosityString = "-";
            break;
    }

    strncpy(dest, verbosityString, n);
    dest[n - 1] = '\0';

    return;
}


/* Convert severity level to a string */
void getSeverityString(char *dest, enum LogSeverity severity, size_t n)
{
    const char *severityString;

    switch (severity)
    {
        case DEBUG:
            severityString = "DEBUG";
            break;
        case INFO:
            severityString = "INFO";
            break;
        case WARNING:
            severityString = "WARNING";
            break;
        case ERROR:
            severityString = "ERROR";
            break;
        case FATAL:
            severityString = "FATAL";
            break;
        default:
            severityString = "NONE";
            break;
    }

    strncpy(dest, severityString, n);
    dest[n - 1] = '\0';

    return;
}


/* Get date and time in RFC 3339 format - YYYY-MM-DD hh:mm:ss */
static void getTime(char *dest, size_t n)
{
    time_t CURRENT_TIME = time(NULL);
    struct tm *CURRENT_TIME_STRUCTURED = localtime(&CURRENT_TIME);

    strftime(dest, n, RFC3339_TIME_FORMAT_STRING, CURRENT_TIME_STRUCTURED);

    dest[n - 1] = '\0';

    return;
}