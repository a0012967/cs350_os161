#ifndef _process_H
#define _process_H
#include <synch.h>

#include <types.h>
#include "opt-A2.h"


#if OPT_A2

#define MIN_PROCESS 1
#define MAX_PROCESS 100


struct process {
    
    pid_t PID;
    
    int exit_code;
    int exit; //if exited,1 else 0
    
    // locks and cvs for waitpid
    struct cv* exit_cv;
    struct lock* exit_lock; // lock for checking whether process exited or not
    
    struct fd_table* table; 
    struct cv* proc_cv;
    struct lock* fd_lock; // Yi: I need a lock for fd_table :) Moved old 
                            // proc_lock outside of the struct, see below.
    struct process* parent;
};
 

/*
 Yi:
 changed proctable to an array of pointers to process. This makes checking parents easier. Also makes more sense when we're looking at proctable from a thread's point of view. Please change it if you don't want points.
 */
struct process **proctable;
 



/*
 Yi: Instead of having a proc_lock in each process lock, we should have one that guards all the processes. A lock for each process is not very useful, we need something to manage all processes.
 */
struct lock *proc_lock;



//Initialize proctable, do this in main (during boot?)
void proctable_init();

// Add process to proctable. process is brand new (no parent)
struct process *add_process_new();

// Add process to proctable. process has a parent (fork?)
struct process *add_process_child(struct process* parent);

// Remove process
int remove_process(pid_t pid);

// process's exit = 1.
//  This doesn't remove the process, because other process need its exit code!!!!!
int exit_process(pid_t pid, int exitcode);

//retrieve process by its pid
struct process* process_get(pid_t pid);

/* Yi:
    we should add these in a sepereate file called 
    pro_syscall.c to stay consistent with file_syscall.c 
    Let me know what you think. We can revert it
 
pid_t getpid();
pid_t waitpid(pid_t pid, int* status, int options);
pid_t fork();
*/

#endif /* _OPT_A2 */

#endif //define
