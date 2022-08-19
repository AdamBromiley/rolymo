#ifndef CONNECTION_H
#define CONNECTION_H


#include <stdbool.h>
#include <stddef.h>

#include <netinet/in.h>


typedef struct Connection
{
    struct sockaddr_in addr; /* Address */
    bool rowAllocated;       /* True if the worker has been allocated a row */
    size_t row;              /* Row number allocated to the worker */
    size_t n;                /* Receive buffer allocated size */
    size_t read;             /* Bytes of data present in the buffer */
    char *buffer;            /* Receive buffer */
} Connection;


Connection createConnection(void);

int createClientReceiveBuffer(Connection *c, size_t n);
void clearClientReceiveBuffer(Connection *c);
void freeClientReceiveBuffer(Connection *c);


#endif