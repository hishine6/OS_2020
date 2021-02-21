#ifndef PAGE_H
#define PAGE_H

#include "threads/thread.h"
#include "threads/palloc.h"
#include "frame.h"
#include<hash.h>

struct PTE{
	void *vaddr; //virtual address
	void *paddr; //physical address
	struct hash_elem H;
	int idx;
	
	bool present_bit;
	bool rw_bit;
	bool dirty_bit;

	bool swap_bit;
};

bool less_func(const struct hash_elem *a, const struct hash_elem *b,void *aux);
unsigned hash_func(const struct hash_elem *e, void *aux);

struct PTE *page_lookup(struct hash*H,void *page);
bool page_add(void *vaddr, void *paddr, bool writable);
#endif
