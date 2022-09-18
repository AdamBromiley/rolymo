#ifndef FUNCTION_H
#define FUNCTION_H


void * generateFractalRow(void *threadInfo);
void * generateFractalRowExt(void *threadInfo);
void * generateFractalRowMP(void *threadInfo);

void * generateFractal(void *threadInfo);
void * generateFractalExt(void *threadInfo);
void * generateFractalMP(void *threadInfo);


#endif