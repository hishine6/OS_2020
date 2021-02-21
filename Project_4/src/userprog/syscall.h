#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "lib/user/syscall.h"

void syscall_init (void);
void check_valid_addr(const void *ptr_to_check);

//project 1
void syscall_init (void);
void halt (void);
void exit (int status);
pid_t exec (const char *cmd);
int wait (pid_t pid);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);

int fibonacci(int n);
int max_of_four_int(int a,int b,int c, int d);


//project 2
bool create (const char *file,unsigned initial_size);
bool remove (const char *file);
int open(const char *file);
void close(int fd);
int filesize(int fd);
void seek(int fd, unsigned position);
unsigned tell(int fd);



#endif /* userprog/syscall.h */
