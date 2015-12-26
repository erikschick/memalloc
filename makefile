all: testmem

memalloc: memalloc.o
	ar rs memalloc.a memalloc.o

testmem: memalloc
	gcc -g -pedantic -Wall -o testmem testmem.c memalloc.a

memalloc.o:
	gcc -c -Wall -pedantic memalloc.c memalloc.h

.PHONY: clean
	
clean:
	rm -f testmem.exe memalloc.o memalloc.a *.h.gch