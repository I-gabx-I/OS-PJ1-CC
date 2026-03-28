#include <stdint.h>
#include "../hal/watchdog.h"
#include "../hal/timer.h"
#include "pcb.h"
#include "scheduler.h"
#include "../lib/stdio.h"
#include "../hal/uart.h"

void timer_irq_handler(unsigned int *frame) {
    clear_timer_interrupt();
    PRINT("Interrupción del Timer recibida\n"); // Apagamos el hardware para que no siga sonando

    static int os_started = 0;

    // --- 1. GUARDAR contexto del proceso actual ---
    if (os_started == 1) {
        pcb_t *cur = &process_table[current_process];

        // El ensamblador nos paso la caja (frame) ordenada asi:
        cur->sp   = frame[0];  // SVC_SP
        cur->lr   = frame[1];  // SVC_LR
        cur->spsr = frame[2];  // SPSR
        for (int i = 0; i < 13; i++) cur->r[i] = frame[3 + i]; // R0-R12
        cur->pc   = frame[16]; // PC
    }

    // --- 2. ELEGIR siguiente proceso ---
    schedule();
    pcb_t *next = &process_table[current_process];

    // --- 3. RESTAURAR contexto del siguiente proceso ---
    // (Metemos las cosas del nuevo proceso en la caja para el ensamblador)
    frame[0] = next->sp;
    frame[1] = next->lr;
    frame[2] = next->spsr;
    for (int i = 0; i < 13; i++) frame[3 + i] = next->r[i];
    frame[16] = next->pc;

    os_started = 1;
}
int main(void) {
    disable_watchdog();
    UART_putc('\n');
    UART_putc('V'); UART_putc('I'); UART_putc('V'); UART_putc('O');
    UART_putc('\n');

    // init_scheduler ya configura todo correctamente, no repetir aqui
    init_scheduler();
    init_timer();

    PRINT("\n*** BARE-METAL OS INICIADO CORRECTAMENTE ***\n");
    PRINT("Esperando interrupciones del Timer...\n");

    while (1) {}
    return 0;
}