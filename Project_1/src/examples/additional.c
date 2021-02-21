#include<stdio.h>
#include "syscall.h"
#include<stdlib.h>

#define MAX_NUM 5

int save[MAX_NUM];


int main(int argc, char *argv[]){
	if(argc!=5){
		printf("ERROR: INPUT SHOULD BE 4 INTIGERS\n");
		return EXIT_FAILURE;
	}

	for(int i=1;i<argc;i++){
		save[i]=atoi(argv[i]);
	}
	printf("%d\n",fibonacci(save[1]));
	printf("%d\n",max_of_four_int(save[1],save[2],save[3],save[4]));

	return EXIT_SUCCESS;
}
