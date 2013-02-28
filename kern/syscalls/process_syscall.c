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
#include <test.h>
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
pid_t getpid(){
    int i;
    for(i = 0; i<MAX_PROCESS; i++){
        
        if(proctable[i]->PID == curthread) {
            
            return proctable[i]->PID;
        }  
    }
    
    return (pid_t)(-1);

}

#endif /* _OPT_A2_ */
