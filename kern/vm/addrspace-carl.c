#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <synch.h>
#include <addrspace.h>
#include <vm.h>
#include <pt.h>
#include "opt-A3.h"


#if OPT_A3
//table_lock = lock_create("table_lock");

/*
 Does most of the work for alloc
 */
static
paddr_t
getppages(unsigned long npages){
    
#if OPT_A3
    //lock_acquire(table_lock);
    if(pt_initialize != 1){
        
        panic("PAGE TABLE NOT INITIALIZE\N");
        
    }
    
    int i;
    unsigned long count_pages;
    for(i = 0; i< coremap_size; i++){
        
        if(coremap[i].valid == 1 && coremap[i].used == 0){
            
            count_pages++;
            if(count_pages == npages){
                i++;
                break;
            }
        }
        else{
            
            count_pages = 0;
        }
        
        
    }
    
    if(count_pages == npages){
        int j;
        for(j =i - npages +1;j<coremap_size;j++){
            
            coremap[j].used= 1;
            
            
        }
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
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
    
    
}


void 
free_kpages(vaddr_t addr)
{
    int i =0;
    while(coremap[i].vaddr != addr){
        
        i++;
        
    }
    
    int len =coremap[i].lenblock;
    for(; i < len;i++){
        
        coremap[i].used = 1;
        coremap[i].lenblock = -1;
        
    }
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	return -1;
}


#endif

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
    
	(void)as;
	(void)vaddr;
	(void)sz;
	(void)readable;
	(void)writeable;
	(void)executable;
	return EUNIMP;
}

int
as_prepare_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */
    
	(void)as;
	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */
    
	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/*
	 * Write this.
	 */
    
	(void)as;
    
	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;
	
	return 0;
}

