#include <vm-tlb.h>
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>

#include "opt-A3.h"

#if OPT_A3
int tlb_get_rr_victim(void)
{
    int victim;
    struct addrspace *as = curthread->t_vmspace;
    
    victim = as->tlb->next_victim;
    as->tlb->next_victim = (as->tlb->next_victim + 1)/64;
    return victim;
    
}
#endif