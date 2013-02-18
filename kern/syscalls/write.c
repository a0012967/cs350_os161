#include <types.h>
#include
#include <lib.h>
//#include <errno.h>
#include "opt-A2.h"
#include <syscall.h>
/*write writes up to buflen bytes to the file specified by fd, at the location in the file specified by the current seek position of the file, taking the data from the space pointed to by buf. The file must be open for writing.*/

//Must ensure that we only allow one thread to do any of the syscalls
volatile struct lock * syslock;
syslock = lock_create("system_call_lock");
int
write(int fd, const void *buf, size_t nbytes){
    assert(syslock != NULL);
    
    lock_acquire(syslock);
    size_t i = 0;
    
    char buffer_chars = (char *)buf; //cast bif which is void * to char *
    
    while(i<nbyts){
      
      putch(buffer_chars[i]);
      i++;
    
    }
    
    
    lock_release(syslock);
    
    
    
    
    
   return 0;
    
}
