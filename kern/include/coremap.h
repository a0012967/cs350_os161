#ifndef _COREMAP_H_
#define _COREMAP_H_






struct coremap{
    
    /* Important stuff:
     Two ways to handle this, map page tables using paddr, or map using vaddr
     
     */
    //enum page_state state; 
    //struct addrspace* as;
    //vaddr_t vaddr;
    paddr_t paddr;
    //int valid;
    int used;
    int len; // length of the block
    pid_t pid;
    // uint64_t timestamp;
    
};

struct lock* core_lock; // a lock to protect the page table


static int coremap_size;
static int pt_initialize = 0;

static struct coremap* coremap;

/*
 
 Swap stuff
 
 */
struct vnode *vswap;
static struct coremap* swap_coremap;
static int swap_init = 0;
static int swap_coresize;

#endif
