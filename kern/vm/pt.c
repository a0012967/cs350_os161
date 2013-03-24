#include <pt.h>
#include <addrspace.h>
#include <vm.h>
#include <elf.h>
#include <syscall.h>
#include <types.h>
#include <kern/errno.h>
#include <lib.h>

#include <thread.h>








/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground. You should replace all of this
 * code while doing the VM assignment. In fact, starting in that
 * assignment, this file is not included in your kernel!
 */

/* under dumbvm, always have 48k of user stack */
#define DUMBVM_STACKPAGES    12



/* Allocate/free some kernel-space virtual pages */

/*

void vm_bootstrap(){
    
    struct page *entry; //at the end should point to the same memory as pagetable in pt.h
    coremap_size =0;
    //allocating memory
    
    paddr_t firstaddr,lastaddr,freeaddr;
    ram_getsize(&firstaddr,&lastaddr);
    //page size defined in vm.h
    coremap = lastaddr-firstaddr/PAGE_SIZE; //addr alignment, nOTE: This rounds out to a whole number
    
    coremap = (struct coremap *)PADDR_TO_KVADDR(firstaddr); //sets the page array
    if(coremap== NULL){
        panic("Can't create page table, no mem\n");
    }
    freeaddr = firstaddr + coremap_size * sizeof(struct coremap);
    if(lastaddr-freeaddr == 0){
        
        panic("OUT OF MEMORYn");
    }
    // freee addr to lastaddr is the systems main memory
    //the actual init
    
    struct coremap * p = (struct coremap *) PADDR_TO_KVADDR((paddr_t)coremap);
    int i;
    for(i =0;i<(freeaddr-firstaddr)/PAGE_SIZE;i++){
        //p->state = fixed; //fixed
        p->valid = 0;
        p->used = 1;
        p->paddr = firstaddr+(i*coremap_size);
        p->lenblock = -1; //initially -1
        // p->pid = -1; //indicate no process has this yet
        //p->vaddr = PADDR_TO_KVADDR(firstaddr)+(i* page_size);
        coremap[i] = p[i];
        //  coremap_size++;
        p+= sizeof(struct coremap);
        
    }
    for(;i<coremap_size;i++){
        // p->state = free; //marked as free
        
        p->valid = 1;
        p->used = 0;
        p->paddr = firstaddr+(i*coremap_size);
        p->lenblock = -1; //initially -1
        // p->pid = -1; //indicate no process has this yet
        p->vaddr = PADDR_TO_KVADDR(firstaddr)+(i* coremap_size);
        coremap[i] = p[i];
        // coremap_size++;
        p+= sizeof(struct coremap);
        
        
    }
    
    pt_initialize =1;
    kprintf("VM BOOTSTRAP COMPLETE: page size: %d, coremap size:%d\n",PAGE_SIZE,coremap_size);
    
}*/
