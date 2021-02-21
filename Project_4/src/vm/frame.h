#ifndef FRAME_H
#define FRAME_H

#include "threads/thread.h"
#include "threads/palloc.h"
#include <list.h>

//Frame Table Entry
struct FTE{
	void *vaddr;
	void *paddr;

	struct list_elem elemL;
	struct thread *mapped_thread;
	int block_idx;
	
	bool pinned;
	bool reference_bit;
};

//Frame Table
struct list Frame_Table;

void frame_init(void);
struct FTE* frame_push(void *vaddr, void *paddr, bool writable);
void set_pin(void *vaddr, bool tf);
#endif
