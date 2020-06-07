#include <log.h>


_Bool verbose = DEFAULT_VERBOSITY;
enum LogSeverity loggingLevel = DEFAULT_LOGGING_LEVEL;

static FILE *logFile = NULL;


/* Get date and time in RFC 3339 format - YYYY-MM-DD hh:mm:ss */
void getTime(char *buffer, size_t bufferSize)
{
    const char *RFC3339_TIME_FORMAT_STRING = "%Y-%m-%d %H:%M:%S";

    const time_t CURRENT_TIME = time(NULL);
    struct tm *CURRENT_TIME_STRUCTURED = localtime(&CURRENT_TIME);

    strftime(buffer, bufferSize, RFC3339_TIME_FORMAT_STRING, CURRENT_TIME_STRUCTURED);

    buffer[bufferSize - 1] = 0;

    return;
}


/* Convert severity level to a string */
void getSeverityString(char *buffer, enum LogSeverity messageLevel, size_t bufferSize)
{
    switch (messageLevel)
    {
        case DEBUG:
            strncpy(buffer, "DEBUG", bufferSize);
            break;
        case INFO:
            strncpy(buffer, "INFO", bufferSize);
            break;
        case WARNING:
            strncpy(buffer, "WARNING", bufferSize);
            break;
        case ERROR:
            strncpy(buffer, "ERROR", bufferSize);
            break;
        case FATAL:
            strncpy(buffer, "FATAL", bufferSize);
            break;
        default:
            strncpy(buffer, "NONE", bufferSize);
            break;
    }

    buffer[bufferSize - 1] = 0;

    return;
}


void logMessage(enum LogSeverity messageLevel, const char *formatString, ...)
{
    va_list formatArguments;

    char timeString[TIME_STRING_LENGTH_MAX + 1];
    char severityString[SEVERITY_STRING_LENGTH_MAX + 1];
    char message[LOG_MESSAGE_LENGTH_MAX + 1];
    char logEntry[LOG_ENTRY_LENGTH_MAX + 1];
    
    /* Ignore if there's nowhere to log to */
    if (logFile == NULL && verbose != 1)
    {
        return;
    }

    /* Ignore if message not severe enough for the chosen logging level */
    if (loggingLevel < messageLevel)
    {
        return;
    }

    /* Get date and time in RFC 3339 format - YYYY-MM-DD hh:mm:ss */
    getTime(timeString, sizeof(timeString));

    /* Convert severity level to a string */
    getSeverityString(messageLevel, severityString, sizeof(severityString));

    /* Read all arguments to logMessage() after the format string */
    va_start(formatArguments, formatString);
    vsnprintf(message, sizeof(message), formatString, formatArguments);
    va_end(formatArguments);

    /* Construct log message */
    snprintf(logEntry, sizeof(logEntry), "[%s] %-*s %s\n", timeString, sizeof(severityString) - 1, severityString, message);

    if (logFile != NULL)
    {
        fprintf(logFile, "%s", logEntry);
    }

    if (verbose == 1)
    {
        fprintf(stderr, "%s", logEntry);
    }

    return;
}


int initialiseLog(const char *logFilePath, const char *programName)
{
    logMessage(DEBUG, "Initialising log file \"%s\"", logFilePath);

    if ((logFile = fopen(logFilePath, "a")) == NULL)
    {
        fprintf(stderr, "%s: Log file could not be intialised\n", programName);
        return 1;
    }

    logMessage(DEBUG, "Log file initialised");
    return 0;
}


int closeLog(void)
{
    logMessage(DEBUG, "Closing log file");

    if (fclose(logFile) != 0)
    {
        logFile = NULL;
        logMessage(WARNING, "Log file could not be closed");
        return 1;
    }

    return 0;
}