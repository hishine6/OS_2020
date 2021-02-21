#ifndef VM_SWAP_H
#define VM_SWAP_H
#include <stdlib.h>
#include<stdio.h>
void swap_init(void);
void swap_in(void *upage,void *kpage,int idx);
int swap_out(void *upage, void *kpage);
void *replacement(enum palloc_flags flag); 

#endif
