/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <test.h>
#include "opt-A2.h"

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

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
runprogram(char *progname, char** argv, int argc)
{
struct vnode *v;
vaddr_t entrypoint, stackptr;
int result;

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
curthread->t_vmspace->progname = progname;
/* Load the executable. */
result = load_elf(v, &entrypoint);
if (result) {
/* thread_exit destroys curthread->t_vmspace */
vfs_close(v);
return result;
}

/* Done with the file now. */
//vfs_close(v);

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
    
/* Warp to user mode. */
md_usermode(0 /*argc*/, NULL /*userspace addr of argv*/,
stackptr, entrypoint);

/* md_usermode does not return */
panic("md_usermode returned\n");
return EINVAL;
}