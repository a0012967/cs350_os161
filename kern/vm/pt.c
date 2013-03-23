#include <pt.h>
#include <addrspace.h>
#include <vm.h>
#include <elf.h>
#include <syscall.h>
#include <types.h>
#include <kern/errno.h>
#include <lib.h>

#include <thread.h>








/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground. You should replace all of this
 * code while doing the VM assignment. In fact, starting in that
 * assignment, this file is not included in your kernel!
 */

/* under dumbvm, always have 48k of user stack */
#define DUMBVM_STACKPAGES    12


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

/* Allocate/free some kernel-space virtual pages */
/*
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
*/
/*
void 
free_kpages(vaddr_t addr)
{
	/* nothing */
    /*
	(void)addr;
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	return -1;
}
*/
void vm_bootstrap(){

   struct page *entry; //at the end should point to the same memory as pagetable in pt.h

//allocating memory

   paddr_t firstaddr,lastaddr,freeaddr;
   ram_getsize(&firstaddr,&lastaddr);
   //page size defined in vm.h
   int page_size = lastaddr-firstaddr/PAGE_SIZE; //addr alignment, nOTE: This rounds out to a whole number
   
   pagetable = (struct page *)PADDR_TO_KVADDR(firstaddr); //sets the page array
   if(pagetable == NULL){
      panic("Can't create page table, no mem\n");
   }
   freeaddr = firstaddr + page_size * sizeof(page_size);
   if(lastaddr-freeaddr == 0){
   
      panic("OUT OF MEMORYn");
   }
   // freee addr to lastaddr is the systems main memory
//the actual init

struct page * p = (struct page *) PADDR_TO_KVADDR((paddr_t)pagetable);
   int i;
   for(i =0;i<(freeaddr-firstaddr)/PAGE_SIZE;i++){
      p->state = fixed; //fixed
      p->vaddr = PADDR_TO_KVADDR(firstaddr)+(i* page_size);
      //p->as   //need address space functions
      //p->PID = curthread
       pagetable[i] = p[i];
      p+= sizeof(struct page);
   
   }
   for(;i<page_size;i++){
      p->state = free; //marked as free
       p->vaddr = PADDR_TO_KVADDR(firstaddr)+(i* page_size);
      //p->as   //need address space functions
      //p->PID = curthread
      pagetable[i] = p[i];
      p+= sizeof(struct page);
   
   
   }

   pt_initialize =1;

}
