#ifndef STACK_H
#define STACK_H


#include <stddef.h>


/* Stack of size_t elements */
typedef struct Stack
{
    size_t max;    /* Maximum number of elements on stack */
    size_t n;      /* Current number of items on stack */
    size_t *stack;
} Stack;


Stack * createStack(size_t n);
int pushStack(Stack *s, size_t n);
int popStack(size_t *n, Stack *s);
void freeStack(Stack *s);


#endif