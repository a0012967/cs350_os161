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


static int coremap_size;
static int pt_initialize = 0;
struct bitmap* core_map; //determines which pages are free and which are not
static struct coremap* coremap;

/*
 
 Swap stuff
 
 */
struct lock *swap_lock;
struct vnode *vswap;
static struct coremap* swap_coremap;
static int swap_init = 0;
static int swap_coresize;
static struct bitmap* swap_map;

void swap_initialize();
void swapin(paddr_t paddr);
void swapout(paddr_t paddr);
void check_swap(paddr_t paddr);
paddr_t pagefault_handler(paddr_t paddr);

#endif
