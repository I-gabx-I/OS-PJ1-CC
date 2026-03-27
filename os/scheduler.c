#include "scheduler.h"

pcb_t process_table[3];
int current_process = 0;

void init_scheduler(void) {
    // PROCESO 1
    process_table[1].pid   = 1;
    process_table[1].state = READY;
    process_table[1].pc    = 0x00100000;  // segun qemu_p1.ld
    process_table[1].sp    = 0x00102000;  // top of P1 stack
    process_table[1].lr    = 0x00100000;
    process_table[1].spsr  = 0x00000013;  // SVC mode, IRQs habilitadas
    for (int i = 0; i < 13; i++) process_table[1].r[i] = 0;

    // PROCESO 2
    process_table[2].pid   = 2;
    process_table[2].state = READY;
    process_table[2].pc    = 0x00200000;  // segun qemu_p2.ld
    process_table[2].sp    = 0x00202000;
    process_table[2].lr    = 0x00200000;
    process_table[2].spsr  = 0x00000013;
    for (int i = 0; i < 13; i++) process_table[2].r[i] = 0;

    // OS = proceso 0
    process_table[0].pid   = 0;
    process_table[0].state = RUNNING;
    current_process = 0;
}

void schedule(void) {
    int next = (current_process == 1) ? 2 : 1;
    process_table[current_process].state = READY;
    process_table[next].state = RUNNING;
    current_process = next;
}