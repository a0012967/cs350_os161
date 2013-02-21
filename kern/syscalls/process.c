#include <process.h>




void proctable_init(struct process * t,int len){
    
    int i;
    for(i = 1;i<len;i++){
        t[i] = NULL;
}


pid_t getpid(){
    int i;
    for(i = 0; i<max_process; i++){
        
        if(proctable[i].me == curthread){
            
            return proctabe[i].PID;
        }
        
        
    }
    
    return (pid_t)(-1);
    
}