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
    int victim;
    //struct addrspace *as = curthread->t_vmspace;
    victim = tlb.next_victim;
    tlb.next_victim = (tlb.next_victim + 1)%NUM_TLB;
    return victim;
    
}
#endif
