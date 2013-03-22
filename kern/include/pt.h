#ifndef _PT_H_
#define _PT_H_

#include <vm.h>
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
   enum page_state state; //intilize it as free? or clean?
   //pid_t pid; //identity process which it belongs to
   //paddr_t paddr; //what our pages will be maped to, can use vaddr_t instead
    struct addrspace* as;
   vaddr_t vaddr;
   //paddr_t paddr // the physical addr it is mapped to
  // pid_t PID; //which frame does this belong to?
   //int used;
   //int kmem;
   //int usermem;
   //int max pages;
  /*
      The address space and virtual address identifier
      
      To evict a page: Look at the vaddr, and as and modify it so that you are no longer in memory
      
  
  */
  
   /*
   Page replacement stuff:
   */
  // uint64_t timestamp;
   
}

struct lock* table_lock; // a lock to protect the page table




int pt_initialize = 0;

static struct page* pagetable;


void init_pagetable(void);
void free_[agetable(vaddr_t v);
vaddr_t page_alloc(unsigned long num_pages);

//these unctions might not be necessary since there are already ones defined in vm.h
paddr_t get_pages(int npages);


//page replacement
void evict_page();




#endif 


#endif
