#ifndef PCB_H
#define PCB_H
 
#include <stdint.h>
 
/* ---------------------------------------------------------------
 * Process States
 * Phase 2 extends Phase 1 (READY/RUNNING) with WAITING and
 * TERMINATED so the scheduler can handle SYS_EXIT and fault
 * containment without touching a dead process.
 * --------------------------------------------------------------- */
#define READY      0
#define RUNNING    1
#define WAITING    2   /* reserved for future blocking syscalls    */
#define TERMINATED 3   /* set by SYS_EXIT or fault handler         */
 
/* ---------------------------------------------------------------
 * Fault / exit reason codes stored in fault_type
 * --------------------------------------------------------------- */
#define FAULT_NONE          0
#define FAULT_PREFETCH      1   /* prefetch abort from USR          */
#define FAULT_DATA          2   /* data abort from USR              */
#define FAULT_ALIGN         3   /* alignment error                  */
#define FAULT_PRIVILEGE     4   /* privilege violation              */
#define FAULT_EXIT          5   /* clean SYS_EXIT                   */
#define FAULT_UNDEF         6   /* undefined instruction from USR   */
 
/* ---------------------------------------------------------------
 * Process Control Block
 *
 * Register save layout (must match root.s frame exactly):
 *
 *   frame[0]  = SVC_SP   (sp of the task in SVC/USR banked set)
 *   frame[1]  = SVC_LR   (lr of the task)
 *   frame[2]  = SPSR     (CPSR snapshot when interrupted)
 *   frame[3]  = r0
 *   ...
 *   frame[15] = r12
 *   frame[16] = PC       (return address, already corrected -4)
 *
 * This layout is shared by the IRQ path, the SVC path, and the
 * abort path so that the scheduler can be called from any of them
 * with the same frame pointer convention.
 * --------------------------------------------------------------- */
typedef struct {
    uint32_t pid;           /* 0=OS, 1=P1, 2=P2                    */
    uint32_t state;         /* READY / RUNNING / WAITING / TERMINATED */
 
    /* --- Saved register context (Phase 1 compatible) ----------- */
    uint32_t sp;            /* banked SP (SVC or USR)              */
    uint32_t lr;            /* banked LR                           */
    uint32_t pc;            /* resume address                      */
    uint32_t spsr;          /* CPSR at interrupt time              */
    uint32_t r[13];         /* r0-r12                              */
 
    /* --- Phase 2 additions ------------------------------------- */
    int32_t  exit_code;     /* recorded by SYS_EXIT                */
    uint32_t fault_type;    /* FAULT_* reason code                 */
    uint32_t last_syscall;  /* syscall ID of most recent svc call  */
} pcb_t;
 
#endif /* PCB_H */
