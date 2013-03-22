#include <pt.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>
#include <elf.h>
#include <syscall.h>
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <thread.h>




void vm_bootstrap(){

   struct page *entry; //at the end should point to the same memory as pagetable in pt.h

//allocating memory

   paddr_t firstaddr,lastaddr,freeadr;
   ram_getsize(&firstaddr,&lastaddr);
   //page size defined in vm.h
   int page_size = lastaddr-firstaddr/PAGE_SIZE; //addr alignment, nOTE: This rounds out to a whole number
   
   pagetable = (struct page *)PADDR_TO_KVADDR(firstaddr); //sets the page array
   if(pagetable == NULL){
      panic("Can't create page table, no mem\n");
   }
   freeaddr = firstaddr + page_size * sizeof(page);
   if(lastaddr-freeaddr == 0){
   
      panic("OUT OF MEMORYn");
   }
   // freee addr to lastaddr is the systems main memory
//the actual init

p = (struct page *) PADDR_TO_KVADDR((paddr_t)pagetable);
   int i;
   for(i =0;i<(freeaddr-firstaddr)/PAGE_SIZE;i++){
      p->state = fixed; //fixed
      p->vaddr = PADDR_TO_KVADDR(firstaddr)+(i* page_size);
      //p->as   //need address space functions
      //p->PID = curthread
      p+= sizeof(struct page);
   
   }
   for(;i<page_size;i++){
      p->state = free; //marked as free
       p->vaddr = PADDR_TO_KVADDR(firstaddr)+(i* page_size);
      //p->as   //need address space functions
      //p->PID = curthread
      pagetable[i] = p;
      p+= sizeof(struct page);
   
   
   }

   pt_initialize =1;

}
