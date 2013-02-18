#ifndef _SYSCALL_H_
#define _SYSCALL_H_

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);
int write(int fd, const void *buf, size_t nbytes);


#endif /* _SYSCALL_H_ */
