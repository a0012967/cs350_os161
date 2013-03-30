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
    
    //initialize pagetable for addrspace
    as->useg1 = array_create();
    as->useg2 = array_create();
    as->usegs = array_create();
    
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
	array_destroy(as->useg1);
    array_destroy(as->useg2);
    array_destroy(as->usegs);
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


//helper function to determine page's permission
int as_set_permission(int r, int w, int e)  {
    if (r == 0) {
        if (w == 0) {
            if (e == 0) {
                return 0; //000
            } else {// e = 1
                return 3;   //001
            }
        } else { // w = 1
            if (e == 0) {
                return 2; //010
            } else {// e = 1
                return 6; //011
            }
        }
    } else {//r = 1
        if (w == 0) {
            if (e == 0) {
                return 1; //100
            } else {// e = 1
                return 5; //101
            }
        } else { // w = 1
            if (e == 0) {
                return 4; //110
            } else {// e = 1
                return 7; //111
            }
        }
    }
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
    
    
    
    if (as->as_vbase1 == 0) { // code segment useg1
        as->as_vbase1 = vaddr;
        as ->as_npages1 = npages;
        
        int i;
        struct page *p;
        
        for (i = 0; i < npages; i++)    {
            p = kmalloc(sizeof(struct page));
            p->vaddr = vaddr + i * PAGE_SIZE;
            p->permission = as_set_permission(readable,writeable,executable);
            array_add(as->useg1, p);
        }
        return 0;
    } else if (as->as_vbase2 == 0) { // data segment useg2
        as->as_vbase2 = vaddr;
        as ->as_npages2 = npages;
        
        int i;
        struct page *p;
        
        for (i = 0; i < npages; i++)    {
            p = kmalloc(sizeof(struct page));
            p->vaddr = vaddr + i * PAGE_SIZE;
            p->permission = as_set_permission(readable,writeable,executable);
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
    (void)as;
	return 0;
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
        p=kmalloc(sizeof(struct page));
        p->vaddr = USERSTACK - i * PAGE_SIZE;//since we are going backwards
                                
        p->valid =0;
        // ???? p->permission = 
        array_add(as->usegs, p);
    }
    as->as_stackpbase = p->vaddr; //MAY BE OFF BY ONE ADDRESS!!
    *stackptr = USERSTACK;
    return 0;
#else
	(void)as;
    
	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;
	
	return 0;
#endif /* _OPT_A3 */
}

