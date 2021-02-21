#include <stdio.h>
#include <stdlib.h>

#include "page.h"
#include "swap.h"
#include "threads/vaddr.h"


/*for Page table, hash*/
unsigned hash_func(const struct hash_elem *e,void *aux){
	struct PTE *temp;
	void *vaddr;
	temp=hash_entry(e,struct PTE, H);
	vaddr=temp->vaddr;
	return hash_bytes(&vaddr,sizeof(vaddr));
}

bool less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux){
	struct PTE *x=hash_entry(a,struct PTE,H);
	struct PTE *y=hash_entry(b,struct PTE,H);

	if(x->vaddr < y->vaddr)
		return true;
	else
		return false;
}


/*look up the page in the current running thread*/
struct PTE *page_lookup(struct hash *PT, void *page){
	struct PTE tempPTE;
	struct PTE *pte;
	struct hash_elem *e;

	void* user_page_start = pg_round_down(page);
	tempPTE.vaddr=user_page_start;
	e=hash_find(PT,&tempPTE.H);
	pte=hash_entry(e,struct PTE, H);
	
	if(e)
		return pte;
	else
		return NULL;
}

bool page_add(void *vaddr, void *paddr, bool writable){
	struct PTE *newpte;
	struct hash_elem *e;
	newpte = (struct PTE*)malloc(sizeof(struct PTE));
	newpte->vaddr=vaddr;
	newpte->paddr=paddr;
	newpte->idx=-1;
	newpte->present_bit=true;
	newpte->rw_bit=writable;
	
	e=hash_find(&thread_current()->Page_Table,&newpte->H);
	if(!e)
		hash_insert(&thread_current()->Page_Table, &newpte->H);
	else{
		free(newpte);
		newpte=hash_entry(e,struct PTE, H);
		newpte->idx=-1;
	}
	return true;		
}
