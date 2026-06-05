#ifndef SCHEDULER_H
#define SCHEDULER_H
 
#include "pcb.h"
 
/* Global process table and current-process index */
extern pcb_t process_table[3];
extern int   current_process;
 
/* Initialise PCBs and set first task ready                        */
void init_scheduler(void);
 
/* Round-robin: advance current_process skipping TERMINATED tasks  */
void schedule(void);
 
/* Mark a task terminated (called by SYS_EXIT or fault handler)    */
void terminate_process(int pid, int32_t exit_code, uint32_t fault_type);
 
#endif /* SCHEDULER_H */
