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


/// fork stuff ///
/*
 struct fork_setup:
    stores information about the parent thread, to be passed on to child.
    Mainly used because it keeps tracks when to let the parent continue on(sem)


 md_forkentry:
    stolen from syscall.c moved it here because it fits in with the fork family.
    does all the work in thread_fork.
 sys_fork:
 calls thread_fork to fork a new thread

*/

struct fork_setup   {
    struct semaphore *child_sem;
    struct thread *parent; //parent, for child to see
    pid_t child_pid; //child's pid, for parent , -1= no more space avail on proctable, 0 = child out of memory
    struct trapframe *tf; //parent's trapframe
};

// YI: NEED TO CHANGE HEADER FILE FOR THIS FUNCTION!!!! 

void
md_forkentry(void *data1, unsigned long data2) { // data1 = fork_setup monitor
    (void)data2;
    struct addrspace *new_addrspace;
    struct trapframe new_trapframe;
    
    //copy addrspace
    if (as_copy(data1->parent->t_vmsapce, &new_addrspace))    {
        data1->child_pid = ENOMEM; // no memory for child process
        V(data1->child_sem);
        thread_exit();
    }
    
    curthread->t_vmspace = new_addrspace; //curthread = child thread
    as_activate(new_addrspace);
        
    //copy trapframe
    memcpy(&new_trapframe, data1->tf, sizeof(struct trapframe));
    new_trapframe.tf_v0 =0;
    new_trapframe.tf_a3 = 0;
    new_tf-tf_epc += 4;
        
    //creat process for child
    struct process *child_process = add_process_child(data1->parent);
    if (child_process == NULL)  {
        data1->child_pid = EAGAIN; // no space in proctable
        V(data1->child_sem);
        thread_exit();
    }
    data1->child_pid = child_process->PID;
    child_process->parent = data1->parent->t_process;
    curthread->t_process = child_process;
        
    // set up new file table
    int result = fd_table_create();
    if (result) {
        data1->child_pid = retult;
        result = remove_process(child_process->PID);
        V(data1->sem);
        thread_exit();
    }
    
    //copy file table
    lock_acquire(data1->parent->t_process->fd_lock);
    for (int i = 0; i < MAX_OPEN; i++)  {
        child_process->table->fds[i] = data1->parent->t_process->table->fds[i];
        
        if (child_process->table->fds[i] != NULL)   { //increase ref count for files in table
            child_process->table->fds[i]->ref_count++;
        }
    }//for 
    lock_release(data1->parent->t_process->fd_lock);
    
    //child is done! Let parent go ahead
    V(data1->child_sem);
    mips_usermode(&new_trapframe);
    
    //mips_usermode should return. It should never pass this stage.
    panic("\nsys_fork->md_forkentry\nmips_usermode did not return\n");
} 
    


    
int sys_fork(struct trapframe *tf, int * retval)    {
    
    //set up fork_setup struct to keep track of fork progress
    
    struct fork_setup judge;
    judge.child_sem = sem_create("fork sem", 0); // don't let parent go until child V this
    if (judge.sem == NULL)  {
        *retval = -1;
        return ENOMEM;
    }
    judge.parent = curthread;
    
    //use thread_fork, func=md_forkentry
    int result = thread_fork("forking child", &judge, 0, &md_forkentry, NULL);
    
    if (result) {
        sem_destroy(judge.child_sem);
        *retval = -1;
        return ENOMEM;
    }
    
    //wait for child to be ready
    P(judge.child_sem);
    *retval = judge.child_pid;
    
    //check for errors (see fork_setup struct) 
    if (judge.child_pid == 0)   {
        *retval = -1;
        return ENOMEM;
    } else if (judge.child_pid < 0) {
        *retval = -1;
        return EAGAIN;
    } else  {
        return 0;
    }
    
    
}
     
#endif /* _OPT_A2_ */
