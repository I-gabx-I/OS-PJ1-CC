#include "../timer.h"

// Direcciones beagle
#define TIMER2_BASE 0x48040000
#define TIMER2_TISR (*((volatile unsigned int *)(TIMER2_BASE + 0x28))) 
#define TIMER2_TIER (*((volatile unsigned int *)(TIMER2_BASE + 0x2C))) 
#define TIMER2_TCLR (*((volatile unsigned int *)(TIMER2_BASE + 0x38))) 
#define TIMER2_TLDR (*((volatile unsigned int *)(TIMER2_BASE + 0x40))) 
#define TIMER2_TCRR (*((volatile unsigned int *)(TIMER2_BASE + 0x3C))) 

#define INTC_BASE 0x48200000
#define INTC_MIR_CLEAR2 (*((volatile unsigned int *)(INTC_BASE + 0xA8)))

// quantum
#define QUANTUM_MS 1000            // Tiempo en milisegundos
#define CLOCK_FREQ 24000000        // 24 MHz
// Formula: El DMTimer2 cuenta hacia arriba, restamos del tope
#define CALC_TICKS(ms) (0xFFFFFFFF - ((ms * (CLOCK_FREQ / 1000))))

void init_timer(void) {
    INTC_MIR_CLEAR2 = (1 << 4);  // Desenmascarar IRQ 68

    TIMER2_TCLR = 0;             // Apagar
    
    // Inyectamos la matematica del Quantum
    TIMER2_TLDR = CALC_TICKS(QUANTUM_MS); 
    TIMER2_TCRR = CALC_TICKS(QUANTUM_MS); 
    
    TIMER2_TISR = 0x7;           // Limpiar interrupciones pendientes
    TIMER2_TIER = 0x2;           // Habilitar interrupcion por desbordamiento
    TIMER2_TCLR = 0x3;           // Auto-reload y encender
}

void clear_timer_interrupt(void) {
    TIMER2_TISR = 0x2; // Bajar la bandera del hardware
}