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
/*
struct page* page_create()
{kprintf("Creating pages...\n");
    struct page* p = kmalloc(sizeof(struct page));

    if (p == NULL)  {
        return NULL;
    }

    p->valid = 0;
    p->vaddr=0; // for now??
    //e->as = as; // we should remove this...
    p->permission = readable; //we should discuss this, what is the dirty bit for a non-valid page?
    
    return p;
}

int pagetable_create(struct addrspace *as){ // as = the addrspace that this pt will be attached to
    kprintf("SIZE OF %d\n",sizeof(struct pagetable));
    struct pagetable *table = kmalloc(sizeof(struct pagetable));
    
    if (table == NULL) {kprintf("Return ENOMEM %d\n",sizeof(struct pagetable));
        return ENOMEM;
    }
    
    int i;
    
    //initialize pagetable with blank entries
    for (i = 0; i < N_PAGES; i++)   {
        struct page * e= page_create();
        
        if (e == NULL)  {kprintf("Return ENOMEM\n");
            return ENOMEM;
        }
        
        table->pt[i] = e;
    }
    as->pt = table;  
    return 0;
}



int pagetable_destroy(struct addrspace *as)     { // destroy the pagetable of the addrspace as
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
*/

#endif
