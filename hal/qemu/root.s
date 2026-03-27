.global start

start:
    b reset_handler      @ 0x00: Reset
    b hang               @ 0x04: Undefined
    b hang               @ 0x08: SWI
    b hang               @ 0x0C: Prefetch abort
    b hang               @ 0x10: Data abort
    b hang               @ 0x14: Reserved
    b irq_handler        @ 0x18: IRQ  <-- CPU salta aqui en interrupciones
    b hang               @ 0x1C: FIQ

reset_handler:
    @ Setup stack modo IRQ
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0xD2       @ IRQ mode, I=1, F=1
    msr cpsr_c, r0
    ldr sp, =_irq_stack_top

    @ Setup stack modo SVC (modo OS)
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0xD3       @ SVC mode, I=1, F=1
    msr cpsr_c, r0
    ldr sp, =_os_stack_top

    @ Limpiar BSS
    ldr r0, =bss_start
    ldr r1, =bss_end
    mov r2, #0
clear_bss:
    cmp r0, r1
    bge bss_cleared
    str r2, [r0], #4
    b   clear_bss
bss_cleared:

    @ Habilitar IRQs (limpiar bit I)
    mrs r0, cpsr
    bic r0, r0, #0x80
    msr cpsr_c, r0

    bl main

hang:
    b hang

@ -------------------------------------------------------
@ IRQ Handler
@ frame en stack: [sp+0]=r0 ... [sp+48]=r12 [sp+52]=lr(PC corregido)
@ -------------------------------------------------------
irq_handler:
    @ 1. Corregir LR y guardar contexto inicial en el stack
    sub lr, lr, #4
    stmfd sp!, {r0-r12, lr}   @ [sp] = r0...r12, PC

    @ 2. Preparar argumentos para timer_irq_handler (el puntero al frame)
    @ Pasamos el Stack Pointer (que apunta al frame guardado) a r0
    mov r0, sp

    @ 3. Llamar a la funcion en C (Esto hara el PRINT y el schedule)
    bl timer_irq_handler

    @ 4. La funcion en C modifico nuestro stack con los datos del nuevo proceso.
    @ Restaurar todos los registros (r0-r12 y PC) y el CPSR de golpe.
    ldmfd sp!, {r0-r12, pc}^
