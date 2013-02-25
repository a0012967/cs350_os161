#include <process.h>
#include <lib.h>
#include <kern/errno.h>

#include "opt-A2.h"


#if OPT_A2

//Initialize proctable, do this in main (during boot?)
void proctable_init()   {
    proctable = kmalloc(sizeof(struct process*) * MAX_PROCESS); 
    proc_lock = lock_create("lock for process table");
    
    for (int i = MIN_PROCESS; i < MAX_PROCESS; i++) {
        proctable[i] = NULL;
    }
}

// Add process to proctable. process is brand new (no parent)
struct *add_process_new()   {
    
    pid_t my_pid = 0;
    for (int i = MIN_PROCESS; i < MAX_PROCESS; i++) {
        if (proctable[i] == NULL)    {
            my_pid = i;
            break;
        }else if (proctable[i]->exit == 1) { // process has already exited
            remove_process(proctable[i]->pid);
            my_pid = i;
            break;
        }
    }//for
    
    if (my_pid == 0)    {
        return NULL; // no place in proctable
    }
    
    //make the process
    struct process *new = kmalloc(sizeof(struct process));
    new->pid = my_pid;
    //new->exit_code=???;
    new->exit=0; // 0 = running
    new->table=NULL;
    new->proc_cv = cv_create("cv for a process");
    new->fd_lock = lock_create("file descriptor table lock");
    new->parent = NULL;
    
    proctable[my_pid] = new;
    return new;
}


// Add process to proctable. process has a parent (fork?)
// Just a copy of add_process_new, but set new->parent = exisiting parent.
struct *add_process_child(struct process* parent)   {
    
    pid_t my_pid = 0;
    for (int i = MIN_PROCESS; i < MAX_PROCESS; i++) {
        if (proctable[i] == NULL)    {
            my_pid = i;
            break;
        }else if (proctable[i]->exit == 1) { // process has already exited
            remove_process(proctable[i]->pid);
            my_pid = i;
            break;
        }
    }//for
    
    if (my_pid == 0)    {
        return NULL; // no place in proctable
    }
    
    //make the process
    struct process *new = kmalloc(sizeof(struct process));
    new->pid = my_pid;
    //new->exit_code=???;
    new->exit=0; // 0 = running
    new->table=NULL;
    new->proc_cv = cv_create("cv for a process");
    new->fd_lock = lock_create("file descriptor table lock");
    new->parent = parent;
    
    proctable[my_pid] = new;
    return new;
}

// Remove process
int remove_process(pid_t pid)   {
    struct process *kill = proctable[pid];
    
    lock_destroy(kill->fd_lock);
    kfree(kill);
    proctable[pid]=NULL;
    return 0;
}

// process's exit = 1.
int exit_process(pid_t pid, int exitcode)   {
    struct process *e = proctable[pid];
    
    e->exit_code = exitcode;
    e->exit = 1;
    
    
    //let my child know that I have exited.
    if (e->parent != NULL) {
        for (int i = MIN_PROCESS; i < MAX_PROCESS; i++) {
            /*
             conditions:
             1. this process exist
             2. this process has a parent
             3. I am not this process
             4. this process's parent is me
             
             then I am the parent, I must tell this process I have exited
             */
            if (proctable[i] != NULL && proctable[i]->parent != NULL && i != e->pid && proctable[i]->parent->pid == e->pid)  {
                c->parent = NULL; 
            }
        }//for
    }

    return 0;
}

//retrieve process by its pid
struct process* process_get(pid_t pid)  {
    if (pid  >= MIN_PROCESS && pid <= MAX_PROCESS)  {
        return proctable[pid];
    } else  {
        return NULL; // let's just return NULL if it's invalid, may change this
    }
}

#endif /* _OPT_A2_ */