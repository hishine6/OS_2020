#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "devices/input.h"

static void syscall_handler (struct intr_frame *);
void get_stack_arguments(struct intr_frame *f,int *args, int n);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
	int args[5];
	void *phys_page_ptr;

	switch (*(uint32_t*)(f->esp)) {
    case SYS_HALT:
     	halt();
		 	break;

    case SYS_EXIT:
			get_stack_arguments(f,&args[0],1);
			exit(args[0]);
      break;
    
		case SYS_EXEC:
			get_stack_arguments(f,&args[0],1);
			phys_page_ptr=pagedir_get_page(thread_current()->pagedir,(const void*)args[0]);
      if(phys_page_ptr==NULL)
      	exit(-1);
	    args[0] = (int)phys_page_ptr;
			f->eax=exec((const char*)args[0]);
      break;

    case SYS_WAIT:
      get_stack_arguments(f,&args[0],1);
			f->eax=wait((pid_t)args[0]);
			break;
    
		case SYS_READ:
			get_stack_arguments(f,&args[0],3);
			phys_page_ptr=pagedir_get_page(thread_current()->pagedir,(const void*)args[1]);
      if(phys_page_ptr==NULL)
	      exit(-1);
      args[1] = (int)phys_page_ptr;
			f->eax=read(args[0],(void*)args[1],(unsigned)args[2]);	
      break;
    
		case SYS_WRITE:
			get_stack_arguments(f,&args[0],3);
			phys_page_ptr=pagedir_get_page(thread_current()->pagedir,(const void*)args[1]);
			if(phys_page_ptr==NULL)
				exit(-1);
			args[1] = (int)phys_page_ptr;
			f->eax = write(args[0],(const void*)args[1],(unsigned)args[2]);
			break;
		
		case SYS_FIB:
			get_stack_arguments(f,&args[0],1);
      f->eax=fibonacci(args[0]);
			break;

		case SYS_MAX_OF_FOUR:
			get_stack_arguments(f,&args[0],4);
      f->eax=max_of_four_int(args[0],args[1],args[2],args[3]);
			break;
		}
}


void check_valid_addr(const void *ptr_to_check){
	if(!is_user_vaddr(ptr_to_check) || ptr_to_check==NULL || ptr_to_check < (void*)0x08048000)
		exit(-1);
}

void get_stack_arguments(struct intr_frame *f, int *args,int n){
	for(int i=0;i<n;i++){
		uint32_t *ptr=(uint32_t*)f->esp+i+1;
		check_valid_addr((const void*)ptr);
		args[i]=*ptr;
	}
}
void halt(void){
	shutdown_power_off();
}
void exit (int status) {
  printf("%s: exit(%d)\n",thread_name(),status);
	thread_current()->exit_status=status;
	thread_exit ();
}

int read(int fd,void * buffer, unsigned size){
	uint8_t* read_buffer = (uint8_t*) buffer;

	int i=-1;
	if(fd==0){
		for(i=0;i<(int)size;i++){
			read_buffer[i]=input_getc();
			if(read_buffer[i]=='\0')
				break;
		}
	}
	return i;
}


int write (int fd, const void *buffer, unsigned size) {
  if (fd == 1) {
    putbuf(buffer, size);
    return size;
  }
  return -1;
}

pid_t exec (const char *cmd){
	if(!cmd)
		return -1;
	
	pid_t child_tid=(pid_t)process_execute(cmd);

	return child_tid;
}

int wait(pid_t pid){
	return process_wait(pid);
}

int fibonacci(int n){
	int a=0;
	int b = 1;
	if(n==0)
		return 0;
	else if(n==1)
		return 1;
	else{
		for(int i=1;i<n;i++){
			int temp = b + a;
			a=b;
			b=temp;
		}
	}
	return b;
}

int max_of_four_int(int a,int b,int c, int d){
	int first_max = a>b ? a:b;
	int second_max = c>d ? c:d;
	return first_max > second_max ? first_max : second_max;
}



