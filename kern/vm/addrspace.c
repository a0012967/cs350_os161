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
#include "opt-A3.h"



#if OPT_A3

/* under dumbvm, always have 48k of user stack */
#define DUMBVM_STACKPAGES    12


void vm_bootstrap(){
    
    struct coremap *entry; //at the end should point to the same memory as pagetable in pt.h
    coremap_size =0;
    //allocating memory
    
    paddr_t firstaddr,lastaddr,freeaddr;
    ram_getsize(&firstaddr,&lastaddr);

    coremap_size = lastaddr/PAGE_SIZE; //addr alignment, nOTE: This rounds out to a whole number
    
    coremap = (struct coremap *)PADDR_TO_KVADDR(firstaddr); //sets the page array
    kprintf("coremap\n");
    if(coremap== NULL){
        panic("Can't create page table, no mem\n");
    }
    
    freeaddr = firstaddr + coremap_size * sizeof(struct coremap);
    kprintf("freeaddr\n");
    if(lastaddr-freeaddr <= 0){
        
        panic("OUT OF MEMORYn");
    }
    // freee addr to lastaddr is the systems main memory
    //the actual init
    
    struct coremap * p = (struct coremap *) PADDR_TO_KVADDR((paddr_t)coremap);
    kprintf("p coremap_Size %d\n",coremap_size);
    entry = coremap;
    int i;
    for(i =0;i<coremap_size;i++){
        
        if(i<((freeaddr-firstaddr)/PAGE_SIZE)){
            coremap->valid = 0;
            coremap->used = 1;

        }
        else{
            
            coremap->valid = 1;
            coremap->used =0;
            
          
        }
        
        coremap->paddr = firstaddr+((i*4)*coremap_size);
        assert((coremap->paddr & PAGE_FRAME) == coremap->paddr);
        coremap->len = -1;

        coremap++;
      
        
    }
    
    coremap = entry;

    pt_initialize =1;

  kprintf("VM BOOTSTRAP COMPLETE: page size: %d, coremap size:%d\n",PAGE_SIZE,coremap_size);
    
}
    /*
     Does most of the work for alloc
     */
static
paddr_t
getppages(unsigned long npages)
{
   // kprintf("call to getpages\n");
#if OPT_A3
    //lock_acquire(table_lock);
    //alloc_kpages can be called before vm_bootstrap so
    //we just stealmem
    if(pt_initialize != 1){
        //kprintf("pt unintilize\n");
        int spl;
        paddr_t addr;
        spl = splhigh();
        
        addr =  ram_stealmem(npages);
        
        splx(spl);
        return addr;
        //panic("PAGE TABLE NOT INITIALIZE\N");
        
    }
    
    int i,j;
    unsigned long count_pages;
   kprintf("getppages: about to count coremap\n");
    for(i = 0; i< coremap_size; i++){
        j = i - npages + 1;
        if(coremap[i].valid && !(coremap[i].used)){
            
            count_pages++;
            if(count_pages == npages){
                
            	coremap[j].len = npages;
                break;
            }
        }
        else{
            count_pages = 0;
        }
        
        
    }
   kprintf("countpages: %d, npages: %d\n", count_pages, npages);
    if(count_pages == npages){
       // int j;
        for(j =i - npages +1;j<(i+1);j++){
            
            coremap[j].used= 1;
            
            
        }
         kprintf("countpages j=%d, i=%d\n", coremap[i-npages+1].len, i);
         assert((coremap[i-npages+1].paddr & PAGE_FRAME) == coremap[i-npages+1].paddr);
        return coremap[i-npages+1].paddr;
        
    }
    
    return 0; //if not successful
    
    
    
    
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
    
 
    //virtually no change, only the implementation of getppages
	paddr_t pa;
	pa = getppages(npages);

	if (pa==0) {
        
        kprintf("pa == 0\n");
		return 0;
	}

	 
	 vaddr_t va;
	 va = PADDR_TO_KVADDR(pa);
         kprintf("HERE\n");
	 return va;
	
    
}


void 
free_kpages(vaddr_t addr)
{
    int i =0;
    while(PADDR_TO_KVADDR(coremap[i].paddr) != addr){
        
        i++;
        
    }
    assert(coremap[i].len != -1);
    
    int len =coremap[i].len;
    
    coremap[i].len = -1;
    
    for(i = 0; i < len;i++){
        coremap[i].used = 0;
    }
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	int i;
	u_int32_t ehi, elo;
	struct addrspace *as = curthread->t_vmspace;
	int spl;
#if OPT_A3    
    int p_i;
#endif /* _OPT_A3_ */
    
	spl = splhigh();
        //lock_acquire(as->tlb->tlb_lock);

	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

        
	if (as == NULL) {
		/*
		 * No address space set up. This is probably a kernel
		 * fault early in boot. Return EFAULT so as to panic
		 * instead of getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	/* Assert that the address space has been set up properly. */
	assert(as->as_vbase1 != 0);
	assert(as->as_pbase1 != 0);
	assert(as->as_npages1 != 0);
	assert(as->as_vbase2 != 0);
	assert(as->as_pbase2 != 0);
	assert(as->as_npages2 != 0);
	assert(as->as_stackpbase != 0);
	assert((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	assert((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
	assert((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
	assert((as->as_pbase2 & PAGE_FRAME) == as->as_pbase2);
	assert((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);

	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;
        
    /* 
        // YI: THIS SECTION IS WRONG!!!: paddr may have use=0
	if (faultaddress >= vbase1 && faultaddress < vtop1) {
		paddr = (faultaddress - vbase1) + as->as_pbase1;
	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
		paddr = (faultaddress - vbase2) + as->as_pbase2;
	}
	else if (faultaddress >= stackbase && faultaddress < stacktop) {
		paddr = (faultaddress - stackbase) + as->as_stackpbase;
	}
	else {
		splx(spl);
                //lock_release(as->tlb->tlb_lock);
		return EFAULT;
	}
     */
    
	/* make sure it's page-aligned */
	assert((paddr & PAGE_FRAME)==paddr);
        
    
    //---Yi: need to find segment, then page. NOTE: THE INDEX MAY BE OFF BY +-1 because of the =
    
    p_i = faultaddress/PAGE_SIZE;
    
    int segment; //0code 1data 2stack
        
    struct page *pg;
    
    if (p_i < 0)    {
        panic("addrspace: invalid page table index\n"); // need exception handling
        
    } else if (faultaddress >= vbase1 && faultaddress < vtop1)   { // look in code
        pg = (struct page *)array_getguy(as->useg1, (faultaddress-vbase1)/PAGE_SIZE);
        segment=0;
    } else if (faultaddress >= vbase2 && faultaddress < vtop2)    { // look in data
        pg = (struct page *)array_getguy(as->useg2, (faultaddress-vbase2)/PAGE_SIZE);
        segment =1;
    } else if (faultaddress >= stackbase && faultaddress < stacktop) { // look in stack
        kprintf("size of stack %d\n", array_getnum(as->usegs));
        pg = (struct page *)array_getguy(as->usegs, ((faultaddress - stackbase)/PAGE_SIZE));
        segment = 2;
    } else {
        panic("addrspace: faultaddress not valid!!\n");
    }
    //---
        
    
    
        if (pg == NULL)
        {
            //create page
            pg = kmalloc(sizeof(struct page));
            pg->vaddr = faultaddress;
            pg->paddr = paddr;
            pg->valid = 1;
            if (segment == 0)   {
                pg->permission = 0x5;
            } else if (segment == 1)    {
                pg->permission = 0x6;
            } else {
                pg->permission = 0x7;
            }
                
            //as->pt->pt[i] = pg; //don't need this line, done in line 293
        }
    
        
        if (!pg->valid) // page not in page table
        {
            paddr = getppages(1);
            if (paddr == NULL)  {
                return ENOMEM;
            }
            pg->paddr = paddr;
            pg->valid = 1;
            pg->vaddr = faultaddress;
        } // may need else case for tlb reload
        
	switch (faulttype) {
	    case VM_FAULT_READONLY:
                
                // we double check that the user has indeed permission to write to page

                
                
                
                    if (pg->permission == 1) // can be written to, as set in as_defined_region
                    {
                        int tlb_entry = TLB_Probe(faultaddress, 0);
                        
                        TLB_Read(&ehi, &elo, tlb_entry);
                        
                        if (ehi != faultaddress)
                            panic("vm_fault: ehi not equal to faultaddr\n");
                        
                        elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
                        
                        TLB_Write(ehi, elo, tlb_entry);
                        
                        break;
                    }
                    else
                    {
                        int retval; // useless for now
                        _exit(0, &retval);   
                    }
                
		/* We always create pages read-write, so we can't get this */
		//panic("dumbvm: got VM_FAULT_READONLY\n");
	    case VM_FAULT_READ: // need to add a new page
                
                break;
                
	    case VM_FAULT_WRITE:
		break;
	    default:
		splx(spl);
                //lock_release(as->tlb->tlb_lock);
		return EINVAL;
	}

	

	for (i=0; i<NUM_TLB; i++) {
		TLB_Read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = faultaddress;
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		TLB_Write(ehi, elo, i);
		splx(spl);
                //lock_release(as->tlb->tlb_lock);
		return 0;
	}

        // need a replacement algorithm
	kprintf("dumbvm: Ran out of TLB entries - cannot handle page fault\n");
	splx(spl);
        int victim = tlb_get_rr_victim();
        if (victim <= 0 || victim >= NUM_TLB)
            return EFAULT;
        
        TLB_Write(ehi, elo, victim);
        
        //lock_release(as->tlb->tlb_lock);
	return 0;
}


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
#if OPT_A3
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
    
	assert(new->as_pbase1 != 0);
	assert(new->as_pbase2 != 0);
	assert(new->as_stackpbase != 0);
    
    unsigned int i;
    struct page *e;
    
    for (i = 0; i < new->as_npages1; i++)   {
        e = kmalloc(sizeof(struct page));
        e->vaddr = new->as_vbase1 + i * PAGE_SIZE;
        e->paddr = new->as_pbase1 + i + PAGE_SIZE;
        e->valid = 1;
        e->permission = 5; //re
        array_add(new->useg1, e);
    }
	memmove((void *)PADDR_TO_KVADDR(new->as_pbase1),
            (const void *)PADDR_TO_KVADDR(old->as_pbase1),
            old->as_npages1*PAGE_SIZE);
    
    
    for (i = 0; i < new->as_npages2; i++)   {
        e = kmalloc(sizeof(struct page));
        e->vaddr = new->as_vbase2 + i * PAGE_SIZE;
        e->paddr = new->as_pbase2 + i + PAGE_SIZE;
        e->valid = 1;
        e->permission = 6; //rw
        array_add(new->useg2, e);
    }
	memmove((void *)PADDR_TO_KVADDR(new->as_pbase2),
            (const void *)PADDR_TO_KVADDR(old->as_pbase2),
            old->as_npages2*PAGE_SIZE);
    
    for (i = 0; i < DUMBVM_STACKPAGES; i++)   {
        e = kmalloc(sizeof(struct page));
        e->vaddr = USERSTACK - i * PAGE_SIZE;
        e->paddr = new->as_stackpbase + i + PAGE_SIZE;
        e->valid = 1;
        e->permission = 7; //rwe
        array_add(new->usegs, e);
    }
    
	memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase),
            (const void *)PADDR_TO_KVADDR(old->as_stackpbase),
            DUMBVM_STACKPAGES*PAGE_SIZE);
	
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
    
    if (as != curthread->t_vmspace)
    {
        for (i=0; i<NUM_TLB; i++) {
            TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
        }
    }
    
	splx(spl);
#else
    (void)as;
#endif /* _OPT_A3_ */
}

//helper function for as_define_region -  to determine page's permission
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
            p->permission = as_set_permission(readable,writeable,executable);
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
#endif /* _OPT_A3_ */
    
}

int
as_prepare_load(struct addrspace *as)
{
#if OPT_A3
    
    assert(as->as_pbase1 == 0);
	assert(as->as_pbase2 == 0);
	assert(as->as_stackpbase == 0);
    
        
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
        p->permission = 7;
        array_add(as->usegs, p);
    }
    //as->as_stackpbase = p->vaddr; //MAY BE OFF BY ONE ADDRESS!!
    *stackptr = USERSTACK;
    return 0;
#else
	(void)as;
    
	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;
	
	return 0;
#endif /* _OPT_A3_ */
}





//////// end addrspace_yi//////////


/*
// Note! If OPT_DUMBVM is set, as is the case until you start the VM
// assignment, this file is not compiled or linked or in any way
// used. The cheesy hack versions in dumbvm.c are used instead.
//

struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}
        
#if OPT_A3
        int err;
        
        kprintf("address of as %x\n",as);
        err = pagetable_create(as);
        //as->pt = array_create();
        
        if (err)
            return NULL;
        
        // create the lock for the tlb table
        //as->tlb->tlb_lock = lock_create("tlb lock");
        
        //if (as->tlb->tlb_lock == NULL)
            //panic("as_create: Cannot create tlb lock\n");
        
        // initialize next victim for round robin algo
        //as->tlb->next_victim = 0;
        
	as->as_vbase1 = 0;
	as->as_pbase1 = 0;
	as->as_npages1 = 0;
	as->as_vbase2 = 0;
	as->as_pbase2 = 0;
	as->as_npages2 = 0;
	as->as_stackpbase = 0;

	return as;
        
#else
        return as;
#endif
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
#if OPT_A3
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

	assert(new->as_pbase1 != 0);
	assert(new->as_pbase2 != 0);
	assert(new->as_stackpbase != 0);

	memmove((void *)PADDR_TO_KVADDR(new->as_pbase1),
		(const void *)PADDR_TO_KVADDR(old->as_pbase1),
		old->as_npages1*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_pbase2),
		(const void *)PADDR_TO_KVADDR(old->as_pbase2),
		old->as_npages2*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase),
		(const void *)PADDR_TO_KVADDR(old->as_stackpbase),
		DUMBVM_STACKPAGES*PAGE_SIZE);
	
	*ret = new;
	return 0;
#else
        struct addrspace *newas;
        
        newas = as_create();
        if (newas == NULL){
            return ENOMEM;
        }
        
        (void)old;
        
        *ret = newas;
        return 0;
#endif
}

void
as_destroy(struct addrspace *as)
{
#if OPT_A3
    
    //lock_destroy(as->tlb);
    pagetable_destroy(as);
    kfree(as);
    
#else
	
	 // Clean up as needed.
	 
	
	kfree(as);
#endif
}

void
as_activate(struct addrspace *as)
{
#if OPT_A3
    kprintf("In as activate\n");
	
    //Write this.
	 
    int i, spl;

	(void)as;

	spl = splhigh();

	// invalidate entries in TLB only if address spaces are different
        
        if (as != curthread->t_vmspace)
        {
            for (i=0; i<NUM_TLB; i++) {
                    TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
            }
        }

	splx(spl);
#else
        (void)as;
#endif
}


int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
#if OPT_A3
    size_t npages; 
    kprintf("In as define region\n");
  //  kprintf("Pageframe: %x, size %d\n",PAGE_FRAME,sz);
	//Align the region. First, the base... 
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;
  //  kprintf("Pageframe after: %x, size %d\n",PAGE_FRAME,sz);
	// ...and now the length. 
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

	// We don't use these - all pages are read-write 
	(void)readable;
	(void)writeable;
	(void)executable;

	if (as->as_vbase1 == 0) {
		as->as_vbase1 = vaddr;
		as->as_npages1 = npages;
		return 0;
	}

	if (as->as_vbase2 == 0) {
		as->as_vbase2 = vaddr;
		as->as_npages2 = npages;
		return 0;
	}

	
	 // Support for more than two regions is not available.
	 
	kprintf("dumbvm: Warning: too many regions\n");
	return EUNIMP;
        
#else

	(void)as;
	(void)vaddr;
	(void)sz;
	(void)readable;
	(void)writeable;
	(void)executable;
	return EUNIMP;
#endif
       
}

int
as_prepare_load(struct addrspace *as)
{
#if OPT_A3
    kprintf("In as prepare load\n");
    assert(as->as_pbase1 == 0);
	assert(as->as_pbase2 == 0);
	assert(as->as_stackpbase == 0);
    
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

	return 0;
#else

	(void)as;
	return 0;
#endif
}

int
as_complete_load(struct addrspace *as)
{
#if OPT_A3
    kprintf("In as complete load\n");

        (void)as;
	return 0;
#else
	(void)as;
	return 0;
#endif
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
#if OPT_A3
        assert(as->as_stackpbase != 0);
        kprintf("In as define stack\n");
	*stackptr = USERSTACK;
	return 0;
#else
	

	(void)as;

	
	*stackptr = USERSTACK;
	
	return 0;
#endif
}

*/
