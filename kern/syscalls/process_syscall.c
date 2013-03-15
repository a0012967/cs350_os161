#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <clock.h>
#include <syscall.h>
#include <machine/trapframe.h>
#include "opt-dumbvm.h"
#include <test.h>
#include <vm.h>
#include <vfs.h>
#include <synch.h>
#include <process.h>
#include <fileDescriptor.h>
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
    
    
    
    if (options != 0) {
        *errno = EINVAL;
        return (pid_t)(-1);
    }
    
    /*if (*status == NULL) {
        *errno = EFAULT;
        return (pid_t)(-1);
    }*/
    
    struct process *kid_process = process_get(pid);
    
        lock_acquire(proc_lock);

    if (kid_process == NULL)   {
        *status = -1;
        lock_release(proc_lock);
        
        *errno=EAGAIN;
        return (pid_t)(-1);
    }
    
    if (kid_process->parent != curthread->t_process)   {
        lock_release(proc_lock);
        *errno = ENOMEM;
        return (pid_t)(-1);
    }
    
    //kprintf("*");
    
    if (kid_process->exit == 1)   {
        //kprintf(")");
        *errno = 0;
        *status = kid_process->exit_code;
        
        remove_process(kid_process->PID);//destroy the kid
        lock_release(proc_lock);
        return pid;
    } else {
        *errno = 0;
        while (kid_process->exit == 0) {
            cv_wait(kid_process->exit_cv, proc_lock);
            //kprintf("released");
        }
        *status = kid_process->exit_code;
        //kprintf(".");
        remove_process(kid_process->PID);//destroy the kid
        //kprintf("#");
        lock_release(proc_lock);
        return pid;
    }
    
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
    struct fork_setup *judge = data1;
    struct addrspace *new_addrspace;
    struct trapframe new_trapframe;
    
    //copy addrspace
    if (as_copy(judge->parent->t_vmspace, &new_addrspace))    {
        judge->child_pid = ENOMEM; // no memory for child process
        V(judge->child_sem);
        thread_exit();
    }
    
    curthread->t_vmspace = new_addrspace; //curthread = child thread
    as_activate(new_addrspace);
    
    //copy trapframe
    memcpy(&new_trapframe, judge->tf, sizeof(struct trapframe));
    new_trapframe.tf_v0 =0;
    new_trapframe.tf_a3 = 0;
    new_trapframe.tf_epc += 4;
    
    //creat process for child
    struct process *child_process = add_process_child(curthread->t_process);
    if (child_process == NULL)  {
        judge->child_pid = EAGAIN; // no space in proctable
        V(judge->child_sem);
        thread_exit();
    }
    judge->child_pid = child_process->PID;
    child_process->parent = judge->parent->t_process;
    curthread->t_process = child_process;
    
    // set up new file table
    int result = fd_table_create();
    if (result) {
        judge->child_pid = result;
        result = remove_process(child_process->PID);
        V(judge->child_sem);
        thread_exit();
    }
    
    //copy file table
    lock_acquire(judge->parent->t_process->fd_lock);
    int i;
    for (i = 0; i < MAX_FILE_OPEN; i++)  {
        child_process->table->fds[i] = judge->parent->t_process->table->fds[i];
        
        if (child_process->table->fds[i] != NULL)   { //increase ref count for files in table
            child_process->table->fds[i]->ref_count++;
        }
    }//for 
    lock_release(judge->parent->t_process->fd_lock);
    
    //child is done! Let parent go ahead
    V(judge->child_sem);
    mips_usermode(&new_trapframe);
    
    //mips_usermode should return. It should never pass this stage.
    panic("\nsys_fork->md_forkentry\nmips_usermode did not return\n");
} 



    
int sys_fork(struct trapframe *tf, int * retval)    {
    
    //set up fork_setup struct to keep track of fork progress
    //kprintf(".");
    struct fork_setup judge;
    judge.child_sem = sem_create("fork sem", 0); // don't let parent go until child V this
    if (judge.child_sem == NULL)  {
        *retval = -1;
        return ENOMEM;
    }
    judge.tf = tf;
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
    if (judge.child_pid <= 0)   {
        *retval = -1;
        return ENOMEM;
    } else  {
        //*retval = 0;
        return 0;
    }
    
    
}
     
#endif /* _OPT_A2_ */
