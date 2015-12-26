#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "memalloc.h"

/**
	Slams memalloc with a bunch of random M_Alloc and M_Free requests
**/
int main(int argc, char *argv[]){
	void *test[10];
	void *temp;
	int r, i, x, y;
	
	M_Init(8192);
	
	srand(time(NULL));
	for(i = 0; i < 10; i++) {
		test[i] = M_Alloc(rand() % 64 + 1);
	}
	temp = test[0];
	for(i = 0; i < 100000; i++) {
		r = rand() % 2;
		if(r == 1) {
			x = rand() % 10;
			y = rand() % 1024 + 1;
			temp = test[x];
			test[x] = M_Alloc(y);
			if(test[x] == NULL) {
				test[x] = temp;
			}
			
		} else {
			M_Free(test[rand() % 10]);
		}
	}
	M_Display();
	
	exit(0);
}