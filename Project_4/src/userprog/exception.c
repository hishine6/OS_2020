#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"

/* Number of page faults processed. */
static long long page_fault_cnt;

static void kill (struct intr_frame *);
static void page_fault (struct intr_frame *);

/* Registers handlers for interrupts that can be caused by user
   programs.

   In a real Unix-like OS, most of these interrupts would be
   passed along to the user process in the form of signals, as
   described in [SV-386] 3-24 and 3-25, but we don't implement
   signals.  Instead, we'll make them simply kill the user
   process.

   Page faults are an exception.  Here they are treated the same
   way as other exceptions, but this will need to change to
   implement virtual memory.

   Refer to [IA32-v3a] section 5.15 "Exception and Interrupt
   Reference" for a description of each of these exceptions. */
void
exception_init (void) 
{
  /* These exceptions can be raised explicitly by a user program,
     e.g. via the INT, INT3, INTO, and BOUND instructions.  Thus,
     we set DPL==3, meaning that user programs are allowed to
     invoke them via these instructions. */
  intr_register_int (3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
  intr_register_int (4, 3, INTR_ON, kill, "#OF Overflow Exception");
  intr_register_int (5, 3, INTR_ON, kill,
                     "#BR BOUND Range Exceeded Exception");

  /* These exceptions have DPL==0, preventing user processes from
     invoking them via the INT instruction.  They can still be
     caused indirectly, e.g. #DE can be caused by dividing by
     0.  */
  intr_register_int (0, 0, INTR_ON, kill, "#DE Divide Error");
  intr_register_int (1, 0, INTR_ON, kill, "#DB Debug Exception");
  intr_register_int (6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
  intr_register_int (7, 0, INTR_ON, kill,
                     "#NM Device Not Available Exception");
  intr_register_int (11, 0, INTR_ON, kill, "#NP Segment Not Present");
  intr_register_int (12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
  intr_register_int (13, 0, INTR_ON, kill, "#GP General Protection Exception");
  intr_register_int (16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
  intr_register_int (19, 0, INTR_ON, kill,
                     "#XF SIMD Floating-Point Exception");

  /* Most exceptions can be handled with interrupts turned on.
     We need to disable interrupts for page faults because the
     fault address is stored in CR2 and needs to be preserved. */
  intr_register_int (14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* Prints exception statistics. */
void
exception_print_stats (void) 
{
  printf ("Exception: %lld page faults\n", page_fault_cnt);
}

/* Handler for an exception (probably) caused by a user process. */
static void
kill (struct intr_frame *f) 
{
  /* This interrupt is one (probably) caused by a user process.
     For example, the process might have tried to access unmapped
     virtual memory (a page fault).  For now, we simply kill the
     user process.  Later, we'll want to handle page faults in
     the kernel.  Real Unix-like operating systems pass most
     exceptions back to the process via signals, but we don't
     implement them. */
     
  /* The interrupt frame's code segment value tells us where the
     exception originated. */
  switch (f->cs)
    {
    case SEL_UCSEG:
      /* User's code segment, so it's a user exception, as we
         expected.  Kill the user process.  */
      printf ("%s: dying due to interrupt %#04x (%s).\n",
              thread_name (), f->vec_no, intr_name (f->vec_no));
      intr_dump_frame (f);
      thread_exit (); 

    case SEL_KCSEG:
      /* Kernel's code segment, which indicates a kernel bug.
         Kernel code shouldn't throw exceptions.  (Page faults
         may cause kernel exceptions--but they shouldn't arrive
         here.)  Panic the kernel to make the point.  */
      intr_dump_frame (f);
      PANIC ("Kernel bug - unexpected interrupt in kernel"); 

    default:
      /* Some other code segment?  Shouldn't happen.  Panic the
         kernel. */
      printf ("Interrupt %#04x (%s) in unknown segment %04x\n",
             f->vec_no, intr_name (f->vec_no), f->cs);
      thread_exit ();
    }
}

bool handle_mm_fault(void *fault_addr){
	struct PTE temp, *target=NULL;
	struct FTE *target_fte;
	
	temp.vaddr=pg_round_down(fault_addr); //the page start address that makes page fault
	
	//find the page table entry for the current page
	target = hash_entry(hash_find(&thread_current()->Page_Table,&temp.H),struct PTE, H);
	
	ASSERT(target!=NULL);
	target_fte=frame_push(pg_round_down(target->vaddr),pg_round_down(target->paddr),true);
	//get free frame address
	if(target_fte==NULL)
		return false; //frame allocate fail
	
	swap_in(target->vaddr,target_fte->paddr,target->idx);
	//swap in the needed page to free frame

	return true;
}

bool stack_grow(void *fault_addr){
	void *temp_esp = PHYS_BASE - PGSIZE; //minimum stack loaded
	void *goal_esp = pg_round_down(fault_addr);  //needed address (under PHYS_BASE)
	int pgNum = (PHYS_BASE - goal_esp) / PGSIZE; //needed page number
	void *temp;

	if(pgNum > 2048){ //over 8MB (=2^33)(=2048*4096)
		return false;
	}
	
	//check if there is unmapped address between PHYS_BASE, target address
	while(1){ 
		if(is_kernel_vaddr(temp_esp) || (pagedir_get_page(thread_current()->pagedir, (const void*)temp_esp)==NULL)) 
			break; 
		if(temp_esp < goal_esp) //one page needed
			break;
		else 
			temp_esp -= PGSIZE;
		pgNum--;
	}
	//add the needed stack between goal address, PHYS_BASE
	while(pgNum != 0){	
		temp = palloc_get_page(PAL_USER);
		if(!temp) { //if no physcial memory available, swapping
			temp = replacement(PAL_USER);
		}
		//give stack new memory
		pagedir_set_page(thread_current()->pagedir, goal_esp, temp, true); // add newly loaded stack
		page_add(goal_esp,temp,false);
		goal_esp = goal_esp + PGSIZE;
		pgNum--;
	}
	return true;
}


/* Page fault handler.  This is a skeleton that must be filled in
   to implement virtual memory.  Some solutions to project 2 may
   also require modifying this code.

   At entry, the address that faulted is in CR2 (Control Register
   2) and information about the fault, formatted as described in
   the PF_* macros in exception.h, is in F's error_code member.  The
   example code here shows how to parse that information.  You
   can find more information about both of these in the
   description of "Interrupt 14--Page Fault Exception (#PF)" in
   [IA32-v3a] section 5.15 "Exception and Interrupt Reference". */
static void
page_fault (struct intr_frame *f) 
{
  bool not_present;  /* True: not-present page, false: writing r/o page. */
  bool write;        /* True: access was write, false: access was read. */
  bool user;         /* True: access by user, false: access by kernel. */
	void *fault_addr;  /* Fault address. */

  /* Obtain faulting address, the virtual address that was
     accessed to cause the fault.  It may point to code or to
     data.  It is not necessarily the address of the instruction
     that caused the fault (that's f->eip).
     See [IA32-v2a] "MOV--Move to/from Control Registers" and
     [IA32-v3a] 5.15 "Interrupt 14--Page Fault Exception
     (#PF)". */
  asm ("movl %%cr2, %0" : "=r" (fault_addr));
	
	//read information about the page fault

  /* Turn interrupts back on (they were only off so that we could
     be assured of reading CR2 before it changed). */
  intr_enable ();
	/*KHJ code*/
	bool flag=true;
  /* Count page faults. */
  page_fault_cnt++;
	
  /* Determine cause. */
  not_present = (f->error_code & PF_P) == 0;
  write = (f->error_code & PF_W) != 0;
  user = (f->error_code & PF_U) != 0;
		
	//Bad Access
	if(!not_present || !fault_addr || is_kernel_vaddr(fault_addr)){
		exit(-1);
	}
	//Valid Reference
	else if(page_lookup(&thread_current()->Page_Table,fault_addr)!=NULL){
		flag=handle_mm_fault(fault_addr);		 		
	}
	//stack can grow, if it is bigger than stack pointer
	else if(fault_addr >= f->esp -32){
		stack_grow(fault_addr);
	}
	//Valid but not thread's address
	else if(!write || !user){
		exit(-1);
	}
	
	//if handle_mm_fault have failed
	if(!flag){
		//if fault_addr is not mapped
		if(pagedir_get_page(thread_current()->pagedir,(const void *)fault_addr)==NULL)
			exit(-1);
  
		printf ("Page fault at %p: %s error %s page in %s context.\n",
		fault_addr,
		not_present ? "not present" : "rights violation",
		write ? "writing" : "reading",
		user ? "user" : "kernel");
 	 	kill (f);
	}
}

