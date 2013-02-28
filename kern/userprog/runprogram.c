/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <test.h>
#include "opt-A2.h"

/*
 * Calculates the padding length needed for a particular
 * string. Total length should be a multiple of 4
 */
static unsigned int 
calc_align_length(char *argv)
{
    unsigned int argvlen = strlen(argv)+1; // length of actual string including '\0'
    unsigned int paddinglen = 4-(argvlen%4);    // mod 4
    
    return argvlen+paddinglen; // total correct alignment length(multiple of 4)
}

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
runprogram(char *progname, char** argv, int argc)
{
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, &v);
	if (result) {
		return result;
	}

	/* We should be a new thread. */
	assert(curthread->t_vmspace == NULL);

	/* Create a new address space. */
	curthread->t_vmspace = as_create();
	if (curthread->t_vmspace==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Activate it. */
	as_activate(curthread->t_vmspace);

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		return result;
	}
#if OPT_A2
        vaddr_t initialptr = stackptr;
        unsigned int i;
        unsigned int j; // i reversed index
        int err;
        
        unsigned int alignlen; // length of argument with correct mod4 alignment
        
        if (argc > 1)
        {
            for (i = 0; i < argc; i++) // copyout the array values
            {
                j = argc-(i+1);

                alignlen = calc_align_length(argv[j]);

                stackptr = stackptr-alignlen;

                if ((err = copyoutstr(argv[j], stackptr, strlen(argv[j])+1, &alignlen)) != 0)
                    kprintf("ERROR copyoutstr %d\n", err);

                argv[j] = stackptr; // fill argv with actual user space ptr
            }

            for (i = 0; i <= argc; i++) // copyout the array addresses
            {
                j = argc-i;
                stackptr = stackptr-4;

                if ((err = copyout(&argv[j], stackptr, 4)) != 0)
                    kprintf("ERROR copyout %d\n", err);

            }

            /* Warp to user mode. */
            md_usermode(argc /*argc*/, stackptr /*userspace addr of argv*/,
                        stackptr, entrypoint);
        }
        else
        {
            /* Warp to user mode. */
            md_usermode(0 /*argc*/, NULL /*userspace addr of argv*/,
                        stackptr, entrypoint);
        }
	
	/* md_usermode does not return */
	panic("md_usermode returned\n");

    

#else
	md_usermode(0 /*argc*/, NULL /*userspace addr of argv*/,stackptr, entrypoint);
	
	/* md_usermode does not return */
#endif
    panic("md_usermode returned\n");

	return EINVAL;
}

