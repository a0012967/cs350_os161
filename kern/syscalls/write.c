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
#include <curthread.h>
/*write writes up to buflen bytes to the file specified by fd, at the location in the file specified by the current 
//seek position of the file, taking the data from the space pointed to by buf. The file must be open for writing.*/
//Must ensure that we only allow one thread to do any of the syscalls
<<<<<<< Updated upstream

//Yi making a silly comment to check if git is working, please delete for next version.
volatile struct lock *syslock;
=======
volatile struct lock *syslock = NULL;
>>>>>>> Stashed changes

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
    if(top < bot || !buf){
        
        return EFAULT;
    }
    
    init();
    
   
    
    struct uio u_write; //used to hold data to write
    mk_kuio(&u_write,buf,nbytes,bot,UIO_WRITE);
  
    struct iovec iovec_write;
    struct vnode *vn;
    char *console = NULL;
    console = kstrdup("con:");
    //kprintf("lock about to acquire\n");
    lock_acquire(syslock);
   // kprintf("lock acquire success\n");
    int result = vfs_open(console,O_WRONLY,&vn);
    
    int result2 = VOP_WRITE(vn,&u_write);
   //vfs_close(vn);
        lock_release(syslock);    
       // kprintf("released lock\n");
    kfree(console);    
    
    
  // kprintf("result2 is %d\n",result2);
    if(result2){
        
        return result2; //should also return EIO and ENOSPC
    }



    
    
 
    
   return nbytes - u_write.uio_resid; //returns how many were written
    
}





int read(int filehandle, void *buf, size_t size){


    vaddr_t bot,top;
    bot = (vaddr_t) buf;
    top = bot + size -1;
    
    if(filehandle < 0){
        
        return EBADF;
        
    }
    if(top < bot){
        
        return EFAULT;
    }



   init();
   
    struct uio u_read;
    mk_kuio(&u_read,buf,size,0,UIO_READ);//sets up for read
  
    //struct iovec iovec_read;
    struct vnode *vn;
    char *console = NULL;
    console = kstrdup("con:");
    //kprintf("lock about to acquire\n");
  //  lock_acquire(syslock);
   // kprintf("lock acquire success\n");
   kprintf("read1\n");
    int result = vfs_open(console,O_RDONLY,&vn);
    kprintf("read1end\n");
    int result2 = VOP_READ(vn,&u_read);
    kprintf("read2\n");
    //    lock_release(syslock);    
       // kprintf("released lock\n");
    kfree(console);    
    
    

    if(result2){
        kprintf("ifststament\n");
        return result2; //should also return EIO and ENOSPC
    }

kprintf("result2 cond failed, result2:%d \n",result2);

//kprintf("return: %d\n",size - u_read.uio_resid);

   return size - u_read.uio_resid; //returns how many were written
  //return 0;
}
