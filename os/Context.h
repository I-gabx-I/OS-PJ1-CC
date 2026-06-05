#ifndef CONTEXT_H
#define CONTEXT_H
 
#include "pcb.h"
 
/*
 * The 17-word trap frame shared by ALL kernel entry paths
 * (IRQ, SVC, prefetch/data abort, undef) — PDF Section 6.
 *
 *   frame[0]  = task SP   (USR/System banked)
 *   frame[1]  = task LR
 *   frame[2]  = SPSR      (task CPSR — carries USR mode bits)
 *   frame[3]  = r0
 *   ...
 *   frame[15] = r12
 *   frame[16] = PC        (resume / faulting address, mode-corrected in asm)
 */
 
static inline void frame_to_pcb(unsigned int *frame, pcb_t *p) {
    p->sp   = frame[0];
    p->lr   = frame[1];
    p->spsr = frame[2];
    for (int i = 0; i < 13; i++) p->r[i] = frame[3 + i];
    p->pc   = frame[16];
}
 
static inline void pcb_to_frame(pcb_t *p, unsigned int *frame) {
    frame[0] = p->sp;
    frame[1] = p->lr;
    frame[2] = p->spsr;
    for (int i = 0; i < 13; i++) frame[3 + i] = p->r[i];
    frame[16] = p->pc;
}
 
#endif /* CONTEXT_H */
 
