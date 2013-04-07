#include <types.h>
#include <kern/unistd.h>
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
#define KVADDR_TO_PADDR(paddr) ((paddr)-MIPS_KSEG0)




void swap_initialize(){
    //Cannot create the necessary swap and paging stuff until VM systems is initialize
    kprintf("swap init\n");
    assert(pt_initialize);
    //swap_coresize = 9*1024*1024;
    char *sfile = NULL;
    sfile = kstrdup("lhd0raw:");
    //struct vnode* swapfile;
    //O_RDWR|O_CREAT|O_TRUNC
    kprintf("before vfs_open\n");
    int result = vfs_open(sfile,O_RDWR,&SWAPFILE);
    kprintf("after vfs openm\n");
    kfree(sfile);
    kprintf("freed\n");
    if (result != 0){
        kprintf("failed to open swapfile, return code: %d\n",result);
        return result;
        
    }
    struct stat finfo;
    kprintf("vop stat\n");
    assert(SWAPFILE != NULL);
    VOP_STAT(SWAPFILE,&finfo);
    kprintf("after vop stat\n");
    swap_coresize = finfo.st_size/PAGE_SIZE;
    kprintf("size of coreswap: %d\n",swap_coresize);
    swap_map = bitmap_create(swap_coresize);
    assert(swap_map != NULL);
    int i;
    swap_coremap = (struct coremap *)kmalloc(swap_coresize * sizeof(struct coremap));
    for(i = 0;i < swap_coresize; i++){
        swap_coremap[i].paddr = i*PAGE_SIZE;
        //kprintf("i: %d paddr: %p\n",i,swap_coremap[i].paddr);
    }
    
    //create the swapfiles
    
    
    kprintf("swap stuff ready\n");
    
}
void coremap_insert(vaddr_t va,paddr_t paddr){
    kprintf("coremap_insert: vaddr %p, paddr %p\n",va,paddr);
    
    
    assert(pt_initialize);
    int index = coremap_find(paddr);
    if(index>=0 && index<coremap_size){
        coremap[index].vaddr = va;
        
    }
    /*
   if(paddr == 0){
    for(index = 0;index<coremap_size;index++){
    
    	kprintf("index: %d, paddr: %p, vaddr %p, used: %d, len: %d\n",index,coremap[index].paddr,coremap[index].vaddr,coremap[index].used,coremap[index].len);
    
    }
     
    }
    */
    
}


void swapin(paddr_t candidate, paddr_t swapee){
	kprintf("swapin call\n");
	struct uio u_swap;
	int offset = swapee % PAGE_SIZE;
	mk_kuio(&u_swap, (void *)PADDR_TO_KVADDR(candidate),PAGE_SIZE,offset,UIO_READ);
	swap_remove(swapee);
    kprintf("swapin: before vop_read\n");
    assert(SWAPFILE != NULL);
	int result = VOP_READ(SWAPFILE, &u_swap);
	if(result){
		panic("Failed swapin\n");
	}
	
}

void swapout(paddr_t candidate,paddr_t swapee){
	
	kprintf("swapout call\n");
	struct uio u_swap;
	int offset = swapee % PAGE_SIZE;
    kprintf("before mk_uio\n");
	mk_kuio(&u_swap, (void *)PADDR_TO_KVADDR(candidate),PAGE_SIZE,offset,UIO_WRITE);
    kprintf("before swap_insert\n");
	swap_insert(swapee);
    kprintf("after swap insert\n");
    assert(SWAPFILE != NULL);
	int result = VOP_WRITE(SWAPFILE, &u_swap);
    kprintf("vop write suceed\n");
	if(result){
		panic("Failed swapout\n");
	}
	
}



/*
 Used to modify the swamp
 */
void swap_remove(paddr_t pa){
    kprintf("swap_remove call\n");
	int i = coremap_find(pa);
    kprintf("swapremove: i is %d\n",i);
	/*if(!(i >=0 && i < coremap_size)){
        
        bitmap_unmark(swap_map,i);
        
    }*/
	
    kprintf("swapremove complete\n");
	//swap_coremap[i].pid = -1;
	//swap_coremap[i].used = 0;
	
    
}
void swap_insert(paddr_t paddr){
    
	int i = coremap_find(paddr);
	assert(i >=0);
	
	//swap_coremap[i].pid = curthread->t_process->PID;
	//swap_coremap[i].used = 1;
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
    //kprintf("coremap_find:trying to find: %p\n", paddr);
    int i;
    for(i =0;i<coremap_size;i++){
        
        if(coremap[i].paddr == paddr){
           // kprintf("coremap_find: found paddr\n");
            return i;  
        }
       // kprintf(".");
    }
    if(i>= coremap_size){
        
        kprintf("coremap_find: Physical Address: %p given does not exists\n",paddr);
        return -1;
        
    }
    
    return 0;
    
}
/*

Get the paddr of the first in page

*/
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
    fifo = -1;
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
    if(fifo ==-1){
    
      return (paddr_t)-1;
    }
    
    assert(fifo<coremap_size && fifo>0);
    //fifo = random()%coremap_size;
    return coremap[fifo].paddr;
}
/*
Finds the index of a free page

*/
int get_index_free(){
	int spl,index;
	//spl = splhigh();
    kprintf("get index free: before btmap alloc\n");
    assert(swap_map != NULL);
	int rc = bitmap_alloc(swap_map,&index);
    rc = bitmap_alloc(swap_map,&index);
    kprintf("get_index_free: %d\n",index);
	if(rc){
		//splx(spl);
		kprintf("VM: Out of Swap space\n");
		int rc;
		_exit(0,&rc);	
	}
	splx(spl);
	return index;

}
/*



*/
paddr_t get_free_page() {
	int spl=splhigh();
	unsigned index;
	int result = bitmap_alloc(core_map, &index);
	if(result) {
		paddr_t paddr = get_fifo_page();
	   if(paddr != -1){
			splx(spl);
			int i = get_index_free();
			swapout(coremap[i].paddr, paddr);
			//mark entry into swap_table
           kprintf("get free page complete, paddr is %p\n",paddr);
			return paddr;
	   }
	
	}
	//add a check for ISVALID
	splx(spl);
	return (coremap[index].paddr);
}


/*
 Page Algorithmn: Fifo
 If this function is called then, we need to swap pages and stuff
 We should still be in the critical section when this function is called 
 so no need to acquire the lock
 */
paddr_t page_algorithmn(int index){
    kprintf("call to page_algorithmn with index %d\n",index);
    paddr_t pa,candidate;
    //pa = get_fifo_page();
    int i = get_index_free();
    kprintf("page_algo: return from get_index_free with i: %d\n",i);
    assert(coremap != NULL && swap_coremap != NULL);
    	pa = coremap[index].paddr;
    	candidate = swap_coremap[i].paddr;
    	//swapout
    kprintf("swapout in page_Algorithmn\n");
    	swapout(candidate,pa);
    	//remove from coremap
    	coremap[index].used = 0;
    	coremap[index].nano = 0;
    	coremap[index].secs =0;
    
    
    return candidate;
}


paddr_t fault_handler(vaddr_t va){
    lock_acquire(core_lock);
	//kprintf("fault_handler call, fault address: %p\n",va);
	paddr_t pa;
	int i;
	for(i = 0;i<coremap_size;i++){
		
		if(coremap[i].vaddr == va){
           // kprintf("fault_handle: paddr: %p, vaddr %p,used: %d, len: %d\n ",coremap[i].paddr,coremap[i].vaddr,coremap[i].used,coremap[i].len);
			break;
		}
	
	}
	
	if(i>=coremap_size){
		//call to page_fault_handle
		pa = pagefault_handler(pa);
	}
	/*for(i = 0;i<coremap_size;i++){
		
		if(coremap[i].paddr == pa){
			break;
		}
	
	}*/
	/*
	We should always get a valid index i
	*/
    //kprintf("fault handler: i is %d\n ",i);
	assert(i<coremap_size);
	time_t secs;
	u_int32_t nano;
	gettime(&secs,&nano);
	coremap[i].secs = secs;
	coremap[i].nano = nano;
	coremap[i].vaddr = va;
    lock_release(core_lock);
	return coremap[i].paddr;

}

paddr_t pagefault_handler(paddr_t pa){
    kprintf("pagefault_handler call\n");
	paddr_t paddr = get_free_page();
    kprintf("pagefault_handler: paddr %p \n",paddr);
	swapin(paddr,pa);
	return paddr;	
}

