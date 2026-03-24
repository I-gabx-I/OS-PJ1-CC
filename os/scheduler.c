#include "scheduler.h"

// Variables globales reales
pcb_t process_table[3];
int current_process = 0;

void init_scheduler(void) {
    // ---------------------------------------------------------
    // PROCESO 1 
    // ---------------------------------------------------------
    process_table[1].pid = 1;
    process_table[1].state = READY;
    
    // Le damos su direccion de inicio 
    process_table[1].pc = 0x82100000; 
    
    // Configuramos su Stack Pointer inicial
    // Segun  p1.ld, el stack de P1 termina en 0x82110000 + 0x2000
    process_table[1].sp = 0x82112000; 
    
    // Modo de procesador: User mode (0x10) con interrupciones habilitadas
    process_table[1].spsr = 0x10; 
    
    // Llenamos sus registros R0-R12 con ceros para que nazca limpio
    for(int i=0; i<13; i++){
        process_table[1].r[i] = 0;
    }

    // ---------------------------------------------------------
    // PROCESO 2 (P2)
    // ---------------------------------------------------------
    process_table[2].pid = 2;
    process_table[2].state = READY;
    
    // Su main empieza aqui segun p2.ld
    process_table[2].pc = 0x82200000; 
    
    // Stack Pointer inicial para P2
    // Segun tu p2.ld, el stack de P2 termina en 0x82210000 + 0x2000
    process_table[2].sp = 0x82212000; 
    
    process_table[2].spsr = 0x10; 
    
    for(int i=0; i<13; i++){
        process_table[2].r[i] = 0;
    }

    // El proceso 0 es el OS mismo (el idle loop). Nace como RUNNING.
    process_table[0].pid = 0;
    process_table[0].state = RUNNING;
    current_process = 0;
}