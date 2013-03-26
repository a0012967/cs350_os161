#include <pt.h>
#include <addrspace.h>
#include <vm.h>
#include <elf.h>
#include <syscall.h>
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <coremap.h>
#include <thread.h>
#include "opt-A3.h"




//THERE IS PORBABLY A SYNTAX ERROR HERE... SOMEWHERE//


/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground. You should replace all of this
 * code while doing the VM assignment. In fact, starting in that
 * assignment, this file is not included in your kernel!
 */

/* under dumbvm, always have 48k of user stack */
#define DUMBVM_STACKPAGES    12

#if OPT_A3


int pagetable_create(addrspace *as)     { // as = the addrspace that this pt will be attached to
    struct pagetable *table = kmalloc(sizeof(struct pagetable));
    
    if (table == NULL) {
        return ENOMEM;
    }
    
    int i;
    
    //initialize pagetable with blank entries
    for (i = 0; i < N_PAGES; i++)   {
        struct page e= kmalloc(sizeof(struct page));
        
        if (e == NULL)  {
            return ENOMEM;
        }
        
        e->page_state = free;
        e->vaddr=0; // for now??
        e->as = as; // we should remove this...
        e->permission = readable; //we should discuss this, what is the dirty bit for a non-valid page?
        
        table->pt[i] = e;
    }
    as->pt = table;  
    return 0;
}



int pagetable_destroy(addrspace *as)     { // destroy the pagetable of the addrspace as
    if (as == NULL) {
        return EAGAIN;
    }
    
    int i;
    for (i = 0; i < N_PAGES; i++)   {
        if (as->pt->pt[i] != NULL) {
            kfree(as->pt->pt[i] );
            as->pt->pt[i] = NULL;
        }
        
        
    }
    kfree(as->pt);
    return 0;
}


#endif
