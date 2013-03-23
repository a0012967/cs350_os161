#ifndef _PT_H_
#define _PT_H_

//#include <vm.h>
#include <types.h>

#include "opt-A3.h"



#if OPT_A3
enum page_state {
   
   free,
   fixed,
   clean,
   dirty
   
};


/*
*/
/*
To free a page, you must know which virtual address it points to, which is accomplished by storing virtual address in page
*/



struct page{

/* Important stuff:
Two ways to handle this, map page tables using paddr, or map using vaddr

*/
   enum page_state state; 

    struct addrspace* as;
   vaddr_t vaddr;
    paddr_t paddr;
    pid_t pid; 
    int lenblock; // length of the block
  // uint64_t timestamp;
   
};

struct lock* table_lock; // a lock to protect the page table


int page_size;
int pt_initialize;

struct page* pagetable;




#endif 


#endif
