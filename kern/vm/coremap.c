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


#if opt_A3


void swap_initialize(){
    
    assert(pt_initialize);
    struct stat finfo;
    VOP_STAT(vswap,&finfo);
    
    swap_coresize = finfo.stsize/PAGE_SIZE;
    kprintf("size of coreswap: %d",swap_coresize;);
    swap_map = bitmap_create(swap_coresize);
    assert(swap_map != NULL);
    int i;
    for(i = 0;i < coremap_size; i++){
        swap_coremap[i].paddr = coremap[i].paddr;
        swap_coremap[i].used = coremap[i].used;
        swap_coremap[i].len = coremap[i].len;
        swap_coremap[i].pid = coremap[i].pid;
    }
    
    kprintf("swap stuff ready\n");
    
}

/*
 Used to modify the swamp
 */
void swap_remove(paddr_t paddr){
    
	int i = coremap_find(pa);
	assert(i >=0);
	
	swap_coremap[i].pid = -1;
	swap_coremap[i].used = 0;
	bitmap_unmark(swap_map,i);
    
}
void swap_insert(paddr_t paddr){
    
	int i = coremap_find(pa);
	assert(i >=0);
	
	swap_coremap[i].pid = curthread->pid;
	swap_coremap[i].used = 1;
	if(!bitmap_isset(swap_map,i)){
        
        bitmap_mark(swap_map,i);
		
	}
	bitmap_unmark(swap_map,i);
    
}

/*
 
 Finds the index of a given paddr
 
 */
int coremap_find(paddr_t pa){
    
    assert(pt_initialize);
    int i;
    for(i =0;i<coremap_size;i++){
        
        if(coremap[i].paddr == paddr){
            return i;  
        }
    }
    if(i>= coremap_size){
        
        kprintf("coremap_find: Physical Address given does not exists\n");
        return -1;
        
    }
    
}

paddr_t coremap_freepage(){
    
    assert(pt_initialize);
    int i;
    for(i =0;i<coremap_size;i++){
        
        if(!(coremap[i].used)){
            return coremap[i].paddr;  
        }
    }
    if(i>= coremap_size){
        
        kprintf("coremap_find: Physical Address given does not exists\n");
        return -1;
        
    }
    
    
}

/*
 
 Finds the index of a given paddr
 
 */
int coreswap_find(paddr_t pa){
    
    assert(pt_initialize);
    int i;
    for(i =0;i<coremap_size;i++){
        
        if(swap_coremap[i].paddr == paddr){
            return i;  
        }
    }
    if(i>= coremap_size){
        
        kprintf("coremap_find: Physical Address given does not exists\n");
        return -1;
        
    }
    
}

/*
 
 We use fifo to replace the frame that has been longest
 */

paddr_t get_fifo_page(){
    
    
	//paddr pa;
	time_t secs;
	uint32_t nano;
	gettime(&secs,&nano);
    time_t max_secs = secs;
    uint32_t max_nano = nano;
    int i,fifo;
    for(i=0;i<coremap_size;i++){
        
        //find the one who has been longest
        
        if(coremap[i].secs >= max_secs && coremap[i].nano >= max_nano){
            
            max_secs = coremap[i].secs;
            max_nano = coremap[i].nano;
            fifo = i;
            
        }
        
    }
    
    assert(i<coremap_size);
    
    return coremap[i].paddr;
    
    
}
/*
 
 used for page_fault
 
 */
paddr_t get_page(){
	
	paddr_t pa;
	pa = get_fifo_page();
    int i = get_free_page();
    assert(i >=0);
    paddr_t swapee = coremap[i].paddr
	swap_out(pa,swapee);
    
}

paddr_t alloc_onepage(){
    /*int i = get_free_page();
    assert(i >=0);
     */
    paddr_t pa = getppages(1);
    
    if(pa <= 0){
        return 0;
        
    }
    return pa;
  
    
    
}

int get_free_page(){
    
    int i;
    for(i=0;i<coremap_size;i++){
        
        if(!(coremap[i].used)){
            return i;
        }
        
        
    }
    //error
    return -1;
}

void coreswap_remove (paddr_t paddr) {
    
	int spl=splhigh();
	swap_coremap[i].vaddr = 0;
	swap_coremap[i].counter = 0;
	swap_coremap[i].pid = 0;	
    
	splx(spl);
	return 0;
}

// we want to swap swapee with candidate
void swapin(paddr_t candidate, paddr_t swapee){
	
	struct uio u_swap;
	int offset = swapee % PAGE_SIZE;
	mk_uio(&u_swap, (void *)PADDR_TO_KVADDR(candidate),offset,UIO_READ);
	coreswap_remove(swapee);
	int result = VOP_READ(vswap, &u_swap);
	if(result){
		panic("Failed swapin\n");
	}
	
}

void swapout(paddr_t candidate,paddr_t swapee){
	
	
	struct uio u_swap;
	int offset = swapee % PAGE_SIZE;
	mk_uio(&u_swap, (void *)PADDR_TO_KVADDR(candidate),offset,UIO_WRITE);
	coreswap_insert(swapee);
	int result = VOP_WRITE(vswap, &u_swap);
	if(result){
		panic("Failed swapin\n");
	}
	
}
/*
 
 Page fault: We need to swap
 
 */
paddr_t pagefault_handler(paddr_t pa){
    //int index = get_free_page;
    paddr_t paddr = get_page();
    swapin(paddr,pa);
    int i = coremap_find(pa);
    assert(i != -1);
    coremap_insertpid(paddr,swap_coremap[i].pid);
    
    return paddr;
}




#endif



