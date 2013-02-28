#include <types.h>
#include <synch.h>
#include <thread.h>
#include <lib.h>
#include <kern/errno.h>
#include <curthread.h>
#include <uio.h>
#include <vfs.h>
#include <kern/unistd.h>
//#include <vnode.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <syscall.h>
#include <fileDescriptor.h>
#include "opt-A2.h"

#if OPT_A2
int fd_table_create() {
    int result, fd;
    char path[5];
    struct fd_table *t = kmalloc(sizeof(struct fd_table));
    
    if (t == NULL) {
        return ENOMEM;
    }
    int i;
    //Initial value of table is NULL
    for (i = 0; i < MAX_FILE_OPEN; i++) {
        t->fds[i] = NULL;
    }
    
    curthread->t_process->table = t; //assign table to curthread
    
    
    //create file descriptors stdin, stdout and stderr for consoles
    for (i = 0; i < 3; i++) {
        strcpy(path, "con:");
        //I'm pretty sure this is wrong, someone check this later: ASSUMING std in/out/err is rw, should check this later
        result = fd_table_open(path, O_RDWR, &fd);
        if (result) {
            return result;
        }
    }
    
    return 0;
}




int fd_table_open(char *name, int flag, int *retval) {
    int fd;
    
    //set next available file descriptor
    struct fd_table *t = curthread->t_process->table;
    for (fd = 0; fd < MAX_FILE_OPEN; fd++) {
        if (t->fds[fd] == NULL) {
            break;
        }
    }
    
    if (fd >= MAX_FILE_OPEN) {
        return EMFILE; // too many open files
    }
    
    //Allocate the file descriptor
    struct file *f = kmalloc(sizeof(struct file));
    if (f==NULL) {
        return ENOMEM;
    }
    
    //Initialize file
    f->flag = flag;
    f->offset = 0;
    f->ref_count = 1;
    
    //open the file for our vnode
    int result = vfs_open(name, flag, &(f->vn));
    if (result) {
        kfree(f->vn);
        kfree(f);
        return result;
    }
    
    //set current table slot
    curthread->t_process->table->fds[fd] = f;
    
    *retval = fd;
    return 0;
}

int fd_table_close(int fd) {
    struct file *f;
    int result;
    //get entry from table
    result = fd_table_get(fd, &f);
    if (result) {
        return result;
    }
    
    f->ref_count--;
    
    if (f->ref_count == 0) { // no longer used by anyone
        vfs_close(f->vn);
        kfree(f);
        curthread->t_fdtable->fds[fd] = NULL;
    }
    return 0;
    
}

int fd_table_get(int fd, struct file **retval) {
    
    struct fd_table *t = curthread->t_fdtable;
    
    // check if fields are valid
    if (fd < 0 || fd >= MAX_FILE_OPEN) {
        return EBADF;
    }
    if (t->fds[fd] == NULL) {
        EBADF;
    }
    
    //found entry!
    *retval = t->fds[fd];
    return 0;
    
}

void fd_table_destroy() {
    struct fd_table *t = curthread->t_fdtable;
    
    if (t == NULL) {
        return;
    }
    int i;
    for (i=0; i<MAX_FILE_OPEN; i++) {
        if (t->fds[i] != NULL) {
            fd_table_close(i);
        }
    }
    
    kfree(t);
}
#endif /* OPT_A2 */
