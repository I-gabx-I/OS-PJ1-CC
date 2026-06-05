#include <stdint.h>
#include "../hal/watchdog.h"
#include "../hal/timer.h"
#include "pcb.h"
#include "scheduler.h"
#include "../lib/stdio.h"
#include "../hal/uart.h"
 
/* ---------------------------------------------------------------
 * OS idle loop (pid=0). Runs in System mode (privileged) on its own
 * stack. Only reached if every user task has terminated; until then
 * the very first timer IRQ hands the CPU to P1 and never comes back
 * here. Kept as a clean halt/idle target (Phase 2, Section 4.6).
 * --------------------------------------------------------------- */
void os_idle(void) {
    while (1) { /* spin: IRQs stay enabled so a revived task could run */ }
}
 
/* ---------------------------------------------------------------
 * timer_irq_handler — C half of the timer IRQ context switch.
 *
 * `frame` points at the 17-word context the assembly handler built
 * on the IRQ stack. Layout (shared by IRQ / SVC / abort paths):
 *
 *   frame[0]  = task SP   (USR/System banked)
 *   frame[1]  = task LR
 *   frame[2]  = SPSR      (task CPSR at interrupt — carries USR mode)
 *   frame[3]  = r0
 *   ...
 *   frame[15] = r12
 *   frame[16] = PC        (already corrected to the interrupted insn)
 *
 * Phase 2 adds the MODE_SWITCH trace lines required by Section 3.8.
 * --------------------------------------------------------------- */
void timer_irq_handler(unsigned int *frame) {
    clear_timer_interrupt();
 
    static int os_started = 0;
 
    if (os_started == 0) {
        /* --- First ever switch: launch the first user task --- */
        schedule();                         /* OS(0) -> P1(1) */
        pcb_t *next = &process_table[current_process];
 
        frame[0]  = next->sp;
        frame[1]  = next->lr;
        frame[2]  = next->spsr;             /* 0x10 -> drops to USR on return */
        for (int i = 0; i < 13; i++) frame[3 + i] = next->r[i];
        frame[16] = next->pc;
 
        /* Trace: Section 3.8 row 1 */
        PRINT("MODE_SWITCH KERNEL_TO_USER pid=%d reason=initial_launch\n",
              current_process);
 
        os_started = 1;
        return;
    }
 
    /* --- 1. SAVE the interrupted user task's context --- */
    int prev = current_process;
    pcb_t *cur = &process_table[prev];
    cur->sp   = frame[0];
    cur->lr   = frame[1];
    cur->spsr = frame[2];
    for (int i = 0; i < 13; i++) cur->r[i] = frame[3 + i];
    cur->pc   = frame[16];
 
    /* Trace: Section 3.8 row 2 (entered kernel because of the timer) */
    PRINT("MODE_SWITCH USER_TO_KERNEL pid=%d reason=timer_irq\n", prev);
 
    /* --- 2. CHOOSE next task (round-robin) --- */
    schedule();
    pcb_t *next = &process_table[current_process];
 
    /* --- 3. RESTORE next task's context into the frame --- */
    frame[0]  = next->sp;
    frame[1]  = next->lr;
    frame[2]  = next->spsr;
    for (int i = 0; i < 13; i++) frame[3 + i] = next->r[i];
    frame[16] = next->pc;
 
    /* Trace: Section 3.8 row 3 (scheduler dispatched a task back to USR) */
    PRINT("MODE_SWITCH KERNEL_TO_USER pid=%d reason=dispatch\n",
          current_process);
}
 
int main(void) {
    disable_watchdog();
 
    UART_putc('\n');
    UART_putc('V'); UART_putc('I'); UART_putc('V'); UART_putc('O');
    UART_putc('\n');
 
    init_scheduler();
    init_timer();
 
    /*
     * Memory barrier: make sure every PCB write above is visible to
     * the IRQ handler before the first timer tick fires. ARMv7 can
     * otherwise reorder/delay these stores (Phase 2 / Phase 1 hint).
     * (Skipped on the ARMv5 QEMU model, which lacks dsb/isb.)
     */
#ifndef QEMU_MODE
    asm volatile("dsb" ::: "memory");
    asm volatile("isb" ::: "memory");
#endif
 
    PRINT("\n*** BARE-METAL OS (Phase 2: USR mode) INICIADO ***\n");
    PRINT("Esperando primer tick del Timer para lanzar tareas USR...\n");
 
    /* OS spins in SVC mode until the first IRQ launches P1 in USR. */
    while (1) {}
    return 0;
}
