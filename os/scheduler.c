#include "scheduler.h"

pcb_t process_table[3];
int current_process = 0;

// La magia del preprocesador: Direcciones dinamicas
#ifdef QEMU_MODE
    // Direcciones del Simulador
    #define P1_ENTRY 0x00100000
    #define P1_STACK 0x00102000
    #define P2_ENTRY 0x00200000
    #define P2_STACK 0x00202000
#else
    // Direcciones Reales de la BeagleBone segun el PDF
    #define P1_ENTRY 0x82100000
    #define P1_STACK 0x82112000 // 0x82110000 + 8KB
    #define P2_ENTRY 0x82200000
    #define P2_STACK 0x82212000 // 0x82210000 + 8KB
#endif

void init_scheduler(void) {
    // OS = proceso 0 (idle loop)
    process_table[0].pid   = 0;
    process_table[0].state = RUNNING;
    process_table[0].pc    = 0;
    process_table[0].sp    = 0;
    process_table[0].spsr  = 0x13;
    for (int i = 0; i < 13; i++) process_table[0].r[i] = 0;

    // P1 = proceso 1 (numeros)
    process_table[1].pid   = 1;
    process_table[1].state = READY;
    process_table[1].pc    = P1_ENTRY;  
    process_table[1].sp    = P1_STACK;  
    process_table[1].lr    = P1_ENTRY;
    process_table[1].spsr  = 0x13;        // SVC mode, IRQs habilitadas
    for (int i = 0; i < 13; i++) process_table[1].r[i] = 0;

    // P2 = proceso 2 (letras)
    process_table[2].pid   = 2;
    process_table[2].state = READY;
    process_table[2].pc    = P2_ENTRY;  
    process_table[2].sp    = P2_STACK;  
    process_table[2].lr    = P2_ENTRY;
    process_table[2].spsr  = 0x13;
    for (int i = 0; i < 13; i++) process_table[2].r[i] = 0;

    current_process = 0;
}

void schedule(void) {
    // Round-Robin: OS(0) -> P1(1) -> P2(2) -> P1(1) -> P2(2) ...
    int next;
    if (current_process == 0)      next = 1;  
    else if (current_process == 1) next = 2;  
    else                           next = 1;  

    process_table[current_process].state = READY;
    process_table[next].state = RUNNING;
    current_process = next;
}