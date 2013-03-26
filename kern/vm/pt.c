#include <pt.h>
#include <addrspace.h>
#include <vm.h>
#include <elf.h>
#include <syscall.h>
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <coremap.h>
#include <thread.h>








/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground. You should replace all of this
 * code while doing the VM assignment. In fact, starting in that
 * assignment, this file is not included in your kernel!
 */

/* under dumbvm, always have 48k of user stack */
#define DUMBVM_STACKPAGES    12

int pagetable_create()