#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <clock.h>
#include <syscall.h>
#include <machine/trapframe.h>
#include <addrspace.h>
#include <test.h>
#include <vm.h>
#include <vfs.h>
#include <synch.h>
#include <process.h>
#include "opt-A2.h"

/*
 //plese see process.c
void proctable_init(struct process *t,int len){
    
    int i;
    for(i = 1;i<len;i++){
        t[i] = NULL;
}
*/
#if OPT_A2
pid_t getpid()
{
    int i;
    for(i = 0; i<MAX_PROCESS; i++){
        
        /** each process linked to a thread **
        if(proctable[i]->PID == curthread) {
            
            return proctable[i]->PID;
        }*/
        
        /** each thread linked to a process **/
        int cur_pid = (int)curthread->t_process->PID;
        
        if (proctable[cur_pid] != NULL)
            return cur_pid;
        
    }
    
    return (pid_t)(-1);

}

pid_t waitpid(pid_t pid, int *status, int options, int *errno)
{
    struct process *p = process_get(pid); // process given by pid
    
    /* Argument checking */
    if (p == NULL) // invalid process, waitpid fails
    {
        return (pid_t)(-1);
    }
    
    if (options != 0) // invalid options 
    {
        errno = EINVAL;
        return (pid_t)(-1);
    }
    
    if (status == NULL /* null ptr*/ || 
        status > USERTOP /* ptr in kernel area */ || 
        (unsigned int)status%4 != 0 /* not properly aligned */ ) // invalid pointer
    {
        errno = EFAULT;
        return (pid_t)(-1);
    }
    
    lock_acquire(p->exit_lock);
    
    if (p->exit == 1) // process has already exited
    {
        *status = p->exit_code;
    }
    else // process has not yet exited
    {
        pid_t parent_of_child_pid = p->parent->PID;
        pid_t parent_pid = curthread->t_process->PID;
        
        // can only wait on your child
        if (parent_of_child_pid == parent_pid)
        {
            while (p->exit == 1) // wait for process to exit
                cv_wait(p->exit_cv, p->exit_lock);
        }
    }
    
    lock_release(p->exit_lock);
    
    return pid;
}

#endif /* _OPT_A2_ */
