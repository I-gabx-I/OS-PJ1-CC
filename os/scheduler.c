#include "scheduler.h"
#include "../lib/stdio.h"
 
pcb_t process_table[NPROC];
int   current_process = 0;
 
/* ---------------------------------------------------------------
 * Memory map (fixed, no MMU). User tasks start in USR (0x10).
 * The OS/idle (pid 0) runs in System mode (0x1F), sharing the USR
 * banked SP/LR so the IRQ/SVC/abort handlers use one save/restore.
 * --------------------------------------------------------------- */
#ifdef QEMU_MODE
    #define P1_ENTRY  0x00100000
    #define P1_STACK  0x00102000
    #define P2_ENTRY  0x00200000
    #define P2_STACK  0x00202000
    #define P3_ENTRY  0x00300000
    #define P3_STACK  0x00302000
#else
    #define P1_ENTRY  0x82100000
    #define P1_STACK  0x82112000
    #define P2_ENTRY  0x82200000
    #define P2_STACK  0x82212000
    #define P3_ENTRY  0x82300000
    #define P3_STACK  0x82312000
#endif
 
#define CPSR_USR  0x00000010u
#define CPSR_SYS  0x0000001Fu
 
extern void     os_idle(void);
extern uint32_t _sys_stack_top;
 
/* Helper to initialise one user PCB. */
static void init_user(int slot, uint32_t entry, uint32_t stack) {
    process_table[slot].pid         = (uint32_t)slot;
    process_table[slot].state       = READY;
    process_table[slot].pc          = entry;
    process_table[slot].sp          = stack;
    process_table[slot].lr          = entry;
    process_table[slot].spsr        = CPSR_USR;
    process_table[slot].exit_code   = 0;
    process_table[slot].fault_type  = FAULT_NONE;
    process_table[slot].last_syscall= 0;
    for (int i = 0; i < 13; i++) process_table[slot].r[i] = 0;
}
 
void init_scheduler(void) {
    /* OS / idle process (pid 0), System mode. */
    process_table[0].pid         = 0;
    process_table[0].state       = RUNNING;
    process_table[0].pc          = (uint32_t)&os_idle;
    process_table[0].lr          = (uint32_t)&os_idle;
    process_table[0].sp          = (uint32_t)&_sys_stack_top;
    process_table[0].spsr        = CPSR_SYS;
    process_table[0].exit_code   = 0;
    process_table[0].fault_type  = FAULT_NONE;
    process_table[0].last_syscall= 0;
    for (int i = 0; i < 13; i++) process_table[0].r[i] = 0;
 
    init_user(1, P1_ENTRY, P1_STACK);   /* digits, write+yield forever  */
    init_user(2, P2_ENTRY, P2_STACK);   /* letters, then SYS_EXIT        */
    init_user(3, P3_ENTRY, P3_STACK);   /* faults deliberately           */
 
    current_process = 0;
}
 
/*
 * Round-Robin over user slots 1..NPROC-1, skipping TERMINATED tasks.
 * If no user task is runnable, fall back to OS idle (pid 0).
 */
void schedule(void) {
    int next = current_process;
 
    for (int i = 0; i < (NPROC - 1); i++) {
        next++;
        if (next >= NPROC) next = 1;     /* wrap into the user range */
        if (process_table[next].state != TERMINATED) {
            if (process_table[current_process].state != TERMINATED)
                process_table[current_process].state = READY;
            process_table[next].state = RUNNING;
            current_process = next;
            return;
        }
    }
 
    /* No runnable user task -> idle (documented halt policy). */
    if (process_table[current_process].state != TERMINATED)
        process_table[current_process].state = READY;
    process_table[0].state = RUNNING;
    current_process = 0;
}
 
void terminate_process(int pid, int32_t exit_code, uint32_t fault_type) {
    if (pid < 1 || pid >= NPROC) return;   /* never terminate the OS */
    process_table[pid].state      = TERMINATED;
    process_table[pid].exit_code  = exit_code;
    process_table[pid].fault_type = fault_type;
}
 
