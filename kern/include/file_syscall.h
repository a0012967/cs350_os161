#ifndef _file_syscall_H_
#define _file_syscall_H_

#include "opt-A2.h"


#if OPT_A2
int sys_open(userptr_t filename, int flags, int mode, int *retval);
int sys_close(int fd);
int write(int fd, const void *buf, size_t nbytes, int *retval);
int read(int fd, void *buf, size_t nbytes,int *err);
void _exit(int exitcode,int *retval);
#endif /* _OPT_A2_ */

#endif //define
