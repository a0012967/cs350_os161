#ifndef _PT_H_
#define _PT_H_

//#include <vm.h>
#include <types.h>

#include "opt-A3.h"



#if OPT_A3



enum permission   {
    readable, writeable, executable  
};


/*
 */
/*
 To free a page, you must know which virtual address it points to, which is accomplished by storing virtual address in page
 */


struct page{
    
    vaddr_t vaddr;
    struct addrspace *as;
    int valid;
    enum permission;
};








#endif 


#endif
