.global start

@ Tabla de Vectores de Excepcion ARM
start:
    b reset_handler      @ 0x00: Reset del sistema
    b hang               @ 0x04: Instruccion Indefinida
    b hang               @ 0x08: Interrupcion por Software (SWI/SVC)
    b hang               @ 0x0C: Aborto de Prefetch
    b hang               @ 0x10: Aborto de Datos
    b hang               @ 0x14: Reservado
    b irq_handler        @ 0x18: Peticion de Interrupcion (IRQ) - Timer
    b hang               @ 0x1C: Interrupcion Rapida (FIQ)

reset_handler:
    @ Reubicacion de la Tabla de Vectores (VBAR)
    @ Define la direccion base de la tabla de vectores apuntando a 'start'
    ldr r0, =start
    mcr p15, 0, r0, c12, c0, 0

    @ Inicializacion del Stack Pointer para el modo IRQ
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0xD2       @ Cambio a modo IRQ (0xD2)
    msr cpsr_c, r0
    ldr sp, =_irq_stack_top @ escribo basura en otro lado

    @ Inicializacion del Stack Pointer para el modo SVC (Supervisor)
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0xD3       @ Cambio a modo SVC (0xD3)
    msr cpsr_c, r0
    ldr sp, =_os_stack_top  @guardo basura en otro lado

    @ Limpieza de la seccion .bss (Variables globales a cero)
    ldr r0, =bss_start
    ldr r1, =bss_end
    mov r2, #0
clear_bss:
    cmp r0, r1
    bge bss_cleared  @bucle infinito
    str r2, [r0], #4
    b   clear_bss
bss_cleared:

    @ Habilitacion de Interrupciones a nivel de CPU
    mrs r0, cpsr
    bic r0, r0, #0x80       @ Limpieza del bit 'I' , no context switch
    msr cpsr_c, r0

    @ Barreras de sincronizacion de memoria (ARMv7)
    dsb @asegura limpieza fin e inicio luego main 
    isb

    @ Salto a la logica principal en C
    bl main

hang:
    b hang

@ Manejador de Interrupciones (Context Switch)

irq_handler:
    @ Ajuste del Program Counter para retornar a la instruccion interrumpida
    sub lr, lr, #4

    @ Guardado del contexto base (R0-R12 y PC) en el stack de IRQ
    stmfd sp!, {r0-r12, lr}

    @ Guardado del Status Register (CPSR del proceso interrumpido)
    mrs r0, spsr
    stmfd sp!, {r0}

    @ Cambio a modo SVC con interrupciones deshabilitadas (Prevencion de reentrancia)
    mrs r1, cpsr
    bic r0, r1, #0x1F
    orr r0, r0, #0x93       @crash fatal del user
    msr cpsr_c, r0

    @ Rescate de los punteros de pila y retorno del proceso de usuario
    mov r2, sp
    mov r3, lr

    @ Retorno al modo IRQ
    msr cpsr_c, r1

    @ Integracion de SP y LR al frame de interrupcion
    stmfd sp!, {r2, r3}

    @ Llamada al planificador en C (pasando el Stack Pointer como argumento)
    mov r0, sp
    bl timer_irq_handler
    
    @ --- Retorno del Planificador ---

    @ Extraccion del SP y LR del nuevo proceso a ejecutar
    ldmfd sp!, {r2, r3}

    @ Inyeccion de los punteros en los registros bancados del modo SVC
    mrs r1, cpsr
    bic r0, r1, #0x1F
    orr r0, r0, #0x93
    msr cpsr_c, r0
    mov sp, r2
    mov lr, r3
    msr cpsr_c, r1

    @ Confirmacion de fin de interrupcion (EOI) al controlador INTC
    ldr r0, =0x48200048    
    mov r2, #1
    str r2, [r0]

    @ Restauracion del SPSR del nuevo proceso
    ldmfd sp!, {r0}
    msr spsr_cxsf, r0

    @ Restauracion de registros generales y salto al nuevo Program Counter
    ldmfd sp!, {r0-r12, pc}^