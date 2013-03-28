#ifndef _PT_H_
#define _PT_H_

//#include <vm.h>
#include <types.h>
#include <addrspace.h>
#include "opt-A3.h"



#if OPT_A3
#ifndef N_PAGES
#define N_PAGES ((USERTOP-MIPS_KUSEG)/PAGE_SIZE)
#endif


enum permission   {
    readable, writeable, exec, rw, re,we,rwe  
};


/*
 */
/*
 To free a page, you must know which virtual address it points to, which is accomplished by storing virtual address in page
 */


struct page{
    
    vaddr_t vaddr;
    int valid;
    enum permission permission;
};




struct pagetable{
    struct page *pt[N_PAGES];
};



struct page* page_create();
int pagetable_create(struct addrspace *as);

int pagetable_destroy(struct addrspace *as);
#endif 


#endif
