#ifndef SCHEDULER_H
#define SCHEDULER_H
 
#include "pcb.h"
 
/* Process slots: 0 = OS/idle, 1 = P1, 2 = P2, 3 = P3 */
#define NPROC 4
 
extern pcb_t process_table[NPROC];
extern int   current_process;
 
void init_scheduler(void);
void schedule(void);   /* round-robin over runnable user tasks 1..NPROC-1 */
void terminate_process(int pid, int32_t exit_code, uint32_t fault_type);
 
#endif /* SCHEDULER_H */
