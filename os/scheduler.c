#include "scheduler.h"

pcb_t process_table[3];
int current_process = 0;

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
    process_table[1].pc    = 0x00100000;  // segun qemu_p1.ld
    process_table[1].sp    = 0x00102000;  // top of P1 stack
    process_table[1].lr    = 0x00100000;
    process_table[1].spsr  = 0x13;        // SVC mode, IRQs habilitadas
    for (int i = 0; i < 13; i++) process_table[1].r[i] = 0;

    // P2 = proceso 2 (letras)
    process_table[2].pid   = 2;
    process_table[2].state = READY;
    process_table[2].pc    = 0x00200000;  // segun qemu_p2.ld
    process_table[2].sp    = 0x00202000;  // top of P2 stack
    process_table[2].lr    = 0x00200000;
    process_table[2].spsr  = 0x13;
    for (int i = 0; i < 13; i++) process_table[2].r[i] = 0;

    current_process = 0;
}

void schedule(void) {
    // OS(0) -> P1(1) -> P2(2) -> P1(1) -> P2(2) ...
    int next;
    if (current_process == 0)      next = 1;  // primer tick: arrancar P1
    else if (current_process == 1) next = 2;  // P1 -> P2
    else                           next = 1;  // P2 -> P1

    process_table[current_process].state = READY;
    process_table[next].state = RUNNING;
    current_process = next;
}