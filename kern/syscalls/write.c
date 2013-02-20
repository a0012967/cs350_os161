#include <types.h>
#include <synch.h>
#include <thread.h>
#include <lib.h>
#include <kern/errno.h>
#include "opt-A2.h"
//#include <unistd.h>
#include <uio.h>
#include <vfs.h>
#include <kern/unistd.h>
#include <vnode.h>
/*write writes up to buflen bytes to the file specified by fd, at the location in the file specified by the current 
//seek position of the file, taking the data from the space pointed to by buf. The file must be open for writing.*/
//Must ensure that we only allow one thread to do any of the syscalls

//Yi making a silly comment to check if git is working, please delete for next version.
volatile struct lock *syslock;

static void init(){

if(syslock ==NULL){
    syslock = lock_create("sys_lock");
    }

}

int
write(int fd, const void *buf, size_t nbytes){
    
    vaddr_t bot,top;
    bot = (vaddr_t) buf;
    top = bot + nbytes -1;
    
    if(fd < 0){
        
        return EBADF;
        
    }
    if(top < bot){
        
        return EFAULT;
    }
    //init();
    //lock_acquire(syslock);
    
    
    struct uio u_write;
    mk_kuio(&u_write,buf,nbytes,0,UIO_WRITE);
  
    struct iovec iovec_write;
    struct vnode *vn;
    char *console = NULL;
    console = kstrdup("con:");
    int result = vfs_open(console,O_WRONLY,&vn);
    
    int result2 = VOP_WRITE(vn,&u_write);
    
    kfree(console);    
    
    
    //lock_release(syslock);
    if(result2){
        
        return result2;
    }
    

    
    
    
    
    
   /* init();
    
    if(fd < 0){
    
    return 0xEBADF;
    }
    
    //All errors should be returned before we access critical section
    //lock_acquire(syslock);
    
    
    size_t i = 0;
    
    const char* buffer_chars = (const char *)buf; //cast bif which is void * to char *
    
    while(i<nbytes){
      
      putch(buffer_chars[i]);
      i++;
    
    }
    
    
    //lock_release(syslock);
    
    
    
    */
    
   return 0;
    
}
