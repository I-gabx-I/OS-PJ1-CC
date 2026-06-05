/*
 * fault.c — Phase 2, Section 5: fault containment & isolation
 *
 * Called from the abort/undef vectors in root.s. A user task that
 * executes an illegal instruction or makes an illegal access traps
 * here; the kernel classifies the fault, marks the offending task
 * TERMINATED (records the reason for observability), and reschedules
 * a healthy peer. The kernel and other tasks keep running.
 *
 * Uses the SAME 17-word frame as the IRQ and SVC paths (see context.h),
 * so the restore path that returns to USR is identical.
 *
 * `kind` is passed by the assembly stub:
 *   FAULT_PREFETCH (1) — prefetch abort vector (0x0C)
 *   FAULT_DATA     (2) — data abort vector     (0x10)
 *   FAULT_UNDEF    (6) — undefined instruction  (0x04)
 */
 
#include <stdint.h>
#include "pcb.h"
#include "scheduler.h"
#include "context.h"
#include "../lib/stdio.h"
 
#define CPSR_MODE_MASK  0x1Fu
#define CPSR_MODE_USR   0x10u
 
/* Read CP15 fault-status / fault-address registers. */
static inline uint32_t read_DFSR(void) { uint32_t v; asm volatile("mrc p15,0,%0,c5,c0,0":"=r"(v)); return v; }
static inline uint32_t read_DFAR(void) { uint32_t v; asm volatile("mrc p15,0,%0,c6,c0,0":"=r"(v)); return v; }
static inline uint32_t read_IFSR(void) { uint32_t v; asm volatile("mrc p15,0,%0,c5,c0,1":"=r"(v)); return v; }
static inline uint32_t read_IFAR(void) { uint32_t v; asm volatile("mrc p15,0,%0,c6,c0,2":"=r"(v)); return v; }
 
/*
 * Decode the ARMv7 short-descriptor Fault Status (FS) field.
 * FS = FSR[3:0] with FSR[10] as the high bit (FS[4]).
 */
static const char *classify_fsr(uint32_t fsr) {
    uint32_t fs = (fsr & 0x0F) | ((fsr >> 6) & 0x10);
    switch (fs) {
        case 0x01: return "alignment";
        case 0x05: case 0x07: return "translation";
        case 0x08: case 0x16: return "external";
        case 0x0D: case 0x0F: return "permission";
        default:              return "other";
    }
}
 
/* ---------------------------------------------------------------
 * fault_handler — invoked by root.s (r0 = &frame, r1 = kind)
 * --------------------------------------------------------------- */
void fault_handler(unsigned int *frame, uint32_t kind) {
    int pid          = current_process;
    uint32_t fault_pc = frame[16];
    uint32_t cpsr     = frame[2];
    uint32_t status = 0, addr = 0;
    const char *type;
 
    switch (kind) {
    case FAULT_DATA:
        status = read_DFSR();
        addr   = read_DFAR();
        type   = classify_fsr(status);
        break;
    case FAULT_PREFETCH:
        status = read_IFSR();
        addr   = read_IFAR();
        type   = classify_fsr(status);
        break;
    case FAULT_UNDEF:
    default:
        type   = "undef";
        addr   = fault_pc;
        break;
    }
 
    /* Trace: Section 3.8 row 6 */
    PRINT("MODE_SWITCH USER_TO_KERNEL pid=%d reason=fault type=%s\n", pid, type);
    PRINT("FAULT pid=%d kind=%d pc=%x cpsr=%x status=%x addr=%x\n",
          pid, (int)kind, fault_pc, cpsr, status, addr);
 
    /*
     * Containment policy (PDF Section 5.3): record reason, terminate
     * the offending task, remove it from the runnable set. The OS
     * (pid 0) is never the faulting USR task, so this is always 1..N.
     */
    if (pid >= 1) {
        frame_to_pcb(frame, &process_table[pid]);   /* snapshot for observability */
        terminate_process(pid, -1, kind);
    }
 
    /* Pick a healthy peer (or idle if none remain). */
    schedule();
    pcb_to_frame(&process_table[current_process], frame);
 
    /* Trace: Section 3.8 row 7 */
    PRINT("MODE_SWITCH KERNEL_TO_USER pid=%d reason=fault_recovery\n",
          current_process);
}
