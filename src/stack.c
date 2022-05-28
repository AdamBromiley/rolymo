#include <stddef.h>
#include <stdlib.h>

#include "stack.h"


/* Create and allocate memory to the stack object */
Stack * createStack(size_t n)
{
    Stack *s = malloc(sizeof(*s));

    if (!s)
        return NULL;

    s->stack = malloc(n * sizeof(*(s->stack)));

    if (!s->stack)
    {
        free(s);
        return NULL;
    }

    s->max = n;
    s->n = 0;

    return s;
}


/* Add item to stack */
int pushStack(Stack *s, size_t n)
{
    if (s->n == s->max)
        return 1;
    
    s->stack[(s->n)++] = n;
    return 0;
}


/* Remove item from stack */
int popStack(size_t *n, Stack *s)
{
    if (s->n == 0)
        return 1;
    
    *n = s->stack[--(s->n)];
    return 0;
}


void freeStack(Stack *s)
{
    if (s)
        free(s->stack);

    free(s);
}