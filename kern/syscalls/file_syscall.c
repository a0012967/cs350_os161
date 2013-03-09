 #include <types.h>
#include <synch.h>
#include <thread.h>
#include <lib.h>
#include <kern/errno.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <uio.h>
#include <vfs.h>
#include <kern/unistd.h>
#include <vnode.h>
#include <curthread.h>
#include <syscall.h>
#include <process.h>
#include "opt-A2.h"
#include <fileDescriptor.h>
/*write writes up to buflen bytes to the file specified by fd, at the location in the file specified by the current 
//seek position of the file, taking the data from the space pointed to by buf. The file must be open for writing.*/
//Must ensure that we only allow one thread to do any of the syscalls

#if OPT_A2

/*
 Yi:
 
 Carl, I put in a fd table lock called fd_lock in process.h, maybe we can coordinate that lock for all the file syscalls?
 */
volatile struct lock *syslock = NULL;
//volatile int offset = 0;


static void init(){

if(syslock ==NULL){
    syslock = lock_create("sys_lock");
    }

}

int
write(int fd, const void *buf, size_t nbytes){
    /*
    vaddr_t bot,top;
    bot = (vaddr_t) buf;
    top = bot + nbytes -1;
    */
    if(fd < 0|| fd >= MAX_FILE_OPEN || curthread->t_process->table->fds[fd]== NULL ||(curthread->t_process->table->fds[fd]->flag != O_WRONLY && curthread->t_process->table->fds[fd]->flag != O_RDWR )){
   /*     kprintf("fd is %d, size is %d\n", fd,(int)nbytes);
        kprintf("maxfile open cond: %d\n",fd >= MAX_FILE_OPEN);
        int bool1 = curthread->t_process->table->fds[fd]== NULL;
        kprintf("1st bool: %d\n",bool1);
        int bool2 = curthread->t_process->table->fds[fd]->flag != O_WRONLY;
        kprintf("2nd bool: %d, flag: %d\n",bool2,curthread->t_process->table->fds[fd]->flag);
        int bool3 = curthread->t_process->table->fds[fd]->flag != O_RDWR;
        kprintf("3rd bool: %d\n",bool3);*/
        return EBADF;
        
       
        
    }
    if(/*top < bot ||*/ !buf){
        
        return EFAULT;
    }
    
    init();
    
   
    
    struct uio u_write; //used to hold data to write
    int offset = curthread->t_process->table->fds[fd]->offset;
    
    //kprintf("offset is %d\n",offset);
    //offset =0;
    //char *buffer = (char *)kmalloc(nbytes);
    
    //int copresult = copyin((userptr_t)buf,buffer,nbytes);
    //kprintf("copresult: %d, length: %d\n",copresult,nbytes);
    mk_kuio(&u_write,buf,nbytes,offset,UIO_WRITE);
    
    struct vnode *vn = curthread->t_process->table->fds[fd]->vn;
    
    
    lock_acquire(syslock);
   
    //int result = vfs_open(console,O_WRONLY,&vn);
    
    int result2 = VOP_WRITE(vn,&u_write);
   // kprintf("write result2 is: %d\n",result2);
        lock_release(syslock);    
       
   // kfree(buffer);    
 
    if(result2){
        
        return result2; //should also return EIO and ENOSPC
    }
    
    

    curthread->t_process->table->fds[fd]->offset =  u_write.uio_resid;

 
    
   return nbytes - u_write.uio_resid; //returns how many were written
    
}





int read(int fd, void *buf, size_t nbytes){
    //curthread->t_process->table[fd].offset
    
    vaddr_t bot,top;
    bot = (vaddr_t) buf;
    top = bot + nbytes -1;
    
    if(fd < 0 || fd >= MAX_FILE_OPEN || curthread->t_process->table->fds[fd]== NULL ||( curthread->t_process->table->fds[fd]->flag != O_RDONLY && curthread->t_process->table->fds[fd]->flag != O_RDWR )){
     /*   kprintf("fd is %d, size is %d\n", fd,(int)nbytes);
        kprintf("maxfile open cond: %d\n",fd >= MAX_FILE_OPEN);
        int bool1 = curthread->t_process->table->fds[fd]== NULL;
        kprintf("1st bool: %d\n",bool1);
        int bool2 = curthread->t_process->table->fds[fd]->flag != O_WRONLY;
        kprintf("2nd bool: %d, flag: %d\n",bool2,curthread->t_process->table->fds[fd]->flag);
        int bool3 = curthread->t_process->table->fds[fd]->flag != O_RDWR;
        kprintf("3rd bool: %d\n",bool3);*/
        return EBADF;
        
    }
    if(/*top < bot ||*/ !buf){
        
        return EFAULT;
    }
    
    init();

    int offset = curthread->t_process->table->fds[fd]->offset;

  
    struct uio u_read; //used to hold data to read
 
    mk_kuio(&u_read,buf,nbytes,offset,UIO_READ);
    
    struct vnode *vn = curthread->t_process->table->fds[fd]->vn;
    lock_acquire(syslock);

    int result2 = VOP_READ(vn,&u_read);

    lock_release(syslock);    
 
    
    if(result2){
        
        return result2; //should also return EIO and ENOSPC
    }
    
    
    curthread->t_process->table->fds[fd]->offset = u_read.uio_resid;
    
    return nbytes - u_read.uio_resid; //returns how many were written
}





// Opens a file using fd table
int sys_open(userptr_t filename, int flags, int mode, int * retval) {
    
    int result;
    char copy_filename[PATH_MAX];
    
    //copy filename from user to kernel
    result = copyinstr(filename, copy_filename, sizeof(copy_filename), NULL);
    
    if (result) {
        return result;
    }
    
    //opens via fd table
    return fd_table_open(filename, flags, retval);
    
    (void) mode; //supress mode in A2
}


int sys_close(int fd) {
    
    //closes through fd_table
    return fd_table_close(fd);
}

void _exit(int exitcode, int * retval)
{
    
    //exit_process(curthread->t_process->PID,exitcode);
    
  //  *retval = exitcode;
    (void)exitcode;
    kprintf("Exiting...\n");
	
    // exit process - uncomment later
    //exit_process(curthread->t_process->PID, exitcode);
    
    //vfs_clearbootfs();
    //vfs_clearcurdir();
    //vfs_unmountall();
    
    splhigh();
    
    //scheduler_shutdown();
    thread_exit();
}


#endif /* _OPT_A2_ */
