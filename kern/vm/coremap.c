#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <machine/vm.h>
#include <addrspace.h>
#include <pt.h>
#include <coremap.h>
#include <vm.h>
#include <kern/stat.h>
#include <vnode.h>
#include <machine/spl.h>
#include <uio.h>
#include "opt-A3.h"

void swap_initialize(){
    
    assert(pt_initialize);
    struct stat finfo;
    VOP_STAT(vswap,&finfo);
    
    swap_coresize = finfo.st_size/PAGE_SIZE;
    kprintf("size of coreswap: %d\n",swap_coresize);
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



void swapin(paddr_t candidate, paddr_t swapee){
	
	struct uio u_swap;
	int offset = swapee % PAGE_SIZE;
	mk_kuio(&u_swap, (void *)PADDR_TO_KVADDR(candidate),PAGE_SIZE,offset,UIO_READ);
	swap_remove(swapee);
	int result = VOP_READ(vswap, &u_swap);
	if(result){
		panic("Failed swapin\n");
	}
	
}

void swapout(paddr_t candidate,paddr_t swapee){
	
	
	struct uio u_swap;
	int offset = swapee % PAGE_SIZE;
	mk_kuio(&u_swap, (void *)PADDR_TO_KVADDR(candidate),PAGE_SIZE,offset,UIO_WRITE);
	swap_insert(swapee);
	int result = VOP_WRITE(vswap, &u_swap);
	if(result){
		panic("Failed swapin\n");
	}
	
}



/*
 Used to modify the swamp
 */
void swap_remove(paddr_t pa){
    
	int i = coremap_find(pa);
	assert(i >=0);
	
	swap_coremap[i].pid = -1;
	swap_coremap[i].used = 0;
	bitmap_unmark(swap_map,i);
    
}
void swap_insert(paddr_t paddr){
    
	int i = coremap_find(paddr);
	assert(i >=0);
	
	swap_coremap[i].pid = curthread->t_process->PID;
	swap_coremap[i].used = 1;
	if(!bitmap_isset(swap_map,i)){
        
        bitmap_mark(swap_map,i);
		
	}
	bitmap_unmark(swap_map,i);
    
}

/*
 
 Finds the index of a given paddr
 
 */
int coremap_find(paddr_t paddr){
    
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
    
    return 0;
    
}
paddr_t get_fifo_page(){
    kprintf("get_fifopage begin\n");
    
	//paddr pa;
	time_t secs;
	u_int32_t nano;
	gettime(&secs,&nano);
    kprintf("get_fifopage: got the time\n");
    time_t max_secs = secs;
    u_int32_t max_nano = nano;
    int i,fifo;
    kprintf("coremap_size is %d\n",coremap_size);
    for(i=0;i<coremap_size;i++){
        //find the one who has been longest
        kprintf("time at i:%d is %lld, used: %d\n",i,(long long )coremap[i].secs,coremap[i].used);
        if(coremap[i].secs >= max_secs && coremap[i].nano >= max_nano){
            
            max_secs = coremap[i].secs;
            max_nano = coremap[i].nano;
            fifo = i;
            
        }
        
    }
    kprintf("get_fifopage: fifo is %d \n",fifo);
    assert(fifo<coremap_size && fifo>0);
    //fifo = random()%coremap_size;
    return coremap[fifo].paddr;
    
    
}

/*
 Page Algorithmn: Fifo
 If this function is called then, we need to swap pages and stuff
 We should still be in the critical section when this function is called 
 so no need to acquire the lock
 */
paddr_t page_algorithmn(){
    paddr_t pa;
    
    
    
    return pa;
}
