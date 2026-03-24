.global start
start:
    @ 1. Leer el tope del stack dinamico desde tu linker script
    ldr sp, =_os_stack_top

    @ 2. Limpiar la seccion .bss usando las variables exactas de tu os.ld
    ldr r0, =bss_start
    ldr r1, =bss_end
    mov r2, #0
clear_bss:
    cmp r0, r1
    bge bss_cleared
    str r2, [r0], #4
    b clear_bss
bss_cleared:
    
    @ 3. Barrera de memoria recomendada por el PDF
    dsb
    isb

    @ 4. Saltar a tu funcion main() en os.c
    bl main

hang:
    @ Si el main llega a terminar (no deberia), nos quedamos atrapados aqui
    b hang