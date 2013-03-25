#ifndef _COREMAP_H_
#define _COREMAP_H_


struct coremap{
    
    /* Important stuff:
     Two ways to handle this, map page tables using paddr, or map using vaddr
     
     */
    //enum page_state state; 
    //struct addrspace* as;
    vaddr_t vaddr;
    paddr_t paddr;
    int valid;
    int used;
    int lenblock; // length of the block
    // uint64_t timestamp;
    
};

struct lock* table_lock; // a lock to protect the page table


int coremap_size;
int pt_initialize;

struct coremap* coremap;

#endif
