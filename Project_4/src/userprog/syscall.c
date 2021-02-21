#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "devices/input.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/synch.h"
#include "vm/frame.h"
static void syscall_handler (struct intr_frame *);
void get_stack_arguments(struct intr_frame *f,int *args, int n);
void check_buffer(void *buffer_check,unsigned size);
void check_valid_addr(const void *ptr_to_check);

struct semaphore wrt;
struct lock mutex;
int readcount;

void syscall_init (void) {
	lock_init(&mutex);
	sema_init(&wrt,1);
	readcount=0;
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void syscall_handler (struct intr_frame *f) {
	int args[5];
	check_valid_addr((const void*)(f->esp));
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
			f->eax=exec((const char*)args[0]);
      break;

    case SYS_WAIT:
      get_stack_arguments(f,&args[0],1);
			f->eax=wait((pid_t)args[0]);
			break;
    
		case SYS_READ:
			get_stack_arguments(f,&args[0],3);
			check_buffer((void*)args[1],args[2]);
			f->eax=read(args[0],(void*)args[1],(unsigned)args[2]);	
			//set_buffer_back((void*)args[1],args[2]);
			break;
    
		case SYS_WRITE:
			get_stack_arguments(f,&args[0],3);
			check_buffer((void*)args[1],args[2]);
			f->eax = write(args[0],(const void*)args[1],(unsigned)args[2]);
			//set_buffer_back((void*)args[1],args[2]);
			break;
		
		case SYS_FIB:
			get_stack_arguments(f,&args[0],1);
      f->eax=fibonacci(args[0]);
			break;

		case SYS_MAX_OF_FOUR:
			get_stack_arguments(f,&args[0],4);
      f->eax=max_of_four_int(args[0],args[1],args[2],args[3]);
			break;
		
		case SYS_CREATE:
			get_stack_arguments(f,&args[0],2);
			f->eax = create((const char *)args[0],(unsigned) args[1]);
			break;

		case SYS_REMOVE:
			get_stack_arguments(f,&args[0],1);
			f->eax=remove((const char *) args[0]);
			break;

		case SYS_OPEN:
			get_stack_arguments(f,&args[0],1);
			f->eax=open((const char*)args[0]);
			break;


		case SYS_CLOSE:
			get_stack_arguments(f,&args[0],1);
			close(args[0]);
			break;

		case SYS_FILESIZE:
			get_stack_arguments(f,&args[0],1);
			f->eax=filesize(args[0]);
			break;

		case SYS_SEEK:
			get_stack_arguments(f,&args[0],2);
			seek(args[0],(unsigned)args[1]);
			break;


		case SYS_TELL:
			get_stack_arguments(f,&args[0],1);
			f->eax=tell(args[0]);
			break;
		}
}


void check_valid_addr(const void *ptr_to_check){
	if(ptr_to_check ==NULL || !is_user_vaddr(ptr_to_check) || ptr_to_check < (void*)0x08048000)
		exit(-1);
}

void get_stack_arguments(struct intr_frame *f, int *args,int n){
	for(int i=0;i<n;i++){
		uint32_t *ptr=(uint32_t*)f->esp+i+1;
		check_valid_addr((const void*)ptr);
		args[i]=*ptr;
	}
}

void check_buffer(void *buffer_check,unsigned size){
	char *ptr=(char*)buffer_check;
	for(unsigned i=0;i< size;i++){
		check_valid_addr((const void*)(ptr++));
		//set_pin(pg_round_down(ptr),true);
	}
}
void set_buffer_back(void *buffer_check,unsigned size){
 char *ptr=(char*)buffer_check;
   for(unsigned i=0;i< size;i++){
     set_pin(pg_round_down(ptr),false);
   }
	
}

void halt(void){
	shutdown_power_off();
}
void exit (int status) {
	printf("%s: exit(%d)\n",thread_name(),status);
	thread_current()->exit_status=status;
	for(int i=3;i<128;i++)
		if(thread_current()->fd[i]!=NULL)
			close(i);

	thread_exit ();
}

int read(int fd,void * buffer, unsigned size){
	uint8_t* read_buffer = (uint8_t*) buffer;
	int i=-1;
	int flag=0;
	lock_acquire(&mutex);
	readcount++;
	if(readcount==1)
		sema_down(&wrt);
	lock_release(&mutex);

	if(fd==0){
		for(i=0;i<(int)size;i++){
			read_buffer[i]=input_getc();
			if(read_buffer[i]=='\0')
				break;
		}
	}
	else if(fd>2){
		if(thread_current()->fd[fd]==NULL){
			flag=1;
		}
		else{
			i = file_read(thread_current()->fd[fd],buffer,size);
		}
	}
	
	lock_acquire(&mutex);
	readcount--;
	if(readcount==0)
		sema_up(&wrt);
	lock_release(&mutex);
	if(flag==1)
		exit(-1);

	return i;

}


int write (int fd, const void *buffer, unsigned size) {
	int re=-1;
	sema_down(&wrt);
	if (fd == 1) {
    putbuf(buffer, size);
    re=size;
  }
	else if(fd>2){
		if(thread_current()->fd[fd]==NULL){
			sema_up(&wrt);
			exit(-1);
		}
		re = file_write(thread_current()->fd[fd],buffer,size);
	}	
	sema_up(&wrt);
	return re;
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

bool create (const char *file,unsigned initial_size){
	if(file==NULL)
		exit(-1);
	bool temp = filesys_create(file,initial_size);
	return temp;
}


bool remove (const char *file){
	if(file==NULL)
		exit(-1);
	bool temp = filesys_remove(file);
	return temp;
}

int open(const char *file){
	int re=-1;
	if(file==NULL)
		exit(-1);

	struct file*f=filesys_open(file);
	if(f==NULL){
		re = -1;
	}
	else{
		for(int i=3;i<128;i++){
			if(thread_current()->fd[i]==NULL){
				if(strcmp(thread_name(),file)==0)
					file_deny_write(f);
				thread_current()->fd[i]=f;
				re= i;
				break;
			}
		}
	}

	return re;
}

void close(int fd){
	if(fd<0 || thread_current()->fd[fd]==NULL)
		exit(-1);
	file_close(thread_current()->fd[fd]);
	thread_current()->fd[fd]=NULL;
}

int filesize(int fd){
	if(thread_current()->fd[fd]==NULL)
     exit(-1);
	return file_length(thread_current()->fd[fd]);
}

void seek(int fd, unsigned position){
	if(thread_current()->fd[fd]==NULL)
     exit(-1);
	file_seek(thread_current()->fd[fd],position);
}

unsigned tell(int fd){
	if(thread_current()->fd[fd]==NULL)
     exit(-1);
	return file_tell(thread_current()->fd[fd]); 
}

