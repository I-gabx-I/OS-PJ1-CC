#include <stdint.h>
#include "../hal/watchdog.h"
#include "../hal/timer.h"
#include "pcb.h"
#include "scheduler.h"
#include "../lib/stdio.h"
#include "../hal/uart.h"

void timer_irq_handler(unsigned int *frame) {
    clear_timer_interrupt();
    PRINT("Timer tick\n");

    static int os_started = 0;

    // --- 1. GUARDAR contexto del proceso actual ---
    if (os_started == 1) {
        pcb_t *cur = &process_table[current_process];

        // Guardar r0-r12 y PC desde el frame
        for (int i = 0; i < 13; i++) cur->r[i] = frame[i];
        cur->pc = frame[13];

        // Leer SVC_SP y SVC_LR del proceso interrumpido
        unsigned int svc_sp, svc_lr;
        asm volatile(
            "mrs r2, cpsr\n"
            "bic r3, r2, #0x1F\n"
            "orr r3, r3, #0x13\n"
            "msr cpsr_c, r3\n"      
            "mov %0, sp\n"          
            "mov %1, lr\n"          // <--- RECUPERAMOS EL LR QUE SE PERDIO
            "msr cpsr_c, r2\n"     
            : "=r"(svc_sp), "=r"(svc_lr) : : "r2", "r3"
        );
        cur->sp = svc_sp;
        cur->lr = svc_lr;

        // Guardar SPSR
        asm volatile("mrs %0, spsr" : "=r"(cur->spsr));
        
        PRINT("[DBG] Saving P%d: pc=0x%x sp=0x%x\n", cur->pid, cur->pc, cur->sp);
    }

    // --- 2. ELEGIR siguiente proceso ---
    schedule();
    pcb_t *next = &process_table[current_process];

    // --- IMPRIMIMOS EL LOG AHORA (Antes de tocar la memoria sagrada) ---
    PRINT("[DBG] Restoring P%d: pc=0x%x sp=0x%x\n", next->pid, next->pc, next->sp);

    // --- 3. RESTAURAR contexto del siguiente proceso ---
    // (A partir de esta linea, C no deberia llamar a ninguna otra funcion compleja)

    // Escribir SVC_SP y SVC_LR del siguiente proceso
    unsigned int new_sp = next->sp;
    unsigned int new_lr = next->lr; // <--- RESTAURAMOS EL LR
    asm volatile(
        "mrs r2, cpsr\n"
        "bic r3, r2, #0x1F\n"
        "orr r3, r3, #0x13\n"
        "msr cpsr_c, r3\n"      
        "mov sp, %0\n"         
        "mov lr, %1\n"         
        "msr cpsr_c, r2\n"      
        : : "r"(new_sp), "r"(new_lr) : "r2", "r3"
    );

    // Restaurar SPSR
    asm volatile("msr spsr_cxsf, %0" : : "r"(next->spsr));

    // Poner r0-r12 y PC del siguiente proceso en el frame
    // ¡HACEMOS ESTO HASTA EL FINAL PARA QUE C NO LO APLASTE!
    for (int i = 0; i < 13; i++) frame[i] = next->r[i];
    frame[13] = next->pc;

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