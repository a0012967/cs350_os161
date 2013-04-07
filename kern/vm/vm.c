#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <uio.h>
#include <elf.h>
#include <synch.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <pt.h>
#include <coremap.h>
#include <clock.h>
#include <machine/tlb.h>
#include <machine/spl.h>
#include <vm-tlb.h>
#include <syscall.h>
#include <addrspace.h>
#include <uw-vmstats.h>
#include <vnode.h>
#include "opt-A3.h"


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

int first_load = 1;
int dup = 0;
int count_dup = 0;

void vm_bootstrap(){
    
    core_lock = lock_create("coremap-lock");
    tlb.tlb_lock = lock_create("tlb-lock");
    tlb.next_victim = 0;
    
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
            coremap->flag = 0x1;//fixed
            
        }
        else{
            
            
            coremap->used =0;
            coremap->flag =0;
            
            
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
    time_t secs;
    u_int32_t nano;
    unsigned long count_pages;
    //kprintf("getppages: about to count coremap\n");
    for(i = 0; i< coremap_size; i++){
        j = i - npages + 1;
        if(!(coremap[i].used)){
            
            count_pages++;
            
            if(count_pages == npages){
                gettime(&secs,&nano);
            	coremap[j].len = npages;
                coremap[j].secs = secs;
                coremap[j].nano = nano;
                
                //assert(curthread != NULL);
                //assert(curthread->t_process != NULL);
                if(curthread != NULL && curthread->t_process !=NULL){
                coremap[j].pid = curthread->t_process->PID;
                }
                break;
            }
        }
        else{
            count_pages = 0;
        }
        
        
    }
    
    if(count_pages == npages){
        // int j;
       // kprintf("coremap: j: %d secs: %ld nano: %ld\n",j,(long)coremap[j].secs,(long )coremap[j].nano);
        
        for(j =i - npages +1;j<(i+1);j++){
            //coremap[j].pid = curthread->t_process->PID;
            coremap[j].used= 1;
            
            
        }

        assert((coremap[i-npages+1].paddr & PAGE_FRAME) == coremap[i-npages+1].paddr);//make sure the address is in the frame
        
        lock_release(core_lock);

        return coremap[i-npages+1].paddr;
        
    }
    
    /*
     
     If we reached here, then it means we cannot find contigous block, i.e we need to swap stuff
     */
    
    /*
    paddr_t pa;
    pa = page_algorithmn();
    return pa;
    */
    
    
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


static
int
load_each_segment(struct vnode *v, off_t offset, vaddr_t vaddr, paddr_t paddr, 
	     size_t memsize, size_t filesize,
	     int is_executable, int first_read)
{
	struct uio u;
	int result;
	size_t fillamt;
        int spl;

	if (filesize > memsize) {
		kprintf("ELF: warning: segment filesize > segment memsize\n");
		filesize = memsize;
	}
        
	DEBUG(DB_EXEC, "ELF: Loading %lu bytes to 0x%lx\n", 
	      (unsigned long) filesize, (unsigned long) vaddr);

	//u.uio_iovec.iov_ubase = (userptr_t)vaddr;
        if(first_read == 0){
            
            u.uio_iovec.iov_ubase = (userptr_t)vaddr;
            u.uio_iovec.iov_len = memsize;   // length of the memory space
            u.uio_resid = filesize;          // amount to actually read
            u.uio_offset = offset;
            u.uio_segflg = is_executable ? UIO_USERISPACE : UIO_USERSPACE;
            u.uio_rw = UIO_READ;
            u.uio_space = curthread->t_vmspace;

        }else{
            
            return 0;
        }
	
        result = VOP_READ(v, &u);
        //lock_acquire(tlb.tlb_lock);
	if (result) {
        
		return result;
	}
	if (u.uio_resid != 0) {
		/* short read; problem with executable? */
		kprintf("ELF: short read on segment - file truncated?\n");
		return ENOEXEC;
	}

	 /*Fill the rest of the memory space (if any) with zeros */
	fillamt = memsize - filesize;
	if (fillamt > 0) {
		DEBUG(DB_EXEC, "ELF: Zero-filling %lu more bytes\n", 
		      (unsigned long) fillamt);
		u.uio_resid += fillamt;
		result = uiomovezeros(fillamt, &u);
	}
	return result;
}

int first_read = 0;
int second_write = 0;
vaddr_t first_v = 0;

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	int i;
	u_int32_t ehi, elo;
	struct addrspace *as = curthread->t_vmspace;
	int spl;
        int result;
        int probe;
 
        if (first_v != faultaddress)
        {
            first_v = faultaddress;
            first_read = 0;
        } else {
            first_read = 1;
        }
        
    int p_i;
    
	spl = splhigh();
    
    
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
    
	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;
    
    
    //---Yi: need to find segment, then page. NOTE: THE INDEX MAY BE OFF BY +-1 because of the =
    
    p_i = faultaddress/PAGE_SIZE;
    
    int segment; // -1invalid 0code 1data 2stack
    size_t seg_size;
    size_t f_size;
    off_t offset;
    int flags;
    
    struct page *pg;
    if (p_i < 0)    {
        panic("addrspace: invalid page table index\n"); // need exception handling
        
    } else if (faultaddress >= vbase1 && faultaddress < vtop1)   { // look in code
        pg = (struct page *)array_getguy(as->useg1, (faultaddress-vbase1)/PAGE_SIZE);
        segment=0;
        seg_size=as->as_npages1;
        f_size = as->filesz1;
        offset = as->off_1;
        flags=as->flag1;
    } else if (faultaddress >= vbase2 && faultaddress < vtop2)    { // look in data
        pg = (struct page *)array_getguy(as->useg2, (faultaddress-vbase2)/PAGE_SIZE);
        segment =1;
        seg_size=as->as_npages2;
        f_size = as->filesz2;
        offset = as->off_2;
        flags=as->flag2;
        first_load = 0;
        //second_write = 1;
    } else if (faultaddress >= stackbase && faultaddress < stacktop) { // look in stack
        pg = (struct page *)array_getguy(as->usegs, ((faultaddress - stackbase)/PAGE_SIZE));
        segment = 2;
        seg_size=DUMBVM_STACKPAGES;
        f_size = as->filesz3;
        first_load = 0;
        flags=RWE;
    } else {
        segment = -1;
        return EFAULT;
    }
        
    //---
   
    int wr_to = 0;
    int err; // useless for now

    //kprintf("Fault %d, Permission %x, Addr %x\n", faulttype,pg->permission,faultaddress );
	switch (faulttype) {
	    case VM_FAULT_READONLY:
                
                    
                    err = EFAULT;
                    _exit(0, &err);   
                
                
		/* We always create pages read-write, so we can't get this */
	    case VM_FAULT_READ: // need to add a new page
                if (!pg->valid)
                {
                    pg->valid = 1;
                    pg->vaddr = faultaddress;
                    
                    if(f_size != 0) {
                        paddr = getppages(seg_size);
                        if (paddr == NULL)
                        {kprintf("memsize\n");
                            return ENOMEM;

                        }
                        
                        pg->paddr = paddr;
                        if (segment != 2){
                        //for (i = 0; i < seg_size; i++) {
                            splx(spl);
                                result = load_each_segment(as->v, offset, faultaddress, paddr, seg_size*PAGE_SIZE, f_size, flags & E_ONLY, 0);
                            spl = splhigh();

                            if (result) {kprintf("load\n");
                                    return result;
                            }
                        //}
                        }
                    
                    } else {
                        paddr = getppages(1);
                        if (paddr == NULL)
                        {
                            return ENOMEM;

                        }
                    }             
                    
                             pg->paddr = paddr;               
                    _vmstats_inc(VMSTAT_PAGE_FAULT_ZERO); /* STATS */

                }
                else
                {
                    wr_to = 1;
                    paddr = pg->paddr;
                    
                    _vmstats_inc(VMSTAT_TLB_RELOAD); /* STATS */
                }
                
                
            break;
            
	    case VM_FAULT_WRITE:
                
                //if (segment == 0 && first_read == 0)
                        wr_to = 1;
                
                if (!pg->valid)
                {
                    pg->valid = 1;
                    pg->vaddr = faultaddress;
                    
                    
                    if(f_size != 0) {
                        
                        if (second_write == 0 && segment == 1){
                            first_read = 0;
                            paddr = getppages(seg_size);
                            second_write = 1;
                            wr_to = 0;
                        } else {
                            first_read = 1;
                            paddr = getppages(1);
                        }
                        
                        //if (segment == 0 || segment == 1)
                            
                       // else 
                         //   paddr = getppages(seg_size);
                        
                        if (paddr == NULL)
                        {
                            return ENOMEM;

                        }
                        pg->paddr = paddr;
                        //paddr = pg->paddr;
                        if (segment != 2){
                            //for (i = 0; i < seg_size; i++) {
                                splx(spl);
    //kprintf("before\n");
                                result = load_each_segment(as->v, offset, faultaddress, paddr, seg_size*PAGE_SIZE, f_size, flags & E_ONLY, first_read);
                                spl = splhigh();
                                //kprintf("after\n");
                                if (result) {
                                    lock_release(tlb.tlb_lock);
                                       return result;
                                }
                            //}
                        }
                    } else {
                        paddr = getppages(1);
                        if (paddr == NULL)
                        {
                            return ENOMEM;

                        }
                    }
                    pg->paddr = paddr;


                    _vmstats_inc(VMSTAT_PAGE_FAULT_ZERO); /* STATS */
                }
                else
                {
                    paddr = pg->paddr;
                    //kprintf("paddr %x\n", paddr);
                    _vmstats_inc(VMSTAT_TLB_RELOAD); /* STATS */
                      
                }
		break;
	    default:
		return EINVAL;
	}

	_vmstats_inc(VMSTAT_TLB_FAULT);
        splx(spl);

if (wr_to == 1 || (segment == 2)){
    
        lock_acquire(tlb.tlb_lock);
        
            for (i=0; i<NUM_TLB; i++) {
                
                    TLB_Read(&ehi, &elo, i);
                    if (elo & TLBLO_VALID) {
                            continue;
                    }
                    ehi = faultaddress;

                    if (count_dup == 1 && faultaddress >= vbase1 && faultaddress < vtop1) {
                        elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
                        //first_load = 0;
                    }
                    else {
                        if (faultaddress >= vbase1 && faultaddress < vtop1){
                             elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
                        }else{
                            elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
                        }
                    }

                    DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
                    TLB_Write(ehi, elo, i);
                    
                    //splx(spl);
            lock_release(tlb.tlb_lock);
            vmstats_inc(VMSTAT_TLB_FAULT_FREE); /* STATS */
                    return 0;
            }

            int victim = tlb_get_rr_victim();
            //kprintf("vm fault: got our victim, %d \n",victim);
            if (victim < 0 || victim >= NUM_TLB)
            {
                 lock_release(tlb.tlb_lock);
                return EFAULT;
            }

            ehi = faultaddress;
            
            if (first_load && wr_to && faultaddress >= vbase1 && faultaddress < vtop1) {
                elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
                //first_load = 0;
            }
            else {
                if (faultaddress >= vbase1 && faultaddress < vtop1){
                        elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
                }else{
                        elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
                }
            }

            TLB_Write(ehi, elo, victim);
            
            
            lock_release(tlb.tlb_lock);


            vmstats_inc(VMSTAT_TLB_FAULT_REPLACE); /* STATS *///
}
                return 0;

}
