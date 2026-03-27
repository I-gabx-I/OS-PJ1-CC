.global start

start:
    b reset_handler      @ 0x00: Reset
    b hang               @ 0x04: Undefined instruction
    b hang               @ 0x08: SWI/SVC
    b hang               @ 0x0C: Prefetch abort
    b hang               @ 0x10: Data abort
    b hang               @ 0x14: Reserved
    b irq_handler        @ 0x18: IRQ
    b hang               @ 0x1C: FIQ

reset_handler:
#ifdef QEMU_MODE
    @ QEMU (VersatilePB / ARMv5): setup IRQ stack first
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0x12       @ IRQ mode
    msr cpsr_c, r0
    ldr sp, =_irq_stack_top

    @ Back to SVC mode for OS stack
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0x13       @ SVC mode
    msr cpsr_c, r0
    ldr sp, =_os_stack_top
#else
    @ BeagleBone (ARMv7)
    ldr sp, =_os_stack_top
#endif

    @ Clear BSS
    ldr r0, =bss_start
    ldr r1, =bss_end
    mov r2, #0
clear_bss:
    cmp r0, r1
    bge bss_cleared
    str r2, [r0], #4
    b clear_bss
bss_cleared:

    @ Enable IRQs
    mrs r0, cpsr
    bic r0, r0, #0x80
    msr cpsr_c, r0

#ifndef QEMU_MODE
    @ ARMv7 only (BeagleBone)
    dsb
    isb
#endif

    bl main

hang:
    b hang

irq_handler:
    stmfd sp!, {r0-r12, lr}
    bl timer_irq_handler
    ldmfd sp!, {r0-r12, pc}^