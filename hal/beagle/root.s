.global start
 
@ ======================================================================
@  ARM Exception Vector Table (BeagleBone / AM335x, Cortex-A8)
@ ======================================================================
start:
    b reset_handler      @ 0x00: Reset
    b undef_entry        @ 0x04: Undefined Instruction      <-- fault path
    b svc_entry          @ 0x08: Software Interrupt (SVC)  <-- syscall path
    b prefetch_abort_entry @ 0x0C: Prefetch Abort          <-- fault path
    b data_abort_entry   @ 0x10: Data Abort                <-- fault path
    b hang               @ 0x14: Reserved
    b irq_handler        @ 0x18: IRQ (Timer)
    b hang               @ 0x1C: FIQ
 
reset_handler:
    @ --- Relocate the vector table (VBAR <- start) ---
    ldr r0, =start
    mcr p15, 0, r0, c12, c0, 0
 
    @ --- IRQ-mode stack ---
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0xD2       @ IRQ mode, I=1 F=1
    msr cpsr_c, r0
    ldr sp, =_irq_stack_top
 
    @ --- System-mode stack (PHASE 2) ---
    @ System mode shares the USR-banked SP/LR. Initialising it here
    @ means the very first IRQ grab reads a sane value (it is later
    @ overwritten per-task), and it is the stack used by os_idle().
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0xDF       @ System mode, I=1 F=1
    msr cpsr_c, r0
    ldr sp, =_sys_stack_top
 
    @ --- Abort-mode stack (PHASE 2: prefetch/data abort handlers) ---
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0xD7       @ Abort mode (0x17), I=1 F=1
    msr cpsr_c, r0
    ldr sp, =_abt_stack_top
 
    @ --- Undefined-mode stack (PHASE 2: undefined-instruction handler) ---
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0xDB       @ Undefined mode (0x1B), I=1 F=1
    msr cpsr_c, r0
    ldr sp, =_und_stack_top
 
    @ --- SVC-mode stack (OS runs here during boot) ---
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0xD3       @ SVC mode, I=1 F=1
    msr cpsr_c, r0
    ldr sp, =_os_stack_top
 
    @ --- Clear .bss ---
    ldr r0, =bss_start
    ldr r1, =bss_end
    mov r2, #0
clear_bss:
    cmp r0, r1
    bge bss_cleared
    str r2, [r0], #4
    b   clear_bss
bss_cleared:
 
    @ --- Memory barrier after .bss clear ---
    dsb
    isb
 
    @ --- Enable IRQs at the CPU (clear I bit) ---
    mrs r0, cpsr
    bic r0, r0, #0x80
    msr cpsr_c, r0
 
    @ --- Enter C ---
    bl main
 
hang:
    b hang
 
@ ======================================================================
@  IRQ Handler  (Timer-driven context switch, USR <-> kernel)
@
@  Builds a 17-word frame on the IRQ stack and passes &frame to C:
@     frame[0]  = task SP   (USR/System banked)
@     frame[1]  = task LR   (USR/System banked)
@     frame[2]  = SPSR      (task CPSR -> carries USR mode bits)
@     frame[3..15] = r0..r12
@     frame[16] = PC        (interrupted instruction, corrected -4)
@
@  PHASE 2 CHANGE vs Phase 1:
@     The temporary mode switch used to read/write the task's banked
@     SP/LR is now SYSTEM mode (0x9F = 0x1F | I-bit), NOT SVC (0x93).
@     User tasks live in USR mode; USR and System share SP/LR, so this
@     is what correctly captures/restores a preempted user task.
@ ======================================================================
irq_handler:
    @ 1. Correct the return address to the interrupted instruction
    sub lr, lr, #4
 
    @ 2. Save r0-r12 and the corrected PC (14 words)
    stmfd sp!, {r0-r12, lr}
 
    @ 3. Save SPSR of the interrupted task (15 words)
    mrs r0, spsr
    stmfd sp!, {r0}
 
    @ 4. CRITICAL SECTION: switch to SYSTEM mode (shares USR SP/LR)
    @    with IRQs disabled, to read the task's banked SP/LR.
    mrs r1, cpsr
    bic r0, r1, #0x1F
    orr r0, r0, #0x9F       @ 0x1F (System) | 0x80 (I=1)
    msr cpsr_c, r0
    mov r2, sp             @ task SP (USR/System banked)
    mov r3, lr             @ task LR
    msr cpsr_c, r1         @ back to IRQ mode
 
    @ 5. Push SP/LR -> frame now complete (17 words) for C
    stmfd sp!, {r2, r3}
 
    @ 6. Call the C scheduler half (r0 = &frame)
    mov r0, sp
    bl timer_irq_handler
 
    @ --- C returns with the NEXT task's context written into frame ---
 
    @ 7. Pop the next task's SP/LR
    ldmfd sp!, {r2, r3}
 
    @ 8. CRITICAL SECTION: System mode again to inject SP/LR
    mrs r1, cpsr
    bic r0, r1, #0x1F
    orr r0, r0, #0x9F
    msr cpsr_c, r0
    mov sp, r2            @ install next task SP
    mov lr, r3            @ install next task LR
    msr cpsr_c, r1        @ back to IRQ mode
 
    @ 9. End-Of-Interrupt to the INTC (0x48200048 = INTC_CONTROL)
    ldr r0, =0x48200048
    mov r2, #1
    str r2, [r0]
 
    @ 10. Restore SPSR of the next task (becomes CPSR on return)
    ldmfd sp!, {r0}
    msr spsr_cxsf, r0
 
    @ 11. Restore r0-r12 and return; '^' copies SPSR -> CPSR,
    @     dropping the CPU into the task's mode (USR for P1/P2).
    ldmfd sp!, {r0-r12, pc}^
 
@ ======================================================================
@  SVC Handler  (Syscall path, USR -> kernel -> USR)
@
@  Twin of irq_handler, building the IDENTICAL 17-word frame so the C
@  dispatcher and scheduler reuse the same save/restore logic.
@
@  DIFFERENCES vs irq_handler:
@    * NO 'sub lr, lr, #4'  -> the SVC return offset is 0; lr_svc
@      already points to the instruction AFTER 'svc' (the resume point).
@    * NO INTC End-Of-Interrupt -> that is timer-specific.
@    * Runs on the SVC-mode stack (we are in SVC after the trap), and
@      IRQs are masked by hardware on SVC entry (no reentrancy).
@
@  The temporary SP/LR rescue still uses SYSTEM mode (0x9F), because the
@  task lives in USR and USR/System share the banked SP/LR.
@ ======================================================================
svc_entry:
    @ Save r0-r12 and lr_svc (resume PC). NOTE: no -4 adjustment.
    stmfd sp!, {r0-r12, lr}
 
    @ Save SPSR (caller USR CPSR)
    mrs r0, spsr
    stmfd sp!, {r0}
 
    @ System mode (shares USR SP/LR), IRQs off -> read task SP/LR
    mrs r1, cpsr
    bic r0, r1, #0x1F
    orr r0, r0, #0x9F
    msr cpsr_c, r0
    mov r2, sp
    mov r3, lr
    msr cpsr_c, r1
 
    @ Push SP/LR -> 17-word frame complete
    stmfd sp!, {r2, r3}
 
    @ Call C dispatcher (r0 = &frame)
    mov r0, sp
    bl svc_handler
 
    @ --- return: pop next task SP/LR, inject via System mode ---
    ldmfd sp!, {r2, r3}
    mrs r1, cpsr
    bic r0, r1, #0x1F
    orr r0, r0, #0x9F
    msr cpsr_c, r0
    mov sp, r2
    mov lr, r3
    msr cpsr_c, r1
 
    @ Restore SPSR of the task we return to, then exception-return to USR
    ldmfd sp!, {r0}
    msr spsr_cxsf, r0
    ldmfd sp!, {r0-r12, pc}^
 
@ ======================================================================
@  Fault Handlers  (Section 5: containment & isolation)
@
@  All three build the SAME 17-word frame as irq/svc and call the C
@  routine fault_handler(&frame, kind). On return the frame holds the
@  NEXT healthy task's context, and we exception-return into USR.
@
@  LR offsets (ARMv7) — these MUST be right or we log the wrong PC:
@    Undefined instruction : lr = fault+4  -> sub #4
@    Prefetch abort        : lr = fault+4  -> sub #4
@    Data abort            : lr = fault+8  -> sub #8
@
@  kind values match the FAULT_* codes in pcb.h:
@    FAULT_UNDEF=6, FAULT_PREFETCH=1, FAULT_DATA=2
@
@  Each runs in its own privileged mode (Undef 0x1B / Abort 0x17) on a
@  dedicated stack set up in reset_handler. The USR SP/LR rescue uses
@  System mode (0x9F), exactly like irq/svc.
@ ======================================================================
 
undef_entry:
    sub lr, lr, #4                 @ faulting (undefined) instruction
    stmfd sp!, {r0-r12, lr}
    mrs r0, spsr
    stmfd sp!, {r0}
    mrs r1, cpsr
    bic r0, r1, #0x1F
    orr r0, r0, #0x9F
    msr cpsr_c, r0
    mov r2, sp
    mov r3, lr
    msr cpsr_c, r1
    stmfd sp!, {r2, r3}
    mov r0, sp
    mov r1, #6                     @ FAULT_UNDEF
    bl fault_handler
    b   fault_return
 
prefetch_abort_entry:
    sub lr, lr, #4                 @ faulting (prefetch) instruction
    stmfd sp!, {r0-r12, lr}
    mrs r0, spsr
    stmfd sp!, {r0}
    mrs r1, cpsr
    bic r0, r1, #0x1F
    orr r0, r0, #0x9F
    msr cpsr_c, r0
    mov r2, sp
    mov r3, lr
    msr cpsr_c, r1
    stmfd sp!, {r2, r3}
    mov r0, sp
    mov r1, #1                     @ FAULT_PREFETCH
    bl fault_handler
    b   fault_return
 
data_abort_entry:
    sub lr, lr, #8                 @ faulting (data) instruction (offset -8)
    stmfd sp!, {r0-r12, lr}
    mrs r0, spsr
    stmfd sp!, {r0}
    mrs r1, cpsr
    bic r0, r1, #0x1F
    orr r0, r0, #0x9F
    msr cpsr_c, r0
    mov r2, sp
    mov r3, lr
    msr cpsr_c, r1
    stmfd sp!, {r2, r3}
    mov r0, sp
    mov r1, #2                     @ FAULT_DATA
    bl fault_handler
    @ falls through to fault_return
 
@ Shared exception-return: frame already holds the next task's context.
fault_return:
    ldmfd sp!, {r2, r3}
    mrs r1, cpsr
    bic r0, r1, #0x1F
    orr r0, r0, #0x9F
    msr cpsr_c, r0
    mov sp, r2
    mov lr, r3
    msr cpsr_c, r1
    ldmfd sp!, {r0}
    msr spsr_cxsf, r0
    ldmfd sp!, {r0-r12, pc}^
