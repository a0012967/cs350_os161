#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include "opt-A3.h"




static
paddr_t
getppages(unsigned long npages)
{
    
#if OPT_A3
    
    if(pt_initialize == 0){
        
        return ram_stealmem(npages);
        
    }
    
    paddr_t addr;
    
    return addr;
    
    
    
#else
	int spl;
	paddr_t addr;
    
	spl = splhigh();
    
	addr = ram_stealmem(npages);
	
	splx(spl);
	return addr;
    
#endif
}




vaddr_t 
alloc_kpages(int npages)
{
	paddr_t pa;
	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}


void 
free_kpages(vaddr_t addr)
{
	/* nothing */
    
	(void)addr;
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	return -1;
}




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
    as_page_dir = alloc_kpages(1);
	as->as_vbase1 = 0;
	as->as_pbase1 = 0;
	as->as_npages1 = 0;
	as->as_vbase2 = 0;
	as->as_pbase2 = 0;
	as->as_npages2 = 0;
	as->as_stackpbase = 0;
    
    //initialize pagetable for as
    int result = pagetable_create(as);
    if (result != 0) {
        //there was an error, do something here?
    }
#endif

	return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
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
}

void
as_destroy(struct addrspace *as)
{
	/*
	 * Clean up as needed.
	 */
	int result = pagetable_destroy(as);
    
    if (result != 0)    {
        //pagetable wasn't destroyed properly, do something about it?
    }
	kfree(as);
}

void
as_activate(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	(void)as;  // suppress warning until code gets written
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
    sz = (sz + PAGE_SIZE -1) &PAGE_FRAME
    npages = sz / PAGE_SIZE;
    
    
    int i;
    struct page *p;
    
    for (i = 0; i < npages; i++)    {
        p=kmalloc(sizeof(struct page));
        p->vaddr = vaddr + i * PAGE_SIZE;
                        //is this offset right? PAGE_SIZE = 4096 
        p->state = free
            // i think we should rename stat to valid, b/c it's called the valid bit.
        //p->permission = readable | writeable | executable
        
        //I need to add an array of these pages to the addrspace. we need add something in the addrspace struct like as->listofpages field so I can do something like  as->listofpages[i]=p or array_add(as->listofpages, p)
    }
    
    //don't use these - all pages are read-write
    (void)readable;
	(void)writeable;
	(void)executable;
    
    if (as->as_vbase1 == 0) {
        as->as_vbase1 = vaddr;
        as ->as_npages1 = npages;
        return 0;
    } else if (as->as_vbase2 == 0) {
        as->as_vbase2 = vaddr;
        as ->as_npages2 = npages;
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
#endif /* _OPT_A3 */
    
}

int
as_prepare_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */
#if OPT_A3
    assert(as->as_pbase1 == 0);
	assert(as->as_pbase2 == 0);
	assert(as->as_stackpbase == 0);
    
    /*
     
     //REMOVING ALL THE NON-ERROR CHECKING STUFF FOR NOW, I DON'T KNOW WHAT TO DO.
     
	as->as_pbase1 = getppages(as->as_npages1);
	if (as->as_pbase1 == 0) {
		return ENOMEM;
	}
    
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

	(void)as;
	return 0;
#endif /* _OPT_A3 */
}

int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */
    
#if OPT_A3
    //don't know yet!!!
#else
	(void)as;
	return 0;
#endif /* _OPT_A3 */
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
    struct page p;
    
    for (i=0; i < DUMBVM_STACKPAGES; i++)   {
                    //does DUMBVM_STACKPAGES still work? what else can we use? xD using the one in pt.c right now
        p=kmalloc(sizeof(struct pte));
        p->vaddr_t = USERSTACK - i * PAGE_SIZE;
                                
        e->state = free;
        //both of following is similar to define_region
        //need e->permission 
        //need to add to pages of address space
    }
#else
	(void)as;
    
	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;
	
	return 0;
#endif /* _OPT_A3 */
}

