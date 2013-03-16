

#ifndef _SYSCALL_H_
#define _SYSCALL_H_
#include "opt-A2.h"
/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);

#if OPT_A2
int sys_open(userptr_t filename, int flags, int mode, int * retval);
int sys_close(int fd);
int write(int fd, const void *buf, size_t nbytes,int *retval);
int read(int fd, void *buf, size_t nbytes, int *retval);
void _exit(int exitcode,int *retval);
int execv(char *progname, char** argv_o);
#endif /* _OPT_A2_ */

#endif /* _SYSCALL_H_ */
