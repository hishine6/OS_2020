#include<stdio.h>
#include<stdlib.h>
#include<bitmap.h>

#include "frame.h"
#include "page.h"
#include "swap.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include "devices/block.h"
#include "userprog/pagedir.h"

struct block *swap_slot; // swap table
bool *swap_check;
static const size_t SEC_PER_PAGE=PGSIZE/BLOCK_SECTOR_SIZE;
struct list_elem *e;

void swap_init(){
	swap_slot = block_get_role(BLOCK_SWAP);
	ASSERT(swap_slot != NULL);
	if(!swap_slot) PANIC("Panic: swap_init()");
	swap_check = (bool*)malloc(sizeof(bool)*(block_size(swap_slot)/SEC_PER_PAGE));
	for(int i=0;i<(int)(block_size(swap_slot)/SEC_PER_PAGE);i++){
		swap_check[i]=false;
	}
	e=NULL;
	ASSERT(swap_check!=NULL);
}



void swap_in(void *upage, void *kpage, int idx){
	struct PTE *target;

	ASSERT (upage !=NULL);
	ASSERT (swap_check[idx])
	
	//swapin using block read
	for(size_t i=0; i< SEC_PER_PAGE;i++)
		block_read(swap_slot, idx*SEC_PER_PAGE+i,kpage+(BLOCK_SECTOR_SIZE*i));
	

	swap_check[idx]=false;
	target=page_lookup(&thread_current()->Page_Table,upage);
	target->idx=-1;
	pagedir_set_page(thread_current()->pagedir,pg_round_down(upage),pg_round_down(kpage),target->rw_bit);

}



int swap_out(void *upage, void *kpage){	
	size_t size=block_size(swap_slot)/SEC_PER_PAGE;
	int idx=0; 
	struct PTE* temp;
	//get unused block
	for(size_t i=0;i<size;i++){
		if(swap_check[i]==false){
			idx=i;
			swap_check[i]=true;
			break;
		}
	}
	//swapping out with block write
	for(size_t i=0;i<SEC_PER_PAGE;i++){
		block_write(swap_slot,idx*SEC_PER_PAGE+i,kpage+(BLOCK_SECTOR_SIZE*i));
	}

	//find the swapped out user page table entry
	temp = page_lookup(&thread_current()->Page_Table,upage);
	temp->idx=idx; //save the block number where page is written
	return idx;
}


void *replacement(enum palloc_flags flag){
	
	/*find victim with FIFO*/
	/*swap out if needed*/
	/*return the free frame*/ 
	
	void *result_page;
	struct FTE * victim=NULL;
	struct list_elem *e =NULL;
	int idx=-1;
	//Find Victim with second chance

	if(e==NULL)
		e = list_front(&Frame_Table);
	
	while(1){
		struct FTE *temp=list_entry(e,struct FTE,elemL);
		if(temp->pinned == true){
			e=list_next(e);
			continue;
		}

		if(temp->reference_bit==false){
			victim=temp;
			break;
		}
		else
			temp->reference_bit=false;

		e=list_next(e);
		if(e==list_end(&Frame_Table))
			e=list_front(&Frame_Table);
	}

	//victim=list_entry(e,struct FTE, elemL);
	
	ASSERT(victim !=NULL && victim->mapped_thread !=NULL)
	
	//Swap out if needed
	idx=swap_out(victim->vaddr, victim->paddr);
	

	//delete victim from frame table
	struct PTE *temp;
	temp = page_lookup(&victim->mapped_thread->Page_Table,victim->vaddr);
	temp->paddr=NULL;
	temp->idx=idx;
	palloc_free_page(victim->paddr);
	pagedir_clear_page(victim->mapped_thread->pagedir,victim->vaddr);
	
	list_remove(&victim->elemL);

	//return address of the victim to use
	free(victim);
	result_page=palloc_get_page(flag); // this will give you the free page
	ASSERT(result_page !=NULL);
	return result_page;
}
