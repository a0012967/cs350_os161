#include <synch.h>
#include <thread.h>
#include <types.h>
#include "opt-a2.h"


#if OPT_A2

#define max_process 100


struct process {
    
    pid_t PID;
    int exit_code;
    int exit; //if exited,1 else 0
    struct thread* me; //can call fdtable from this one
    struct cv* proc_cv;
    struct lock* proc_lock;
    
    
    
    
}

process proctable[max_process];




void proctable_init(process* t,int len);
void add_process(process* t,process p);
pid_t getpid();
pid_t waitpid(pid_t pid, int* status, int options);
pid_t fork();


#endif
