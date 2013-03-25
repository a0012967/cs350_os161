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

enum permission   {
    readable, writeable, executable  
};


/*
 */
/*
 To free a page, you must know which virtual address it points to, which is accomplished by storing virtual address in page
 */


struct page{
    
    
    enum page_state;
    vaddr_t vaddr;
    struct addrspace *as;
    enum permission;
    
};








#endif 


#endif
