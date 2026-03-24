#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "pcb.h"

// La palabra 'extern' es clave: avisa que estas variables existen, pero no las crea aqui.
extern pcb_t process_table[3];
extern int current_process;

// Funciones del scheduler
void init_scheduler(void);

#endif