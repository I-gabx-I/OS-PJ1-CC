#include "timer.h"

// --- Direcciones de Memoria ---
// DM TIMER2
#define TIMER2_BASE 0x48040000
#define TIMER2_TISR (*((volatile unsigned int *)(TIMER2_BASE + 0x28))) 
#define TIMER2_TIER (*((volatile unsigned int *)(TIMER2_BASE + 0x2C))) 
#define TIMER2_TCLR (*((volatile unsigned int *)(TIMER2_BASE + 0x38))) 
#define TIMER2_TLDR (*((volatile unsigned int *)(TIMER2_BASE + 0x40))) 
#define TIMER2_TCRR (*((volatile unsigned int *)(TIMER2_BASE + 0x3C))) 

// INTC (Interrupt Controller)
#define INTC_BASE 0x48200000
#define INTC_MIR_CLEAR2 (*((volatile unsigned int *)(INTC_BASE + 0xA8)))

// --- Funciones de Hardware ---
void init_timer(void) {
    // 1. Desenmascarar IRQ 68 (DMTimer2) en INTC
    INTC_MIR_CLEAR2 = (1 << 4);

    // 2. Configurar DMTimer2
    TIMER2_TCLR = 0;             
    TIMER2_TLDR = 0xFFFFFFFF - 24000000; 
    TIMER2_TCRR = 0xFFFFFFFF - 24000000; 
    
    TIMER2_TISR = 0x7;           
    TIMER2_TIER = 0x2;           
    TIMER2_TCLR = 0x3;           
}

void clear_timer_interrupt(void) {
    // Limpiar solo la interrupción de desbordamiento en el hardware
    TIMER2_TISR = 0x2; 
}