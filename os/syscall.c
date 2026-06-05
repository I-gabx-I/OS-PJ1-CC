/*
 * syscall.c — Phase 2, Section 4: kernel syscall dispatcher
 *
 * Called from svc_entry in root.s after a user task executes `svc #0`.
 * The assembly stub builds the SAME 17-word frame the IRQ handler uses
 * (see os.c / root.s), so the scheduler can switch tasks after YIELD or
 * EXIT with the identical save/restore logic.
 *
 * Frame layout (shared by IRQ / SVC / abort paths — PDF Section 6):
 *   frame[0]  = task SP   (USR/System banked)
 *   frame[1]  = task LR
 *   frame[2]  = SPSR  (caller CPSR — used to verify USR origin)
 *   frame[3]  = r0    (syscall ID in; return value out)
 *   frame[4]  = r1    (arg1)
 *   frame[5]  = r2    (arg2)
 *   frame[6]  = r3    (arg3)
 *   frame[7..15] = r4-r12
 *   frame[16] = PC    (instruction AFTER the svc — resume point)
 */
 
#include <stdint.h>
#include <stddef.h>
#include "pcb.h"
#include "scheduler.h"
#include "context.h"
#include "../lib/stdio.h"
#include "../hal/uart.h"
 
/* Syscall IDs (must mirror lib/user_syscalls.h) */
#define SYS_YIELD  0
#define SYS_EXIT   1
#define SYS_WRITE  2
 
/* Return / error codes (PDF Section 4.8) */
#define ERR_BAD_SYSCALL  (-1)
#define ERR_BAD_FD       (-2)
#define ERR_BAD_PTR      (-3)
 
/* CPSR mode field */
#define CPSR_MODE_MASK  0x1Fu
#define CPSR_MODE_USR   0x10u
 
/* User address space bounds for pointer validation (covers P1..P3) */
#ifdef QEMU_MODE
    #define USR_REGION_BASE  0x00100000u
    #define USR_REGION_LIMIT 0x00320000u
#else
    #define USR_REGION_BASE  0x82100000u
    #define USR_REGION_LIMIT 0x82320000u
#endif
 
#define SYS_WRITE_MAX_LEN  4096u
 
/* True if [ptr, ptr+len) lies fully inside the user region. */
static int valid_user_ptr(uint32_t ptr, uint32_t len) {
    if (len == 0)                       return 0;
    if (ptr < USR_REGION_BASE)          return 0;
    if (ptr + len < ptr)                return 0;   /* overflow */
    if (ptr + len > USR_REGION_LIMIT)   return 0;
    return 1;
}
 
/* ---------------------------------------------------------------
 * svc_handler — C dispatcher invoked by root.s (r0 = &frame)
 * --------------------------------------------------------------- */
void svc_handler(unsigned int *frame) {
    uint32_t saved_cpsr = frame[2];
    uint32_t id   = frame[3];   /* r0 */
    uint32_t arg1 = frame[4];   /* r1 */
    uint32_t arg2 = frame[5];   /* r2 */
    uint32_t arg3 = frame[6];   /* r3 */
 
    int caller = current_process;
 
    /* Verify the trap came from user mode (PDF Section 4.1). */
    if ((saved_cpsr & CPSR_MODE_MASK) != CPSR_MODE_USR) {
        frame[3] = (unsigned int)(int32_t)ERR_BAD_SYSCALL;
        return;
    }
 
    process_table[caller].last_syscall = id;
 
    /* Trace: Section 3.8 row 4 */
    PRINT("MODE_SWITCH USER_TO_KERNEL pid=%d reason=syscall id=%d\n",
          caller, (int)id);
 
    int32_t rc = ERR_BAD_SYSCALL;
 
    switch (id) {
 
    /* ---- SYS_YIELD (PDF 4.5): always reschedule ---- */
    case SYS_YIELD: {
        pcb_t *cur = &process_table[caller];
        frame_to_pcb(frame, cur);
        cur->r[0] = 0;                       /* yield returns 0 when caller resumes */
        if (cur->state != TERMINATED) cur->state = READY;
 
        schedule();
        pcb_to_frame(&process_table[current_process], frame);
        rc = 0;
        break;
    }
 
    /* ---- SYS_EXIT (PDF 4.6): terminate caller, never return ---- */
    case SYS_EXIT: {
        int32_t code = (int32_t)arg1;
        pcb_t *cur = &process_table[caller];
        frame_to_pcb(frame, cur);            /* leave a clean record */
 
        terminate_process(caller, code, FAULT_EXIT);
        PRINT("PROCESS_EXIT pid=%d code=%d\n", caller, (int)code);
 
        schedule();
        pcb_to_frame(&process_table[current_process], frame);
        rc = 0;
        break;
    }
 
    /* ---- SYS_WRITE (PDF 4.7): write user buffer to UART ---- */
    case SYS_WRITE: {
        int32_t  fd  = (int32_t)arg1;
        uint32_t buf = arg2;
        uint32_t len = arg3;
 
        if (fd != 1) {                       /* this phase: only fd 1 */
            rc = ERR_BAD_FD;
            break;
        }
        if (len > SYS_WRITE_MAX_LEN) len = SYS_WRITE_MAX_LEN;
        if (!valid_user_ptr(buf, len)) {     /* validate BEFORE deref */
            rc = ERR_BAD_PTR;
            break;
        }
 
        const char *p = (const char *)(uintptr_t)buf;
        for (uint32_t i = 0; i < len; i++) UART_putc(p[i]);
        rc = (int32_t)len;
        /* No reschedule: caller resumes immediately with r0 = count. */
        frame[3] = (unsigned int)rc;
        break;
    }
 
    /* ---- Unknown ID ---- */
    default:
        rc = ERR_BAD_SYSCALL;
        frame[3] = (unsigned int)rc;
        break;
    }
 
    /* Trace: Section 3.8 row 5 (current_process may differ after yield/exit) */
    PRINT("MODE_SWITCH KERNEL_TO_USER pid=%d reason=syscall_return id=%d rc=%d\n",
          current_process, (int)id, (int)rc);
}
