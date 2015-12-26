#include "memalloc.h"

#define THE_MAGIC_NUMBER 1234567

#define toHeader(header) ((header_t*) header)
#define toFooter(footer) ((footer_t*) footer)

/****

ALL POINTERS ARE STORED AS CHAR*
TO SIMPLIFY POINTER ADDITION
AND HEADER/POINTER CONVERSION

****/

char *memoryLocation;
int memoryMagic; /* If this number is THE_MAGIC_NUMBER then
                    the memoryLocation is initialized */
char *lastLooked;
char *endOfMemory;

/**
-----------------------------------------------
-------	Private Functions ---------------------
**/

char *getFooterPointer(char *header) {
	return header + toHeader(header)->size + sizeof(header_t);
}

footer_t *getFooter(char *header) {
	return toFooter(getFooterPointer(header));
}

char *nextHeaderPointer(char *header) {
	return header + toHeader(header)->size
		+ sizeof(header_t) + sizeof(footer_t);
}

char *prevHeaderPointer(char *header) {
	char *prevFooter = header - sizeof(footer_t);
	if(prevFooter < memoryLocation) {
		return NULL;
	}
	
	return header - sizeof(footer_t) -
		toFooter(prevFooter)->size - sizeof(header_t);
}

/**
-----------------------------------------------
-------	Public Functions ----------------------
**/

/**
Initialize the managed memory
returns -1 if the OS cannot provide memory, or if M_Init has already been called
returns 0 otherwise
**/
int M_Init(int size)
{
	char *foot;
	char *head = mmap(NULL, size, PROT_READ|PROT_WRITE,
						MAP_ANON|MAP_PRIVATE, -1, 0);
	if(head == (void *) -1){
		return -1;
	}
	if(memoryMagic == THE_MAGIC_NUMBER) {
		/* M_Init has already been called */
		return -1;
	}
	lastLooked = head;
	memoryMagic = THE_MAGIC_NUMBER;
	toHeader(head)->size = size - sizeof(header_t) - sizeof(footer_t);

	foot = getFooterPointer(head);
	toFooter(foot)->size = toHeader(head)->size;
	
	toHeader(head)->magic = 0;
	toFooter(foot)->magic = 0;

	memoryLocation = head;
	endOfMemory = memoryLocation + size - 1;
	return 0;
}

/**
Allocate memory
returns NULL if no available space
**/
void *M_Alloc(int size)
{
	char *firstLooked = lastLooked;
	char *foot;
	char *firstFooter;
	char *secondHeader;
	char *coalesceHeader;
	int originalSize;
	int sizeWithOverhead;
	int looped = 0;
	if(memoryMagic != THE_MAGIC_NUMBER) {
		/* M_Init has not been called */
		return NULL;
	}
	if(size % 16 != 0) {
		size /= 16;
		size++;
		size *= 16;
	}
	
	sizeWithOverhead = size + sizeof(header_t) + sizeof(footer_t);
	
	/* While chunk allocated OR insufficient size chunk */
	while(toHeader(lastLooked)->magic == THE_MAGIC_NUMBER
			|| toHeader(lastLooked)->size < sizeWithOverhead) {
		lastLooked = nextHeaderPointer(lastLooked);
		if(looped == 1 && lastLooked > firstLooked) {
			/* No available memory chunk of sufficient size */
			return NULL;
		}
		if(lastLooked > endOfMemory) {
			/* Loop back to start of memory */
			lastLooked = memoryLocation;
			if(looped == 1) {
				/* No available memory chunk of sufficient size */
				return NULL;
			}
			looped = 1;
		}
	}
	
	if(toHeader(lastLooked)->size > sizeWithOverhead) {
		/* MAKE NEW CHUNK FOR THE REMAINING SPACE */
		originalSize = toHeader(lastLooked)->size;
		
		/* Set the size of the original header */
		toHeader(lastLooked)->size = size;
		
		/* Make the new footer for the first chunk */
		firstFooter = getFooterPointer(lastLooked);
		toFooter(firstFooter)->size = toHeader(lastLooked)->size;
		
		/* Make the new header for the second chunk */
		secondHeader = nextHeaderPointer(lastLooked);
		toHeader(secondHeader)->size = originalSize - sizeWithOverhead;
		
		/* Find the second footer and set its new size */
		foot = getFooterPointer(secondHeader);
		toFooter(foot)->size = toHeader(secondHeader)->size;
		
		/* Set magic of first chunk allocated */
		toHeader(lastLooked)->magic = THE_MAGIC_NUMBER;
		toFooter(firstFooter)->magic = THE_MAGIC_NUMBER;
		
		/* Set magic of second chunk to not allocated */
		toHeader(secondHeader)->magic = 0;
		toFooter(foot)->magic = 0;
		
		coalesceHeader = nextHeaderPointer(secondHeader);
		if(coalesceHeader < endOfMemory) {
			if(toHeader(coalesceHeader)->magic != THE_MAGIC_NUMBER) {
				/* Next chunk is unallocated */
				toHeader(secondHeader)->size += sizeof(footer_t) +
						sizeof(header_t) + toHeader(coalesceHeader)->size;
				getFooter(secondHeader)->size = toHeader(secondHeader)->size;
			}
		}
	} else if(toHeader(lastLooked)->size > size) {
		/* Set magic to allocated */
		toHeader(lastLooked)->magic = THE_MAGIC_NUMBER;
		getFooter(lastLooked)->magic = THE_MAGIC_NUMBER;
	}
	
	return (void*) (lastLooked + sizeof(header_t));
}

/**
Free memory
returns -1 if M_Free was called on unallocated memory
returns 0 otherwise
**/
int M_Free(void *p)
{
	char *thisHeader = (char*)p - sizeof(header_t);
	char *coalesceFooter = getFooterPointer(thisHeader);
	char *coalesceHeader = thisHeader;
	int size;
	if(memoryMagic != THE_MAGIC_NUMBER) {
		/* M_Init has not been called */
		return -1;
	}
	
	if(toHeader(thisHeader)->magic != THE_MAGIC_NUMBER) {
		/* This chunk is not allocated */
		return -1;
	}
	
	getFooter(thisHeader)->magic = 0;
	toHeader(thisHeader)->magic = 0;
	size = toHeader(thisHeader)->size;
	
	if(prevHeaderPointer(thisHeader) != NULL &&
		toHeader(prevHeaderPointer(thisHeader))->magic != THE_MAGIC_NUMBER) {
		coalesceHeader = prevHeaderPointer(thisHeader);
		size += toHeader(coalesceHeader)->size + sizeof(header_t) + sizeof(footer_t);
	}
	if(nextHeaderPointer(thisHeader) < endOfMemory &&
		toHeader(nextHeaderPointer(thisHeader))->magic != THE_MAGIC_NUMBER) {
		coalesceFooter = getFooterPointer(nextHeaderPointer(thisHeader));
		size += toFooter(coalesceFooter)->size + sizeof(header_t) + sizeof(footer_t);
	}
	
	/* Set magic to unallocated */
	toHeader(coalesceHeader)->size = size;
	getFooter(coalesceHeader)->size = size;
	toHeader(coalesceHeader)->magic = 0;
	getFooter(coalesceHeader)->magic = 0;
	
	return 0;
}

/**
Displays all memory chunks
**/
void M_Display()
{
	char *current = memoryLocation;
	if(memoryMagic != THE_MAGIC_NUMBER) {
		/* M_Init has not been called */
		printf("Memory has not been initialized\n");
		return;
	} else {
		printf("Printing all memory chunks:\n");
		while(current <= endOfMemory) {
			if(toHeader(current)->magic == THE_MAGIC_NUMBER) {
				printf("Allocated  : %p is size %d\n", current, toHeader(current)->size);
				current = nextHeaderPointer(current);
			} else {
				if(toHeader(current)->size == 0) {
					printf("Error      : %p has a null size\n", current);
					/* Find next valid chunk and continue */
					while(toHeader(current)->size == 0) {
						current = nextHeaderPointer(current);
					}
				} else {
					printf("Free       : %p is size %d\n", current, toHeader(current)->size);
					current = nextHeaderPointer(current);
				}
			}
		}
	}
}
