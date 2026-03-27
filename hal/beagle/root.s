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
    @ *** FIX #1: Apuntar el VBAR a nuestra tabla de vectores ***
    @ Sin esto, cuando el timer dispara, el CPU salta a 0x00000018 (U-Boot)
    @ en vez de a 0x82000018 (nuestro codigo). El timer "dispara" pero nadie lo escucha.
    ldr r0, =start
    mcr p15, 0, r0, c12, c0, 0   @ Escribir VBAR
 
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
@ IRQ Handler (El verdadero Context Switch)
@ frame en stack (desde SP hacia arriba):
@   [0] = SVC_SP  [1] = SVC_LR  [2] = SPSR
@   [3..15] = R0-R12             [16] = PC (LR_irq corregido)
@ -------------------------------------------------------
irq_handler:                         @ *** FIX #2: etiqueta duplicada eliminada ***
 
    @ 1. Corregir el PC interrumpido (instruccion exacta)
    sub lr, lr, #4
 
    @ 2. Guardar R0-R12 y el PC en nuestro stack de IRQ
    stmfd sp!, {r0-r12, lr}
 
    @ 3. Guardar el SPSR (estado del proceso interrumpido)
    mrs r0, spsr
    stmfd sp!, {r0}
 
    @ 4. ZONA CRITICA: Cambiar a SVC desactivando IRQs para el context switch
    mrs r1, cpsr
    bic r0, r1, #0x1F
    orr r0, r0, #0x93      @ 0x13 (SVC) | 0x80 (I=1, sin re-entrada)
    msr cpsr_c, r0
 
    @ 5. Tomar el SP y LR del proceso actual (que viven en SVC)
    mov r2, sp
    mov r3, lr
 
    @ 6. Volver al modo IRQ
    msr cpsr_c, r1
 
    @ 7. Completar la "caja" (frame de 17 posiciones) con SP y LR del SVC
    stmfd sp!, {r2, r3}
 
    @ 8. Llamar al handler en C pasandole el stack como frame
    mov r0, sp
    bl timer_irq_handler
 
    @ --- REGRESO DE C: la caja trae los datos del nuevo proceso ---
 
    @ 9. Sacar el SP y LR del nuevo proceso
    ldmfd sp!, {r2, r3}
 
    @ 10. ZONA CRITICA: Inyectar SP y LR en el modo SVC del nuevo proceso
    mrs r1, cpsr
    bic r0, r1, #0x1F
    orr r0, r0, #0x93
    msr cpsr_c, r0
    mov sp, r2
    mov lr, r3
    msr cpsr_c, r1
 
    @ 11. *** FIX #3: Decirle al INTC que terminamos (End-Of-Interrupt) ***
    @ Sin esto el INTC no vuelve a disparar IRQs al CPU despues de la primera.
    ldr r0, =0x48200048    @ INTC_CONTROL
    mov r2, #1
    str r2, [r0]
 
    @ 12. Restaurar el SPSR del nuevo proceso
    ldmfd sp!, {r0}
    msr spsr_cxsf, r0
 
    @ 13. Restaurar R0-R12 y saltar al nuevo PC (^ restaura el modo automaticamente)
    ldmfd sp!, {r0-r12, pc}^