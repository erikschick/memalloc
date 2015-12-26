#ifndef _MEMALLOC_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

typedef struct __header_t {
	int size;
	int magic;  /* If this number is THE_MAGIC_NUMBER then
                   the chunk is allocated */
} header_t;

typedef struct __footer_t {
	int size;
	int magic;  /* If this number is THE_MAGIC_NUMBER then
                   the chunk is allocated */
} footer_t;

extern char *memoryLocation;
extern int memoryMagic; /* If this number is THE_MAGIC_NUMBER then
                           the memoryLocation is initialized */
extern char *lastLooked;
extern char *endOfMemory;

/**
Initialize the managed memory
returns -1 if the OS cannot provide memory, or if M_Init has already been called
returns 0 otherwise
**/
int M_Init(int);


/**
Allocate memory
returns NULL if no available space
**/
void *M_Alloc(int);

/**
Free memory
returns -1 if M_Free was called on unallocated memory
returns 0 otherwise
**/
int M_Free(void*);

/**
Displays all memory chunks
**/
void M_Display();

#endif
