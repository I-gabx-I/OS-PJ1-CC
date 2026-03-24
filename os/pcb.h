#ifndef PCB_H
#define PCB_H

// Estados del proceso
#define READY 0
#define RUNNING 1

// Estructura del PCB
typedef struct {
    unsigned int pid;
    unsigned int state;
    unsigned int sp;
    unsigned int lr;
    unsigned int pc;
    unsigned int spsr;
    unsigned int r[13];
} pcb_t;

#endif