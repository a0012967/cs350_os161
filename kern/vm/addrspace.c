#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <pt.h>
#include <coremap.h>
#include <machine/tlb.h>
#include <machine/spl.h>
#include <vm-tlb.h>
#include <syscall.h>
#include <uw-vmstats.h>
#include "opt-A3.h"



#if OPT_A3

/* under dumbvm, always have 48k of user stack */
#define DUMBVM_STACKPAGES    12


#endif /* _OPT_A3_*/



////////addrspace_yi//////////

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}
    
	/*
	 * Initialize as needed.
	 */
#if OPT_A3
    //as->as_page_dir = alloc_kpages(1);
	as->as_vbase1 = 0;
	//as->as_pbase1 = 0;
	as->as_npages1 = 0;
	as->as_vbase2 = 0;
	//as->as_pbase2 = 0;
	as->as_npages2 = 0;
	//as->as_stackpbase = 0;
    
    //initialize pagetable for addrspace
    as->useg1 = array_create();
    as->useg2 = array_create();
    as->usegs = array_create();
    //as->tlb = kmalloc(sizeof(struct tlb));   
    //as->tlb->tlb_lock = lock_create("tlb lock");
    //as->tlb->next_victim = 0;
    
#endif
    
	return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
#if OPT_A3
    
    // as_copy IS BROKEN RIGHT NOW!!!!!
    struct addrspace *new;
    
	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}
    
	new->as_vbase1 = old->as_vbase1;
	new->as_npages1 = old->as_npages1;
	new->as_vbase2 = old->as_vbase2;
	new->as_npages2 = old->as_npages2;
    
	if (as_prepare_load(new)) {
		as_destroy(new);
		return ENOMEM;
	}
    
	//assert(new->as_pbase1 != 0);
	//assert(new->as_pbase2 != 0);
	//assert(new->as_stackpbase != 0);
    
    unsigned int i;
    struct page *e;
    
    for (i = 0; i < new->as_npages1; i++)   {
        e = kmalloc(sizeof(struct page));
        e->vaddr = new->as_vbase1 + i * PAGE_SIZE;
        //e->paddr = new->as_pbase1 + i + PAGE_SIZE;
        e->valid = 1;
        e->permission = 0x5; //re
        array_add(new->useg1, e);
    }
	/*
    memmove((void *)PADDR_TO_KVADDR(new->as_pbase1),
            (const void *)PADDR_TO_KVADDR(old->as_pbase1),
            old->as_npages1*PAGE_SIZE);
    */
    
    for (i = 0; i < new->as_npages2; i++)   {
        e = kmalloc(sizeof(struct page));
        e->vaddr = new->as_vbase2 + i * PAGE_SIZE;
        //e->paddr = new->as_pbase2 + i + PAGE_SIZE;
        e->valid = 1;
        e->permission = 0x6; //rw
        array_add(new->useg2, e);
    }
	/*
    memmove((void *)PADDR_TO_KVADDR(new->as_pbase2),
            (const void *)PADDR_TO_KVADDR(old->as_pbase2),
            old->as_npages2*PAGE_SIZE);
     */
    
    for (i = 0; i < DUMBVM_STACKPAGES; i++)   {
        e = kmalloc(sizeof(struct page));
        e->vaddr = USERSTACK - i * PAGE_SIZE;
        //e->paddr = new->as_stackpbase + i + PAGE_SIZE;
        e->valid = 1;
        e->permission = 0x7; //rwe
        array_add(new->usegs, e);
    }
    
    /*
	memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase),
            (const void *)PADDR_TO_KVADDR(old->as_stackpbase),
            DUMBVM_STACKPAGES*PAGE_SIZE);
     */
	
	*ret = new;
	return 0;
    
    
#else
    struct addrspace *newas;
    
	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}
    
	/*
	 * Write this.
	 */
    
	(void)old;
	
	*ret = newas;
	return 0;
#endif /* _OPT_A3_ */
}

void
as_destroy(struct addrspace *as)
{
	array_destroy(as->useg1);
    array_destroy(as->useg2);
    array_destroy(as->usegs);
    //lock_destroy(as->tlb->tlb_lock);
	kfree(as);
}

void
as_activate(struct addrspace *as)
{
#if OPT_A3
	/*
	 * Write this.
	 */
    int i, spl;
    
	(void)as;
    
	spl = splhigh();
    
	// invalidate entries in TLB only if address spaces are different
    
    //if (as != curthread->t_vmspace)
    //{
        vmstats_inc(VMSTAT_TLB_INVALIDATE); /* STATS */
        for (i=0; i<NUM_TLB; i++) {
            TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
        }
    //}
    
	splx(spl);
#else
    (void)as;
#endif /* _OPT_A3_ */
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
                 int readable, int writeable, int executable)
{ 
	/*
	 * Write this.
	 */
#if OPT_A3
    
    size_t npages;
    
    // Align the region: the base
    sz+= vaddr & ~(vaddr_t)PAGE_FRAME;
    vaddr &= PAGE_FRAME;
    
    // Align the region: the length
    sz = (sz + PAGE_SIZE -1) &PAGE_FRAME;
    npages = sz / PAGE_SIZE;
    
    
    
    if (as->as_vbase1 == 0) { // code segment useg1
        as->as_vbase1 = vaddr;
        as ->as_npages1 = npages;
        
        unsigned int i;
        struct page *p;
        
        for (i = 0; i < npages; i++)    {
            p = kmalloc(sizeof(struct page));
            p->vaddr = vaddr + i * PAGE_SIZE;
            p->permission = readable|writeable|executable;
            p->valid = 0;
            array_add(as->useg1, p);
        }
        return 0;
    } else if (as->as_vbase2 == 0) { // data segment useg2
        as->as_vbase2 = vaddr;
        as ->as_npages2 = npages;
        
        unsigned int i;
        struct page *p;
        
        for (i = 0; i < npages; i++)    {
            p = kmalloc(sizeof(struct page));
            p->vaddr = vaddr + i * PAGE_SIZE;
            p->permission = readable|writeable|executable;
            p->valid = 0;
            array_add(as->useg2, p);
        }
        
        return 0;
    }
    kprintf("addrspace.c: Warning: too many regions\n");
    return EUNIMP;
    
    
#else
    
    
	(void)as;
	(void)vaddr;
	(void)sz;
	(void)readable;
	(void)writeable;
	(void)executable;
	return EUNIMP;
#endif /* _OPT_A3_ */
    
}

int
as_prepare_load(struct addrspace *as)
{
#if OPT_A3
    
    //assert(as->as_pbase1 == 0);
	//assert(as->as_pbase2 == 0);
	//assert(as->as_stackpbase == 0);
    
        
    /*
	as->as_pbase1 = getppages(as->as_npages1);
	if (as->as_pbase1 == 0) {
		return ENOMEM;
	}
    assert((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
	as->as_pbase2 = getppages(as->as_npages2);
	if (as->as_pbase2 == 0) {
		return ENOMEM;
	}
        
	as->as_stackpbase = getppages(DUMBVM_STACKPAGES);
	if (as->as_stackpbase == 0) {
		return ENOMEM;
	}
     */
    
	return 0;
#else
	/*
	 * Write this.
	 */
    
	(void)as;
	return 0;
#endif /*_OPT_A3_*/
}

int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */
    
#if OPT_A3
    //don't know yet!!!
    (void)as;
	return 0;
#else
	(void)as;
	return 0;
#endif /* _OPT_A3_ */
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/*
	 * Write this.
	 */
#if OPT_A3
    //don't know yet!!!
    int i;
    struct page *p;
    
    for (i=0; i < DUMBVM_STACKPAGES; i++)   {
        //does DUMBVM_STACKPAGES still work? what else can we use? xD using the one in pt.c right now
        p=kmalloc(sizeof(struct page));
        p->vaddr = USERSTACK - i * PAGE_SIZE;//since we are going backwards
        
        p->valid =0;
        p->permission = 0x7;
        array_add(as->usegs, p);
    }

    *stackptr = USERSTACK;
    return 0;
#else
	(void)as;
    
	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;
	
	return 0;
#endif /* _OPT_A3_ */
}