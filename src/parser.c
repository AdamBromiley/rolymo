#include <parser.h>


/* Convert string to double and handle errors */
int stringToDouble(char *string, double *x, double xMin, double xMax)
{
    char *endptr;

    errno = 0;

    *x = strtod(string, &endptr);
    
    if (endptr == string)
    {
        /* Failed to convert */
        return PARSER_ERROR;
    }
    else if (errno == ERANGE)
    {
        /* Out of range of double */
        return PARSER_ERANGE;
    }
    else if (*x < xMin)
    {
        return PARSER_EMIN;
    }
    else if (*x > xMax)
    {
        return PARSER_EMAX;
    }
    
    if (*endptr != '\0')
    {
        /* More characters in string (might not be considered an error) */
        return PARSER_EEND;
    }

    return PARSER_NONE;
}


/* Convert string to unsigned long and handle errors */
int stringToULong(const char *string, unsigned long int *x, unsigned long int xMin, unsigned long int xMax, int base)
{
    char sign;
    char *endptr;

    if ((base < 2 && base != 0) || base > 36)
    {
        return PARSER_EBASE;
    }

    /* Get pointer to start of number */
    while (isspace(*string) != 0)
    {
        string++;
    }
    sign = *string;

    errno = 0;

    *x = strtoul(string, &endptr, base);
    
    if (endptr == string || errno == EINVAL)
    {
        /* Failed to convert */
        return PARSER_ERROR;
    }
    else if (errno == ERANGE)
    {
        /* Out of range of long int */
        return PARSER_ERANGE;
    }
    else if (*x < xMin)
    {
        return PARSER_EMIN;
    }
    else if (*x > xMax)
    {
        return PARSER_EMAX;
    }
    else if (sign == '-' && *x != 0)
    {
        return PARSER_EMIN;
    }
    
    if (*endptr != '\0')
    {
        /* More characters in string (might not be considered an error) */
        return PARSER_EEND;
    }

    return PARSER_NONE;
}


/* Convert string to uintmax_t and handle errors */
int stringToUIntMax(const char *string, uintmax_t *x, uintmax_t xMin, uintmax_t xMax, int base)
{
    char sign;
    char *endptr;

    if ((base < 2 && base != 0) || base > 36)
    {
        return PARSER_EBASE;
    }

    /* Get pointer to start of number */
    while (isspace(*string) != 0)
    {
        string++;
    }
    sign = *string;

    errno = 0;

    *x = strtoumax(string, &endptr, base);
    
    if (endptr == string || errno == EINVAL)
    {
        /* Failed to convert */
        return PARSER_ERROR;
    }
    else if (errno == ERANGE)
    {
        /* Out of range of uintmax_t */
        return PARSER_ERANGE;
    }
    else if (*x < xMin)
    {
        return PARSER_EMIN;
    }
    else if (*x > xMax)
    {
        return PARSER_EMAX;
    }
    else if (sign == '-' && *x != 0)
    {
        return PARSER_EMIN;
    }

    if (*endptr != '\0')
    {
        /* More characters in string (might not be considered an error) */
        return PARSER_EEND;
    }

    return PARSER_NONE;
}