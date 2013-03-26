#ifndef VM_TLB_H
#define	VM_TLB_H

#include <synch.h>
#include "opt-A3.h"

#if OPT_A3
/* Code for manipulating the TLB including replacement */

struct tlb {
    
    int next_victim;
    
    struct lock *tlb_lock;
    
};

int tlb_get_rr_victim(void);

#endif
#endif	/* VM_TLB_H */

