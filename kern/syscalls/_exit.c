#include <types.h>
#include <kern/unistd.h>
#include <machine/spl.h>
#include <thread.h>
#include <scheduler.h>
#include <syscall.h>
#include "opt-A2.h"

void _exit(int exitcode)
{
    (void)exitcode;
    kprintf("Shutting down.\n");
	
    //vfs_clearbootfs();
    //vfs_clearcurdir();
    //vfs_unmountall();

    splhigh();

    //scheduler_shutdown();
    thread_shutdown();
}