#ifndef LOG_H
#define LOG_H


enum Verbosity
{
        QUIET,
        VERBOSE
};


enum LogSeverity
{
        FATAL,
        ERROR,
        WARNING,
        INFO,
        DEBUG
};


void logMessage(enum LogSeverity messageLevel, const char *formatString, ...);
int initialiseLog(enum Verbosity mode, enum LogSeverity level, const char *filePath);
int closeLog(void);


#endif