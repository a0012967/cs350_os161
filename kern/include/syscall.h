#include "opt-A2.h"

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);


#if OPT_A2
int sys_open(userptr_t filename, int flags, int mode, int *retval);
int sys_close(int fd);

void _exit(int exitcode);
#endif /* _OPT_A2_ */

#endif /* _SYSCALL_H_ */
