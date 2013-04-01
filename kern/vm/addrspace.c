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

   /*
    * note:
    * permissions as set in elf.h:
    * readable = 0x4
    * writable = 0x2
    * executable = 0x1
    * 
    */

#define R_ONLY 0x4
#define W_ONLY 0x2
#define E_ONLY 0x1
#define RW_ONLY 0x6
#define RE_ONLY 0x5
#define WE_ONLY 0x3
#define RWE 0x7

void vm_bootstrap(){
    
    core_lock = lock_create("coremap-lock");
    
    
    struct coremap *entry; //at the end should point to the same memory as pagetable in pt.h
    coremap_size =0;
    //allocating memory
    
    paddr_t firstaddr,lastaddr,freeaddr;
    ram_getsize(&firstaddr,&lastaddr);
    
    coremap_size = lastaddr/PAGE_SIZE; //addr alignment, nOTE: This rounds out to a whole number
    
    coremap = (struct coremap *)PADDR_TO_KVADDR(firstaddr); //sets the page array
    
    if(coremap== NULL){
        panic("Can't create page table, no mem\n");
    }
    
    freeaddr = firstaddr + coremap_size * sizeof(struct coremap);
    //kprintf("freeaddr\n");
    if(lastaddr-freeaddr <= 0){
        
        panic("OUT OF MEMORYn");
    }
    // freee addr to lastaddr is the systems main memory
    //the actual init
    
    struct coremap * p = (struct coremap *) PADDR_TO_KVADDR((paddr_t)coremap);
    
    entry = coremap;
    int i;
    for(i =0;i<coremap_size;i++){
        
        if(i<((freeaddr-firstaddr)/PAGE_SIZE)){
            
            coremap->used = 1;
            
        }
        else{
            
            
            coremap->used =0;
            
            
        }
        coremap->pid = -1;
        coremap->paddr = firstaddr+(i*PAGE_SIZE);
        // kprintf("coremap: paddr %p  PAGE_FRAME: %p, bitwise and: %p\n",(void *) coremap->paddr,(void *)PAGE_FRAME, (void *)(coremap->paddr & PAGE_FRAME));
        assert((coremap->paddr & PAGE_FRAME) == coremap->paddr); //checks if the paddr is in the frame
        coremap->len = -1;
        
        coremap++;
        
        
    }
    
    coremap = entry;
    
    pt_initialize =1;
    
    //kprintf("VM BOOTSTRAP COMPLETE: page size: %d, coremap size:%d\n",PAGE_SIZE,coremap_size);
    
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
    lock_acquire(core_lock);
    int i,j;
    unsigned long count_pages;
    //kprintf("getppages: about to count coremap\n");
    for(i = 0; i< coremap_size; i++){
        j = i - npages + 1;
        if(!(coremap[i].used)){
            
            count_pages++;
            if(count_pages == npages){
                
            	coremap[j].len = npages;
                // assert(curthread != NULL);
                // assert(curthread->t_process != NULL);
                // coremap[j].pid = curthread->t_process->PID;
                break;
            }
        }
        else{
            count_pages = 0;
        }
        
        
    }
    
    if(count_pages == npages){
        // int j;
        for(j =i - npages +1;j<(i+1);j++){
            //coremap[j].pid = curthread->t_process->PID;
            coremap[j].used= 1;
            
            
        }

        assert((coremap[i-npages+1].paddr & PAGE_FRAME) == coremap[i-npages+1].paddr);//make sure the address is in the frame
        
        lock_release(core_lock);

        return coremap[i-npages+1].paddr;
        
    }
    lock_release(core_lock);
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

void coremap_insertpid(paddr_t pa,pid_t pid){
    int i;
    for(i=0;i<coremap_size;i++){
        
        if(coremap[i].paddr == pa){
            break;
            
        }
        
    }
    
    int len = coremap[i].len;
    coremap[i].pid = pid;
    
}



vaddr_t 
alloc_kpages(int npages)
{
    

    //kprintf("call to alloc_kpages: npages: %d\n",npages);

    //virtually no change, only the implementation of getppages
	paddr_t pa;
	pa = getppages(npages);
    
	if (pa==0) {
        
        kprintf("pa == 0\n");
		return 0;
	}
    
    


    vaddr_t va;
    va = PADDR_TO_KVADDR(pa);
    
    if(!(va > USERTOP)){
        assert(curthread!=NULL);
        assert(curthread->t_process != NULL);
        coremap_insertpid(pa,curthread->t_process->PID);
    }
    // kprintf("alloc address: %p, npages: %d\n",va,npages);
    
    

    return va;
	
    
}


void 
free_kpages(vaddr_t addr)
{
    lock_acquire(core_lock);
    // kprintf("trying to free: %p\n",addr);
    int i =0;
    //kprintf("paddr: %p\n", PADDR_TO_KVADDR(coremap[i].paddr));
    //kprintf("1\n");
    
    if(!(addr>USERTOP)){
       // kprintf("this case: %d\n",curthread->t_process->PID);
        for(i = 0; i< coremap_size;i++){
            
            assert(curthread != NULL);
            assert(curthread->t_process != NULL);
            if(PADDR_TO_KVADDR(coremap[i].paddr) == addr && coremap[i].pid == curthread->t_process->PID){
                
                //i++;
                //kprintf("Found the vaddr: at i: %d\n",i);
                break;
            }
        }
    }
    else{
        
        for(i = 0; i< coremap_size;i++){
            
            
            if(PADDR_TO_KVADDR(coremap[i].paddr) == addr){
                
                //i++;
                //kprintf("Found the vaddr: at i: %d\n",i);
                break;
            }
        }
    }
    
    if(i >= coremap_size){
        
        panic("Couldn't find paddr matching vaddr needing to free\n");
        
    }
    
    
    
    /*
     while(PADDR_TO_KVADDR(coremap[i].paddr) != addr){
     kprintf("paddr: %p i: %d\n", PADDR_TO_KVADDR(coremap[i].paddr),i);
     //kprintf("1\n");
     i++;
     
     
     
     }*/
    
    
    
    /*
     if(val )
     int j;
     for(j = 0; j<coremap_size;j++){
     
     kprintf("j: %d paddr: %p, used: %d, len: %d \n",j,coremap[j].paddr,coremap[j].used,coremap[j].len);
     
     }*/
    
    
    
    
    //kprintf("i is: %d, len is %d, paddr is %p\n",i,coremap[i].len,PADDR_TO_KVADDR(coremap[i].paddr));
    
    assert(coremap[i].len != -1);
    
    int len =coremap[i].len;
    
    coremap[i].len = -1;
    int z;
    for(z = 0; z < len;z++){
        //coremap[z+i].pid = -1;
        coremap[z+i].used = 0;
    }
    // kprintf("finished freeing \n");
    /*
     if(i == 10){
     
     int j = 0;
     kprintf("---------------------\n");
     for(j =0;j<20;j++){
     
     
     kprintf("j: %d, paddr: %p , used: %d, len: %d\n",j,coremap[j].paddr,coremap[j].used,coremap[j].len);
     
     
     }
     
     
     }
     */
    lock_release(core_lock);
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	int i;
	u_int32_t ehi, elo;
	struct addrspace *as;
	int spl;
#if OPT_A3    
    int p_i;
#endif /* _OPT_A3_ */
    
	spl = splhigh();
    //lock_acquire(as->tlb->tlb_lock);
    
	faultaddress &= PAGE_FRAME;
    
	//DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);
    //swtich(faulttype) used to be here
    as = curthread->t_vmspace;
	if (as == NULL) {
		/*
		 * No address space set up. This is probably a kernel
		 * fault early in boot. Return EFAULT so as to panic
		 * instead of getting into an infinite faulting loop.
		 */
		return EFAULT;
	}
    
	/* Assert that the address space has been set up properly. 
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
    */
	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;
    

    
	/* make sure it's page-aligned */
	//assert((paddr & PAGE_FRAME)==paddr);
    
    
    //---Yi: need to find segment, then page. NOTE: THE INDEX MAY BE OFF BY +-1 because of the =
    
    p_i = faultaddress/PAGE_SIZE;
    
    int segment; // -1invalid 0code 1data 2stack
    
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
        pg = (struct page *)array_getguy(as->usegs, ((faultaddress - stackbase)/PAGE_SIZE));
        segment = 2;
    } else {
        segment = -1;
    }
    
    //---
   
    int wr_to = 0;

    //kprintf("Fault %d, Permission %x, Addr %x\n", faulttype,pg->permission,faultaddress );
	switch (faulttype) {
	    case VM_FAULT_READONLY:
                
                // we double check that the user has indeed permission to write to page
                if (pg->permission == W_ONLY || pg->permission == RW_ONLY ||
                        pg->permission == WE_ONLY || pg->permission == RWE) // can be written to, as set in as_defined_region
                {
                    int tlb_entry = TLB_Probe(faultaddress, 0);

                    TLB_Read(&ehi, &elo, tlb_entry);

                    if (ehi != faultaddress)
                        panic("vm_fault: ehi not equal to faultaddr\n");

                    elo = pg->paddr | TLBLO_DIRTY | TLBLO_VALID;

                    TLB_Write(ehi, elo, tlb_entry);

                    wr_to = 1;
                    break;
                }
                else
                {
                    splx(spl);
                     //lock_release(as->tlb->tlb_lock);
                    int err; // useless for now
                    err = EFAULT;
                    _exit(0, &err);   
                }
                
		/* We always create pages read-write, so we can't get this */
	    case VM_FAULT_READ: // need to add a new page
                if (!pg->valid)
                {
                    pg->valid = 1;
                    pg->vaddr = faultaddress;
                    paddr = getppages(1);
                    if (paddr == NULL)
                    {
                        //lock_release(as->tlb->tlb_lock);
                        return ENOMEM;
                    }
                    pg->paddr = paddr;
                    
                
                    _vmstats_inc(VMSTAT_PAGE_FAULT_ZERO); /* STATS */
                }
                else
                {
                    paddr = pg->paddr;
                    _vmstats_inc(VMSTAT_TLB_RELOAD); /* STATS */
                }
            break;
            
	    case VM_FAULT_WRITE:
                /*
                if (pg->permission == R_ONLY || pg->permission == RE_ONLY) {
                    splx(spl);kprintf("data2 %x\n",pg->permission);
                    int err; // useless for now
                    err = EFAULT;
                    _exit(0, &err);  
                }*/
                
                if (!pg->valid)
                {
                    pg->valid = 1;
                    pg->vaddr = faultaddress;
                    paddr = getppages(1);
                    if (paddr == NULL)
                    {
                        //lock_release(as->tlb->tlb_lock);
                        return ENOMEM;
                    }
                    pg->paddr = paddr;


                    _vmstats_inc(VMSTAT_PAGE_FAULT_ZERO); /* STATS */
                }
                else
                {
                    paddr = pg->paddr;
                    _vmstats_inc(VMSTAT_TLB_RELOAD); /* STATS */
                }
		break;
	    default:
		splx(spl);
                //lock_release(as->tlb->tlb_lock);
		return EINVAL;
	}

	_vmstats_inc(VMSTAT_TLB_FAULT);

        if (!wr_to) {
            for (i=0; i<NUM_TLB; i++) {
                    TLB_Read(&ehi, &elo, i);
                    if (elo & TLBLO_VALID) {
                            continue;
                    }
                    ehi = faultaddress;

                    //if (faultaddress >= vbase1 && faultaddress < vtop1){
                         //elo = paddr | TLBLO_VALID;
                    //}else{
                        elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
                    //}

                    DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
                    TLB_Write(ehi, elo, i);
                    splx(spl);
            //lock_release(as->tlb->tlb_lock);

                    vmstats_inc(VMSTAT_TLB_FAULT_FREE); /* STATS */
                    return 0;
            }

        // need a replacement algorithm
            //kprintf("dumbvm: Ran out of TLB entries - cannot handle page fault\n");
            splx(spl);
            int victim = tlb_get_rr_victim();
            //kprintf("vm fault: got our victim, %d \n",victim);
            if (victim < 0 || victim >= NUM_TLB)
            {
                 //lock_release(as->tlb->tlb_lock);
                return EFAULT;
            }

            ehi = faultaddress;
            //if (faultaddress >= vbase1 && faultaddress < vtop1){
                //elo = paddr | TLBLO_VALID;
           //}else{
               elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
           //}

            TLB_Write(ehi, elo, victim);
            splx(spl);

            vmstats_inc(VMSTAT_TLB_FAULT_REPLACE); /* STATS */
            //lock_release(as->tlb->tlb_lock);
                return 0;
        }
        else{
             //lock_release(as->tlb->tlb_lock);
            splx(spl);
            vmstats_inc(VMSTAT_TLB_FAULT_REPLACE); /* STATS */
            return 0;
        }
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
    as->tlb = kmalloc(sizeof(struct tlb));   
    as->tlb->tlb_lock = lock_create("tlb lock");
    as->tlb->next_victim = 0;
    
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
    lock_destroy(as->tlb->tlb_lock);
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