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
#include <test.h>
#include "opt-A2.h"


#if OPT_A2
/*
 //plese see process.c
void proctable_init(struct process *t,int len){
    
    int i;
    for(i = 1;i<len;i++){
        t[i] = NULL;
}
*/

void _exit(int exitcode)
{
    (void)exitcode;
    kprintf("Shutting down.\n");
	
    //vfs_clearbootfs();
    //vfs_clearcurdir();
    //vfs_unmountall();
    
    splhigh();
    
    //scheduler_shutdown();
    thread_shutdown();
}


pid_t getpid(){
    int i;
    for(i = 0; i<MAX_PROCESS; i++){
        
        if(proctable[i].me == curthread){//Yi: should proctable[i].me be a thread? If so, we should start using pointers.
            
            return proctable[i].PID;
        }
        
        
    }
    
    return (pid_t)(-1);
    
}

#endif /* _OPT_A2_ */