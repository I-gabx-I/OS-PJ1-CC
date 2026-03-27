#include <stdint.h>
#include "../hal/watchdog.h"
#include "../hal/timer.h"
#include "pcb.h"
#include "scheduler.h"
#include "../lib/stdio.h"
#include "../hal/uart.h" 



// print para el os
// extern void PRINT(const char *fmt, ...);



void timer_irq_handler() {
    // Le pedimos al HAL que apague la alarma del hardware
    clear_timer_interrupt(); 
    
    // Logica temporal del OS para saber que la interrupcion llego
    PRINT("\ntick-\n");
}

// Tabla de procesos y variables de control


int main() {
    // 1. Apagar el watchdog lo antes posible
    disable_watchdog();
    UART_putc('\n');
    UART_putc('V');
    UART_putc('I');
    UART_putc('V');
    UART_putc('O');
    UART_putc('\n');

    // 2. Aqui ira la inicializacion del UART0
    
    // 3. Aqui ira la inicializacion de la tabla de procesos (PCBs)
    init_scheduler();
    // 4. Aqui ira la configuracion del DMTimer2 y el INTC
    init_timer();


    PRINT("\n\n*** BARE-METAL OS INICIADO CORRECTAMENTE ***\n");
    PRINT("Esperando interrupciones del Timer...\n");
    // Ciclo infinito del OS (idle loop)
    while(1) {
        // El OS se queda esperando las interrupciones del timer
    }

    return 0;
}