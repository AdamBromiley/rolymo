#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


#define TIME_STRING_LENGTH_MAX 32
#define SEVERITY_STRING_LENGTH_MAX 10
#define LOG_MESSAGE_LENGTH_MAX 256
#define LOG_ENTRY_LENGTH_MAX 512

#define RFC3339_TIME_FORMAT_STRING "%Y-%m-%d %H:%M:%S"


const enum LogSeverity LOG_SEVERITY_MIN = LOG_NONE;
const enum LogSeverity LOG_SEVERITY_MAX = DEBUG;


static enum Verbosity verbose = QUIET;
static enum LogSeverity loggingLevel = INFO;
static FILE *logFile = NULL;


static void getTime(char *dest, size_t n);
static void getSeverityString(char *dest, enum LogSeverity severity, size_t n);


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
    logMessage(DEBUG, "Closing log file");

    if (fclose(logFile))
    {
        logFile = NULL;
        logMessage(ERROR, "Log file could not be closed");
        return 1;
    }

    logFile = NULL;
    logMessage(DEBUG, "Log file closed");
    return 0;
}


/* Get date and time in RFC 3339 format - YYYY-MM-DD hh:mm:ss */
static void getTime(char *dest, size_t n)
{
    const time_t CURRENT_TIME = time(NULL);
    const struct tm *CURRENT_TIME_STRUCTURED = localtime(&CURRENT_TIME);

    strftime(dest, n, RFC3339_TIME_FORMAT_STRING, CURRENT_TIME_STRUCTURED);

    dest[n - 1] = '\0';

    return;
}


/* Convert severity level to a string */
static void getSeverityString(char *dest, enum LogSeverity severity, size_t n)
{
    switch (severity)
    {
        case DEBUG:
            strncpy(dest, "DEBUG", n);
            break;
        case INFO:
            strncpy(dest, "INFO", n);
            break;
        case WARNING:
            strncpy(dest, "WARNING", n);
            break;
        case ERROR:
            strncpy(dest, "ERROR", n);
            break;
        case FATAL:
            strncpy(dest, "FATAL", n);
            break;
        default:
            strncpy(dest, "NONE", n);
            break;
    }

    dest[n - 1] = '\0';

    return;
}