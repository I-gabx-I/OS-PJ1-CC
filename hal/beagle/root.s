.global start

start:
    b reset_handler      @ 0x00: Reset
    b hang               @ 0x04: Undefined instruction
    b hang               @ 0x08: Software interrupt (SWI/SVC)
    b hang               @ 0x0C: Prefetch abort
    b hang               @ 0x10: Data abort
    b hang               @ 0x14: Reserved
    b irq_handler        @ 0x18: IRQ
    b hang               @ 0x1C: FIQ

reset_handler:
    ldr sp, =_os_stack_top

    ldr r0, =bss_start
    ldr r1, =bss_end
    mov r2, #0
clear_bss:
    cmp r0, r1
    bge bss_cleared
    str r2, [r0], #4
    b clear_bss
bss_cleared:
    
    mrs r0, cpsr
    bic r0, r0, #0x80
    msr cpsr_c, r0

    @ Barreras de memoria exclusivas de ARMv7 (BeagleBone)
    dsb
    isb

    bl main

hang:
    b hang

irq_handler:
    stmfd sp!, {r0-r12, lr}
    bl timer_irq_handler
    ldmfd sp!, {r0-r12, pc}^