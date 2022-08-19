#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "connection.h"


Connection createConnection(void)
{
    Connection c =
    {
        .rowAllocated = false,
        .row = 0,
        .n = 0,
        .read = 0,
        .buffer = NULL
    };

    return c;
}


int createClientReceiveBuffer(Connection *c, size_t n)
{
    if (c)
    {
        c->buffer = calloc(1, n);

        if (c->buffer)
        {
            c->n = n;
            c->read = 0;
            return 0;
        }
    }

    return 1;
}


void clearClientReceiveBuffer(Connection *c)
{
    memset(c->buffer, 0, c->n);
    c->read = 0;
}


void freeClientReceiveBuffer(Connection *c)
{
    if (c)
    {
        free(c->buffer);
        c->n = 0;
        c->read = 0;
        c->buffer = NULL;
    }
}