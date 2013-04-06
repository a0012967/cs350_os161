#include <types.h>
#include <synch.h>
#include <thread.h>
#include <lib.h>
#include <kern/errno.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <uio.h>
#include <vfs.h>
#include <vnode.h>
#include <kern/unistd.h>
#include <vnode.h>
#include <curthread.h>
#include <syscall.h>
#include <process.h>
#include "opt-A2.h"
#include "opt-dumbvm.h"
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
write(int fd, const void *buf, size_t nbytes,int *retval){
    //kprintf("%");
    if(fd < 0|| fd >= MAX_FILE_OPEN ||curthread->t_process->table->fds[fd]== NULL){
        
        return EBADF;
        
    }
    if(!buf || ((buf == ((void*)0x40000000)) || (buf == ((void*)0x80000000)))){
        
        return EFAULT;
    }
    
    //init();
    
    
    
    struct uio u_write; //used to hold data to write
    int offset = curthread->t_process->table->fds[fd]->offset;
    
    mk_kuio(&u_write,buf,nbytes,offset,UIO_WRITE);
    
    
    
    struct vnode *vn = curthread->t_process->table->fds[fd]->vn;
    // lock_acquire(syslock);
    
    if (curthread->t_process->table->fds[fd]->flag == O_RDONLY)
        return EBADF;
            
    int result2 = VOP_WRITE(vn,&u_write);
    
    //lock_release(syslock);
    
    if(result2){
        
        return result2; //should also return EIO and ENOSPC
    }
    
    curthread->t_process->table->fds[fd]->offset = u_write.uio_offset;
    
    
    *retval = nbytes - u_write.uio_resid;//returns how many were written
    
    return 0;
    
}





int read(int fd, void *buf, size_t nbytes, int *retval){
    
    if(fd < 0 || fd >= MAX_FILE_OPEN /*|| curthread->t_process->table->fds[fd]== NULL*/){
        
        return EBADF;
        
    }
    if(/*top < bot ||*/ !buf){
        
        return EFAULT;
    }
    
    // init();
    if (curthread->t_process->table->fds[fd] == NULL)
        return EBADF;
    
    if (curthread->t_process->table->fds[fd]->flag == O_WRONLY)
        return EBADF;
    
    int offset = curthread->t_process->table->fds[fd]->offset;
    
    
    struct uio u_read; //used to hold data to read
    
    mk_kuio(&u_read,buf,nbytes,offset,UIO_READ);
    
    
    struct vnode *vn = curthread->t_process->table->fds[fd]->vn;
    //lock_acquire(syslock);
    
    int result2 = VOP_READ(vn,&u_read);
    
    // lock_release(syslock);
    
    
    if(result2){
        //should also return EIO and ENOSPC
        return result2;
    }
    
    
    curthread->t_process->table->fds[fd]->offset = u_read.uio_offset;
    
    *retval = nbytes - u_read.uio_resid; //returns how many were written
    
    return 0;
}





// Opens a file using fd table
int sys_open(userptr_t filename, int flags, int mode, int * retval) {
    //kprintf("(");
    int result;
    char copy_filename[PATH_MAX];
    
    //copy filename from user to kernel
    result = copyinstr(filename, copy_filename, sizeof(copy_filename), NULL);
    
    if (result) {
        return result;
    }
    
    //kprintf(")");
    
    //opens via fd table
    return fd_table_open(filename, flags, retval);
    
    (void) mode; //supress mode in A2
}


int sys_close(int fd) {
    //kprintf("&");
    //closes through fd_table
    if(fd<0 || fd>= MAX_FILE_OPEN||curthread->t_process->table->fds[fd] == NULL){
        
        return EBADF;
    }
    int result = fd_table_close(fd);
    
    //kprintf("+");
    if (result != 0) {
        //uh oh
        return -1;
    } else {
        return 0;
    }
}

//checks to see if a process has exited
int process_exited(struct process* proc){
    
    return proc->exit;
    
}

void _exit(int exitcode, int * retval)
{
    
    lock_acquire(proc_lock); // get parent's lock
    
    //fd_table_destroy();
    
    struct cv *kid_cv = curthread->t_process->exit_cv;
    
    
    if (curthread->t_process->parent == NULL) {
        //I'm a root, just exit
        //exit_process(curthread->t_process->PID, exitcode);
        
    } else { // I'm a child
        
        if (exit_process(curthread->t_process->PID, exitcode) != 0) {
            *retval = -1;
            lock_release(proc_lock);
            vfs_close(curthread->t_vmspace->v);
            thread_exit();
        }
        
        cv_signal(kid_cv, proc_lock);
    
        cv_destroy(curthread->t_process->exit_cv);
        cv_destroy(curthread->t_process->proc_cv);
    }
    
    
    
    
    //*retval = 0;
    lock_release(proc_lock);
    vfs_close(curthread->t_vmspace->v);
    thread_exit();
    
    
}

/*
 * Calculates the padding length needed for a particular
 * string. Total length should be a multiple of 4
 */
static unsigned int
calc_align_length(char *argv)
{
    unsigned int argvlen = strlen(argv)+1; // length of actual string including '\0'
    unsigned int paddinglen = 4-(argvlen%4); // mod 4
    
    return argvlen+paddinglen; // total correct alignment length(multiple of 4)
}

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
execv(char *progname, char** argv_o)
{
    // Argument testing
    
    if (progname == NULL || (progname >= ((void*)0x40000000)))
        return EFAULT;
    
    if (strlen(progname) == 0)
        return EINVAL;
    
    if (argv_o == NULL || (argv_o >= ((void*)0x40000000)))
        return EFAULT;
    
    struct vnode *v;
    vaddr_t entrypoint, stackptr;
    int result;
    int argc = 0;
    int k;
    
    
    // copy into kernel heap
    while (argv_o[argc] != NULL)
    {
        //kprintf("arguments %s\n", argv_o[argc]);
        argc++;
    }
    
    char **argv = kmalloc(sizeof(char*) * (argc+1));
    
    for (k = 0; k < argc ; k++)
    {//kprintf("Looping %d\n", k);
        //kprintf("ARGUMENTS IN NEW %s\n", argv_o[k]);
        argv[k] = kmalloc(sizeof(char) * (strlen(argv_o[k])+1));
        strcpy(argv[k], argv_o[k]);
        
    }
    argv[argc] = NULL;
    /* Open the file. */
    result = vfs_open(progname, O_RDONLY, &v);
    if (result) {
        return result;
    }
    
    as_destroy(curthread->t_vmspace);
    curthread->t_vmspace = NULL;
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
    
    //kprintf("here!\n");
    //Initialize the Process & put it onto proctable
    if (curthread->t_process == NULL)
    {
        lock_acquire(proc_lock);
        //kprintf("here1!\n");
        struct process *new_processs = add_process_new();
        
        if (new_processs == NULL) {
            return EAGAIN; // error occured
        } else {
            new_processs->exit_code = curthread->t_process->exit_code;
            curthread->t_process = new_processs;
            kprintf("HERE %d\n", curthread->t_process->exit_code);
            
        }
        
        //Initialize fd table
        result = fd_table_create();
        if (result) {
            return result; // Error occured
        }
        //kprintf("here2!\n");
        lock_release(proc_lock);
    }
    
    //Argument passing
    vaddr_t initialptr = stackptr;
    unsigned int i;
    unsigned int j; // i reversed index
    int err;
    
    unsigned int alignlen; // length of argument with correct mod4 alignment
    //kprintf("argc num %s\n", argv[0]);
    //if (argc > 1)
    //{
    for (i = 0; i < argc; i++) // copyout the array values
    {
        j = argc-(i+1);
        
        alignlen = calc_align_length(argv[j]);
        
        stackptr = stackptr-alignlen;
        
        if ((err = copyoutstr(argv[j], stackptr, strlen(argv[j])+1, &alignlen)) != 0)
            kprintf("ERROR copyoutstr %d\n", err);
        //kprintf("argc num %d\n", i);
        argv[j] = stackptr; // fill argv with actual user space ptr
    }
    
    
    
    for (i = 0; i <= argc; i++) // copyout the array addresses
    {
        j = argc-i;
        stackptr = stackptr-4;
        
        if ((err = copyout(&argv[j], stackptr, 4)) != 0)
            kprintf("ERROR copyout %d\n", err);
        
    }
    //kprintf("here4!\n");
    /* Warp to user mode. */
    md_usermode(argc /*argc*/, stackptr /*userspace addr of argv*/,
                stackptr, entrypoint);
    //}
    //else
    //{
    /* Warp to user mode. */
    //md_usermode(1 /*argc*/, NULL /*userspace addr of argv*/,
    // stackptr, entrypoint);
    //}
    
    /* md_usermode does not return */
    panic("md_usermode returned\n");
    
    return EINVAL;
}

#endif /* _OPT_A2_ */
