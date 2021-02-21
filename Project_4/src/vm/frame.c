#include<stdio.h>
#include<stdlib.h>

#include "frame.h"
#include "page.h"
#include "swap.h"
#include "threads/thread.h"

void frame_init(){
	list_init(&Frame_Table);
}

//adding new frame table entry to frame table
struct FTE *frame_push(void *vaddr, void *paddr, bool writable){
	 struct FTE * new =NULL;
	 
	 new=(struct FTE*)malloc(sizeof(struct FTE));
	 new->mapped_thread=thread_current();
	 
	 //when new frame needed
	 if(paddr==NULL){
	  paddr=palloc_get_page(PAL_USER); //getting page from user pool
		if(paddr ==NULL) // no more frame available
			paddr = replacement(PAL_USER);// replacement strategy needed
	 }
	 new->vaddr=vaddr;
	 new->paddr=paddr;
	 new->pinned=false;
	 new->reference_bit=true;
	 list_push_back(&Frame_Table,&new->elemL);
	 return new;
}


void set_pin(void *t_vaddr, bool tf){
	struct PTE tempPTE;
	struct hash_elem *e;
	struct PTE *pte;

	tempPTE.vaddr=t_vaddr;
	e=hash_find(&thread_current()->Page_Table,&tempPTE.H);
	pte=hash_entry(e,struct PTE, H);

	if(pte){
		struct list_elem *e;
		for(e=list_begin(&Frame_Table);e!=list_end(&Frame_Table);e=list_next(e)){
			struct FTE*temp=list_entry(e,struct FTE, elemL);
			if(temp->paddr==pte->paddr){
				temp->pinned=tf;
				break;
			}
		}
	}
}


