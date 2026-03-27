#include <stdint.h>
#include "../hal/watchdog.h"
#include "../hal/timer.h"
#include "pcb.h"
#include "scheduler.h"
#include "../lib/stdio.h"
#include "../hal/uart.h"

// frame[0..12] = r0..r12, frame[13] = PC corregido del proceso interrumpido
void timer_irq_handler(unsigned int *frame) {
    clear_timer_interrupt();

    PRINT("Timer tick\n");

    // --- GUARDAR contexto del proceso actual ---
    if (current_process != 0) {
        pcb_t *cur = &process_table[current_process];

        // Guardar r0-r12 y PC desde el frame en el stack IRQ
        for (int i = 0; i < 13; i++) cur->r[i] = frame[i];
        cur->pc = frame[13];

        // Guardar SVC_SP: cambiamos a modo SVC brevemente para leerlo
        unsigned int svc_sp;
        asm volatile(
            "mrs r2, cpsr\n"
            "bic r3, r2, #0x1F\n"
            "orr r3, r3, #0x13\n"   
            "msr cpsr_c, r3\n"
            "mov %0, sp\n"
            "msr cpsr_c, r2\n"      
            : "=r"(svc_sp) : : "r2", "r3"
        );
        cur->sp = svc_sp;

        // Guardar CPSR del proceso (estaba en SPSR_irq al entrar al handler)
        asm volatile("mrs %0, spsr" : "=r"(cur->spsr));
    }

    // --- ELEGIR siguiente proceso ---
    schedule();
    pcb_t *next = &process_table[current_process];

    // --- RESTAURAR contexto del siguiente proceso ---

    // Poner r0-r12 y PC en el frame para que ldmfd los restaure
    for (int i = 0; i < 13; i++) frame[i] = next->r[i];
    frame[13] = next->pc;

    // Restaurar SVC_SP del siguiente proceso
    unsigned int new_sp = next->sp;
    asm volatile(
        "mrs r2, cpsr\n"
        "bic r3, r2, #0x1F\n"
        "orr r3, r3, #0x13\n"
        "msr cpsr_c, r3\n"      
        "mov sp, %0\n"          
        "msr cpsr_c, r2\n"     
        : : "r"(new_sp) : "r2", "r3"
    );

    // Poner SPSR = CPSR guardado del siguiente proceso
    // ldmfd ^ lo restaurara como CPSR al retornar
    asm volatile("msr spsr_cxsf, %0" : : "r"(next->spsr));
}

int main(void) {
    disable_watchdog();
    UART_putc('\n');
    UART_putc('V'); UART_putc('I'); UART_putc('V'); UART_putc('O');
    UART_putc('\n');

    init_scheduler();
    init_timer();

    PRINT("\n*** BARE-METAL OS INICIADO CORRECTAMENTE ***\n");
    PRINT("Esperando interrupciones del Timer...\n");

    while (1) {
        // El OS se queda en este loop infinito, el timer_irq_handler se encarga de todo
    }
    return 0;
}