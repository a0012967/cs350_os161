#include <vm-tlb.h>
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>

#include "opt-A3.h"

#if OPT_A3

int tlb_get_rr_victim(void)
{
    
    kprintf("tlb_get 1\n");
    int victim;
    struct addrspace *as = curthread->t_vmspace;
    kprintf("tlb_get 2\n");
    victim = as->tlb->next_victim;
    kprintf("tlb_get 3\n");
    as->tlb->next_victim = (as->tlb->next_victim + 1)%NUM_TLB;
    kprintf("tlb_get 4\n");
    return victim;
    
}
#endif
