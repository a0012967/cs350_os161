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
#include <file_syscall.h>
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
write(int fd, const void *buf, size_t nbytes,int *retval){
    
    if(fd < 0|| fd >= MAX_FILE_OPEN /*|| curthread->t_process == NULL || curthread->t_process->table == NULL/*||(curthread->t_process->table->fds[fd]->flag != O_WRONLY && curthread->t_process->table->fds[fd]->flag != O_RDWR )*/){
        //kprintf("WRITE HERE\n");
        /* kprintf("write, fd is %d, size is %d\n", fd,(int)nbytes);
        kprintf("maxfile open cond: %d\n",fd >= MAX_FILE_OPEN);
        int bool1 = curthread->t_process->table->fds[fd]== NULL;
        kprintf("1st bool: %d\n",bool1);
        int bool2 = curthread->t_process->table->fds[fd]->flag != O_WRONLY;
        kprintf("2nd bool: %d, flag: %d\n",bool2,curthread->t_process->table->fds[fd]->flag);
        int bool3 = curthread->t_process->table->fds[fd]->flag != O_RDWR;
        kprintf("3rd bool: %d\n",bool3);
        */ 
        return EBADF;
        
       
        
    }
    if(!buf){
        
        return EFAULT;
    }
    
    init();
    
   
    
    struct uio u_write; //used to hold data to write
    int offset = curthread->t_process->table->fds[fd]->offset;
    mk_kuio(&u_write,buf,nbytes,offset,UIO_WRITE);
    struct vnode *vn = curthread->t_process->table->fds[fd]->vn;
    lock_acquire(syslock);
   
    //int result = vfs_open(console,O_WRONLY,&vn);
    
    int result2 = VOP_WRITE(vn,&u_write);
  
        lock_release(syslock);    
 
    if(result2){
        
        return result2; //should also return EIO and ENOSPC
    }
    
    curthread->t_process->table->fds[fd]->offset =  u_write.uio_resid;

 
    *retval = nbytes - u_write.uio_resid;
   return 0; //returns how many were written
    
}





int read(int fd, void *buf, size_t nbytes, int *err){
    //curthread->t_process->table[fd].offset
    
    vaddr_t bot,top;
    bot = (vaddr_t) buf;
    top = bot + nbytes -1;
    
    if(fd < 0 || fd >= MAX_FILE_OPEN || curthread->t_process->table->fds[fd]== NULL /*||( curthread->t_process->table->fds[fd]->flag != O_RDONLY && curthread->t_process->table->fds[fd]->flag != O_RDWR )*/){

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
  //  kprintf("preparing to read\n");
    int result2 = VOP_READ(vn,&u_read);
    //kprintf("read succesfful\n");
    lock_release(syslock);    
 
    
    if(result2){
        *err = result2; //should also return EIO and ENOSPC
        return -1; 
    }
    
    //kprintf("return val is %d",nbytes - u_read.uio_resid);
    curthread->t_process->table->fds[fd]->offset = u_read.uio_resid;
    *err = 0; //success
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

//checks to see if a process has exited
int process_exited(struct process* proc){
	
	return proc->exit;

}

void _exit(int exitcode, int * retval)
{  
    // exiting process here doesn't work, exit immediately
    thread_exit();
    
    /*
    assert(curthread != NULL);
    
    //check to see if parent is NULL or exited
    if(curthread->t_process == NULL || curthread->t_process->parent == NULL || process_exited(curthread->t_process->parent)){
    	//dont care who we report too
    	thread_exit();
    
    }//if
    
    //free the fd stuff
    int i;
    lock_acquire(syslock);
         
    exit_process(curthread->t_process->PID,exitcode);
    assert(curthread->t_process->exit_code == exitcode);
    
    lock_release(syslock);
        
    splhigh();
    thread_exit();
     */
}

/*
 * Calculates the padding length needed for a particular
 * string. Total length should be a multiple of 4
 */
static unsigned int 
calc_align_length(char *argv)
{
    unsigned int argvlen = strlen(argv)+1; // length of actual string including '\0'
    unsigned int paddinglen = 4-(argvlen%4);    // mod 4
    
    return argvlen+paddinglen; // total correct alignment length(multiple of 4)
}

int execv(const char *progname, char **argv, int *retval)
{
    	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;
        int argc = 0;
        
        while (argv[argc] != NULL)
        {
            argc++;
        }
        
        argc++;

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, &v);
	if (result) {
		return result;
	}

	/* We should be a new thread. */
	assert(curthread->t_vmspace == NULL);

	/* Create a new address space. */
	curthread->t_vmspace = as_create();
	if (curthread->t_vmspace==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Activate it. */
	as_activate(curthread->t_vmspace);

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		return result;
	}

    
    
    //Initialize the Process & put it onto proctable
    lock_acquire(proc_lock);
    struct process *new_processs = add_process_new();
    if (new_processs == NULL) {
        return EAGAIN; // error occured
    } else {
        curthread->t_process = new_processs;
    }
    
    //Initialize fd table
    result = fd_table_create();
    if (result) {
        return result; // Error occured
    }
    
    lock_release(proc_lock);
    
    //Argument passing
        vaddr_t initialptr = stackptr;
        unsigned int i;
        unsigned int j; // i reversed index
        int err;
        
        unsigned int alignlen; // length of argument with correct mod4 alignment
        
        if (argc > 1)
        {
            for (i = 0; i < argc; i++) // copyout the array values
            {
                j = argc-(i+1);

                alignlen = calc_align_length(argv[j]);

                stackptr = stackptr-alignlen;

                if ((err = copyoutstr(argv[j], stackptr, strlen(argv[j])+1, &alignlen)) != 0)
                    kprintf("ERROR copyoutstr %d\n", err);

                argv[j] = stackptr; // fill argv with actual user space ptr
            }
            
            for (i = 0; i <= argc; i++) // copyout the array addresses
            {
                j = argc-i;
                stackptr = stackptr-4;

                if ((err = copyout(&argv[j], stackptr, 4)) != 0)
                    kprintf("ERROR copyout %d\n", err);

            }

            /* Warp to user mode. */
            md_usermode(argc /*argc*/, stackptr /*userspace addr of argv*/,
                        stackptr, entrypoint);
        }
        else
        {
            /* Warp to user mode. */
            md_usermode(0 /*argc*/, NULL /*userspace addr of argv*/,
                        stackptr, entrypoint);
        }
	
	/* md_usermode does not return */
	panic("md_usermode returned\n");

    
	md_usermode(0 /*argc*/, NULL /*userspace addr of argv*/,stackptr, entrypoint);
	
	/* md_usermode does not return */

    panic("md_usermode returned\n");

	return EINVAL;
}


#endif /* _OPT_A2_ */
