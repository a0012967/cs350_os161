/*
 This file will not compile with the OS right now, please just read it, we need to work other details out before we link it.
 
 For bookkeeping purpose, we can move _exit and execv into this file. Or even to process_syscall.c because that is a more fitting home than their current location.
 */

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
//#include "opt-A2.h"
#include <fileDescriptor.h>
#include <syscall.h>


/*write writes up to buflen bytes to the file specified by fd, at the location in the file specified by the current 
//seek position of the file, taking the data from the space pointed to by buf. The file must be open for writing.*/
//Must ensure that we only allow one thread to do any of the syscalls

#if OPT_A2

void _exit(int exitcode, int * retval)
{  
    lock_acquire(curthread->t_process->exit_lock); // This probably won't work, because the parent might be holding the lock -> deadlock
    
    
    fdtable_destroy(); // I think we should destroy this first, because vnode has ref_count value and we want to make sure that is correct ASAP (--)
    
    if (curthread->t_process->parent == NULL)   { // if I have no parent
        if (remove_process(curthread->t_process->PID) != 0) {
            //didn't remove properly, should we return some Eerror?
        
        } else { // I have a parent!
            if (exit_process(curthread->t_process->PID, exitcode))  {
                //again, should we return some error?
            }
            //signal my parent
            cv_signal(curthread->t_process->exit_cv, curthread->t_process->exit_lock);
            
        }//fi
        
        
    }
    cv_destroy(curthread->t_process->exit_cv);
    
    /*
     ESTELLE, WE NEED TO TALK ABOUT THIS PART!!!
     
     when we call _exit, we basically need to destroy the process's allocated resource. This includes our exit_lock.
     
     
     
     But the parent still need exit_lock because it's waiting for the cv_signal
     
     what if lock_destroy runs before cv_signal is complete?
     
     we need to somehow keep this exit_lock for the parent, but still destory it because the owner(the kid) is exiting.
     
     I'm just leaving this note here so we can plan this tomorrow. But I'm worried that there are issues with letting the parent sleep on the child's lock. I wonder if it's okay to just let them sleep on the child's exit_cv address for this lock, that'll still protect the waitpid...
     
     
     Also, we need to release the lock first, before we destory. But then the parent would be holding the lock, so we can't destory until parent release too. This MAY cause a problem (it might not because hopefully waitpid will return by then). 
     
     I don't know if this makes sense, I'm still kinda confused, let's talk tomorrow.
     */
    lock_release(curthread->t_process->exit_lock);
    lock_destroy(curthread->t_process->exit_lock);
    
    
    cv_destroy(curthread->t_process->proc_cv); // may have to remove this later because it's useless right now, we'll see.
    thread_exit();

}



#endif /* _OPT_A2_ */
