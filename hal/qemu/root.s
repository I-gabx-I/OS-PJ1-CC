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
    @ -------------------------------------------------------
    @ IRQ Handler (El verdadero Context Switch)
    @ -------------------------------------------------------
    irq_handler:
        @ 1. Corregir el PC interrumpido (instruccion exacta)
        sub lr, lr, #4

        @ 2. Guardar R0-R12 y el PC en nuestro stack de IRQ
        stmfd sp!, {r0-r12, lr}

        @ 3. Guardar el SPSR (estado del proceso)
        mrs r0, spsr
        stmfd sp!, {r0}

        @ 4. ZONA CRITICA: Cambiar a modo SVC desactivando interrupciones (I=1 -> 0x80)
        @ Esto cumple con aislar el Context Switch para evitar reentrancia.
        mrs r1, cpsr
        bic r0, r1, #0x1F
        orr r0, r0, #0x93      @ 0x13 (SVC) | 0x80 (Desactiva IRQ)
        msr cpsr_c, r0

        @ 5. Tomar prestados el SP y LR del proceso (que vive en SVC)
        mov r2, sp
        mov r3, lr

        @ 6. Volver al modo IRQ
        msr cpsr_c, r1

        @ 7. Guardar el SP y LR en nuestro stack. 
        @ ¡La "caja" (frame) de 17 posiciones ya esta lista para C!
        stmfd sp!, {r2, r3}

        @ 8. Llamar al jefe en C pasandole el stack (r0 = sp)
        mov r0, sp
        bl timer_irq_handler

        @ --- REGRESO DE C (La caja trae los datos del nuevo proceso) ---
    
        @ 9. Sacar el SP y LR del nuevo proceso
        ldmfd sp!, {r2, r3}

        @ 10. ZONA CRITICA: Cambiar a SVC (sin interrupciones) para inyectarselos
        mrs r1, cpsr
        bic r0, r1, #0x1F
        orr r0, r0, #0x93
        msr cpsr_c, r0
        mov sp, r2
        mov lr, r3
        msr cpsr_c, r1

        @ 11. Restaurar el SPSR del nuevo proceso
        ldmfd sp!, {r0}
        msr spsr_cxsf, r0

        @ 12. Restaurar R0-R12 y saltar al nuevo PC (El ^ restaura el modo automatico)
        ldmfd sp!, {r0-r12, pc}^
