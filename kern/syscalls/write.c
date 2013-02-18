#include <types.h>

#include <lib.h>

#include "opt-A2.h"
#include <syscall.h>
/*write writes up to buflen bytes to the file specified by fd, at the location in the file specified by the current seek position of the file, taking the data from the space pointed to by buf. The file must be open for writing.*/
int
write(int fd, const void *buf, size_t nbytes){
    assert(buf != NULL);
    
    if(fd == 1){
        
        kprintf(buf);
    }
    
    
    
    
    
    
    
    
    
    
    
    return 0;
    
}