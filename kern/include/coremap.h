#ifndef _COREMAP_H_
#define _COREMAP_H_


#include <bitmap.h>
#include <types.h>
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <pt.h>
#include <coremap.h>
#include <machine/tlb.h>
#include <machine/spl.h>
#include <vm-tlb.h>
#include <syscall.h>
#include <uw-vmstats.h>
#include <clock.h>
#include "opt-A3.h"


struct coremap{
    
    /* Important stuff:
     Two ways to handle this, map page tables using paddr, or map using vaddr
     
     */
    //enum page_state state; 
    //struct addrspace* as;
    //vaddr_t vaddr;
    paddr_t paddr;
    //int swapped;
    int used;
    int len; // length of the block
    pid_t pid;
    //time stamp
    time_t secs;
    u_int32_t nano;
    
};

struct lock* core_lock; // a lock to protect the page table


int coremap_size;
int pt_initialize;
struct bitmap* core_map; //determines which pages are free and which are not
struct coremap* coremap;

/*
 
 Swap stuff
 
 */
 void coremap_insertpid(paddr_t pa,pid_t pid);
struct lock *swap_lock;
struct vnode *vswap;
struct coremap* swap_coremap;
int swap_init;
int swap_coresize;
static struct bitmap* swap_map;

void swap_initialize();
void swapin(paddr_t paddr,paddr_t pa2);
void swapout(paddr_t paddr,paddr_t pa2);
void check_swap(paddr_t paddr);
int coremap_find(paddr_t pa);
extern paddr_t get_page();
paddr_t pagefault_handler(paddr_t paddr);

#endif
