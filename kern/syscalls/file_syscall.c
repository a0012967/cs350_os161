#include <types.h>
#include <synch.h>
#include <syscall.h>
#include <thread.h>
#include <lib.h>
#include <kern/errno.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <uio.h>
#include <vfs.h>
#include <kern/unistd.h>
//#include <vnode.h>
#include <curthread.h>
#include "opt-A2.h"
/*write writes up to buflen bytes to the file specified by fd, at the location in the file specified by the current 
//seek position of the file, taking the data from the space pointed to by buf. The file must be open for writing.*/
//Must ensure that we only allow one thread to do any of the syscalls

#if OPT_A2

/*
 Yi:
 
 Carl, I put in a fd table lock called fd_lock in process.h, maybe we can coordinate that lock for all the file syscalls?
 */
volatile struct lock *syslock = NULL;
volatile int offset = 0;


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
    
    if(fd < 0 || !buf){
        
        return EBADF;
        
    }
    if(top < bot || !buf){
        
        return EFAULT;
    }
    
    init();
    
   
    
    struct uio u_write; //used to hold data to write
    mk_kuio(&u_write,buf,nbytes,0,UIO_WRITE);
  
    struct iovec iovec_write;
    struct vnode *vn;
    char *console = NULL;
    console = kstrdup("con:");

    lock_acquire(syslock);
   
    int result = vfs_open(console,O_WRONLY,&vn);
    
    int result2 = VOP_WRITE(vn,&u_write);
   // kprintf("write result2 is: %d\n",result2);
        lock_release(syslock);    
       
    kfree(console);    
 
    if(result2){
        
        return -1; //should also return EIO and ENOSPC
    }



 
    
   return nbytes - u_write.uio_resid; //returns how many were written
    
}





int read(int fd, void *buf, size_t nbytes){
    
    
    vaddr_t bot,top;
    bot = (vaddr_t) buf;
    top = bot + nbytes -1;
    
    if(fd < 0 || !buf){
        
        return EBADF;
        
    }
    if(top < bot || !buf){
        
        return EFAULT;
    }
    
    init();
    
    
    
    struct uio u_read; //used to hold data to write
    mk_kuio(&u_read,buf,nbytes,offset,UIO_READ);
    
    struct iovec iovec_write;
    struct vnode *vn;
    char *console = NULL;
    console = kstrdup("con:");
    
    lock_acquire(syslock);
    
    int result = vfs_open(console,O_RDONLY,&vn);
    
    int result2 = VOP_READ(vn,&u_read);
    
    lock_release(syslock);    
    
    kfree(console);    
    
    if(result2){
        
        return result2; //should also return EIO and ENOSPC
    }
    
    return nbytes - u_read.uio_resid; //returns how many were written
}





// Opens a file using fd table
int sys_open(userptr_t filename, int flags, int mode, int *retval) {
    
    int result;
    char copy_filename[PATH_MAX];
    
    //copy filename from user to kernel
    result = copyinstr(filename, copy_filename, sizeof(copy_filename), NULL);
    
    if (result) {
        return Result;
    }
    
    //opens via fd table
    return fdtable_open(fname, flags, retval);
    
    (void) mode; //supress mode in A2
}


int sys_close(int fd) {
    
    //closes through fd_table
    return fd_table_close(fd);
}



#endif /* _OPT_A2_ */
