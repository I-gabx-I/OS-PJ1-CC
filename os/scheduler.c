#include "scheduler.h"
#include "../lib/stdio.h"
 
pcb_t process_table[3];
int   current_process = 0;
 
/* ---------------------------------------------------------------
 * Memory map — same fixed layout as Phase 1.
 *
 * Phase 2 change: the INITIAL SPSR of the two user processes goes
 * from 0x13 (SVC) to 0x10 (USR) so they start in unprivileged mode.
 * The OS/idle process keeps a privileged context but in System mode
 * (0x1F), which shares the USR-banked SP/LR — that lets the IRQ
 * handler use ONE save/restore path for every task.
 * --------------------------------------------------------------- */
#ifdef QEMU_MODE
    #define P1_ENTRY  0x00100000
    #define P1_STACK  0x00102000
    #define P2_ENTRY  0x00200000
    #define P2_STACK  0x00202000
#else
    /* BeagleBone fixed addresses */
    #define P1_ENTRY  0x82100000
    #define P1_STACK  0x82112000   /* base 0x82110000 + 8 KB */
    #define P2_ENTRY  0x82200000
    #define P2_STACK  0x82212000   /* base 0x82210000 + 8 KB */
#endif
 
/*
 * CPSR values used as the initial SPSR for each task.
 *   USR  = 0x10 : unprivileged, ARM state, IRQ+FIQ enabled
 *   SYS  = 0x1F : privileged System mode (idle), IRQ+FIQ enabled
 */
#define CPSR_USR  0x00000010u
#define CPSR_SYS  0x0000001Fu
 
/* The OS idle loop and its dedicated stack live in os.c (System mode). */
extern void     os_idle(void);
extern uint32_t _sys_stack_top;   /* linker symbol: top of idle/System stack */
 
void init_scheduler(void) {
    /* ---- OS / idle process (pid=0): System mode ---- */
    process_table[0].pid         = 0;
    process_table[0].state       = RUNNING;          /* current at boot */
    process_table[0].pc          = (uint32_t)&os_idle;
    process_table[0].lr          = (uint32_t)&os_idle;
    process_table[0].sp          = (uint32_t)&_sys_stack_top;
    process_table[0].spsr        = CPSR_SYS;          /* System mode */
    process_table[0].exit_code   = 0;
    process_table[0].fault_type  = FAULT_NONE;
    process_table[0].last_syscall= 0;
    for (int i = 0; i < 13; i++) process_table[0].r[i] = 0;
 
    /* ---- P1 (pid=1): digits 0-9, runs in USR ---- */
    process_table[1].pid         = 1;
    process_table[1].state       = READY;
    process_table[1].pc          = P1_ENTRY;
    process_table[1].sp          = P1_STACK;
    process_table[1].lr          = P1_ENTRY;          /* first return lands at entry */
    process_table[1].spsr        = CPSR_USR;          /* <-- Phase 2: USR mode */
    process_table[1].exit_code   = 0;
    process_table[1].fault_type  = FAULT_NONE;
    process_table[1].last_syscall= 0;
    for (int i = 0; i < 13; i++) process_table[1].r[i] = 0;
 
    /* ---- P2 (pid=2): letters a-z, runs in USR ---- */
    process_table[2].pid         = 2;
    process_table[2].state       = READY;
    process_table[2].pc          = P2_ENTRY;
    process_table[2].sp          = P2_STACK;
    process_table[2].lr          = P2_ENTRY;
    process_table[2].spsr        = CPSR_USR;          /* <-- Phase 2: USR mode */
    process_table[2].exit_code   = 0;
    process_table[2].fault_type  = FAULT_NONE;
    process_table[2].last_syscall= 0;
    for (int i = 0; i < 13; i++) process_table[2].r[i] = 0;
 
    current_process = 0;
}
 
/*
 * Round-Robin scheduler.
 * Advances OS(0)->P1(1)->P2(2)->P1(1)->P2(2)... and SKIPS any task
 * marked TERMINATED so a dead process is never resumed.  If no user
 * task is runnable, falls back to the OS idle (pid=0), which is never
 * terminated — this is the documented idle/halt policy.
 */
void schedule(void) {
    int next = current_process;
 
    for (int attempts = 0; attempts < 3; attempts++) {
        if (next == 0)      next = 1;
        else if (next == 1) next = 2;
        else                next = 1;
 
        if (process_table[next].state != TERMINATED) break;
    }
 
    if (process_table[next].state == TERMINATED) next = 0;   /* idle fallback */
 
    if (process_table[current_process].state != TERMINATED)
        process_table[current_process].state = READY;
    process_table[next].state = RUNNING;
    current_process = next;
}
 
/*
 * Mark a process terminated. Called by SYS_EXIT and the fault handler
 * (both arrive in later Phase-2 steps). Never terminates the OS (pid=0).
 */
void terminate_process(int pid, int32_t exit_code, uint32_t fault_type) {
    if (pid < 1 || pid > 2) return;
    process_table[pid].state      = TERMINATED;
    process_table[pid].exit_code  = exit_code;
    process_table[pid].fault_type = fault_type;
}
