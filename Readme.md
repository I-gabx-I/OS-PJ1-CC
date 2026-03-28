Instrucciones de Construcción y Ejecución (BeagleBone Black)
Dado que el sistema no cuenta con MMU, los binarios deben cargarse en direcciones de memoria físicas específicas definidas durante el link time.

1. Compilación
Para limpiar construcciones anteriores y generar los binarios físicos:

Bash
make clean
make

Esto generará los archivos os.bin, p1.bin y p2.bin en la carpeta build/. (Nota: Para compilar el simulador, use make TARGET=QEMU).
2. Carga en Memoria (U-Boot vía YMODEM)
Conecte la BeagleBone por puerto serial (115200 baudios). Detenga el arranque automático para ingresar al prompt de U-Boot y ejecute los siguientes comandos cargando los archivos respectivos:


Cargar el OS (64KB max):
U-Boot# loady 0x82000000
(Enviar os.bin usando YMODEM)
Cargar el Proceso 1:
U-Boot# loady 0x82100000
(Enviar p1.bin usando YMODEM)
Cargar el Proceso 2:
U-Boot# loady 0x82200000
(Enviar p2.bin usando YMODEM)
3. Ejecución
Para arrancar el Sistema Operativo, transfiera el control a la dirección base del OS:

Plaintext
U-Boot# go 0x82000000
Arquitectura y Resolución de Problemas (Historial de Desarrollo)
Durante el desarrollo de este OS Bare-Metal, nos enfrentamos a desafíos arquitectónicos complejos típicos de la programación a bajo nivel en arquitecturas ARMv7. A continuación, se documentan los problemas críticos y sus soluciones:

1. Refactorización Multi-Plataforma (BSP)
Problema: Necesitábamos probar el sistema sin depender del hardware físico en todo momento.

Solución: Se implementó una capa HAL (Hardware Abstraction Layer) separando hal/beagle/ y hal/qemu/. El Makefile se reescribió para inyectar la variable $(HW_DIR) dinámicamente. Además, en el scheduler.c, se implementó la directiva #ifdef QEMU_MODE para manejar automáticamente las direcciones de los bloques de memoria sin "quemar" (hardcodear) datos incompatibles en la lógica del OS.
2. Aislamiento del Context Switch (El "Frankenstein" de C)
Problema: Originalmente, el guardado de los registros (SP y LR) se realizaba inyectando código ensamblador (asm volatile) directamente dentro de la función timer_irq_handler en C. Esto rompía la regla de aislamiento, ya que el compilador GCC podía aplastar registros antes de que el contexto fuera guardado.

Solución: Se extrajo toda la manipulación de registros del archivo os.c. El Context Switch se reescribió puramente en el archivo root.s. Ahora, el ensamblador salva el estado, cambia temporalmente al modo SVC (con interrupciones I=1 desactivadas para evitar reentrancia), rescata el SP y LR del proceso, y finalmente pasa el Frame completo a C.
3. Colapso Silencioso de Pila (Data Abort en IRQ)
Problema: Al ejecutar el sistema en la BeagleBone física, el OS arrancaba perfectamente, pero al primer tick del Timer, el procesador se congelaba.

Solución: Descubrimos dos omisiones críticas de la arquitectura ARM:

VBAR: El CPU saltaba a la dirección 0x00000018 por defecto. Se corrigió escribiendo la dirección de la etiqueta start en el Vector Base Address Register (VBAR) a través del coprocesador CP15.

IRQ Stack: El modo IRQ no tenía su propia pila inicializada. Se modificó el os.ld para crear una región dedicada (_irq_stack_top = 0x82010000) y se inicializó en root.s antes de habilitar las interrupciones.
4. El Controlador de Interrupciones "Sordo" (INTC)
Problema: El sistema solo lograba hacer un cambio de contexto (P1) y luego se detenía indefinidamente.


Solución: Se corrigió un error en el mapa de memoria del INTC (se estaba apuntando al registro CLEAR1 en 0xA8 en lugar del CLEAR2 en 0xC8 para el IRQ 68). Además, se inyectó una instrucción crítica al final del manejador de interrupciones en ensamblador para enviar la señal End-Of-Interrupt (EOI) al registro de control del INTC (0x48200048), permitiendo que el hardware liberara la línea para futuros ticks del Timer.