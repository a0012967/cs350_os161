#ifndef _fileDescriptor_H_
#define _fileDescriptor_H_
#include <kern/limits.h>
#include <types.h>

#include "opt-A2.h"
#include <vnode.h>

#if OPT_A2

#define MAX_FILE_OPEN 20


struct file {
    
    //char name[NAME_MAX];
    
    int permission; // Remembers what flag file was opened with. Important for read/write
    off_t offset; // file pointer offset
    int ref_count;
    int flag; //O_RD etc
    struct vnode *vn; //pointer to actual file
};


/*
 The file descriptor table
 Used for kernel-side bookkeeping
 
 Basically an array of  pointers file descriptors.
 Length is set by how many files can be opened at any given time
 */
struct fd_table  {
    struct file *fds[MAX_FILE_OPEN];
};


//Open file and add to table
int fd_table_open(char *name, int flag, int *retval);

// Close file and remove from table
int fd_table_close(int fd);
int fd_table_close_me(int fd, struct fd_table *t);

// gets entry from table
int fd_table_get(int fd, struct file **retval);
int fd_table_get_me(int fd, struct file **retval, struct fd_table *t);

int fd_table_create();
void fd_table_destroy();
void fd_table_destroy_me(struct fd_table* t);

#endif /* _OPT_A2 */
#endif //define
