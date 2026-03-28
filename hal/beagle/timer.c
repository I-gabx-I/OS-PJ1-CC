#include "../timer.h"

// Direcciones de memoria para los perifericos de la BeagleBone (AM335x)
#define TIMER2_BASE 0x48040000
#define TIMER2_TISR (*((volatile unsigned int *)(TIMER2_BASE + 0x28))) // Timer Interrupt Status Register
#define TIMER2_TIER (*((volatile unsigned int *)(TIMER2_BASE + 0x2C))) // Timer Interrupt Enable Register
#define TIMER2_TCLR (*((volatile unsigned int *)(TIMER2_BASE + 0x38))) // Timer Control Register
#define TIMER2_TLDR (*((volatile unsigned int *)(TIMER2_BASE + 0x40))) // Timer Load Register
#define TIMER2_TCRR (*((volatile unsigned int *)(TIMER2_BASE + 0x3C))) // Timer Counter Register

#define INTC_BASE 0x48200000
#define INTC_MIR_CLEAR2 (*((volatile unsigned int *)(INTC_BASE + 0xC8))) // Desenmascara IRQs 64-95
#define INTC_CONTROL    (*((volatile unsigned int *)(INTC_BASE + 0x48))) // Control general del INTC

// --- CONFIGURACION DEL QUANTUM (ROUND-ROBIN) ---
// Para cambiar el tiempo del context switch, modifique QUANTUM_MS.
#define QUANTUM_MS 1000            // Tiempo asignado a cada proceso (en milisegundos)
#define CLOCK_FREQ 24000000        // Frecuencia del reloj del DMTimer2 (24 MHz)

// Macro de conversion: Calcula cuantos 'ticks' equivalen a los milisegundos solicitados.
// Como DMTimer2 cuenta hacia arriba, se resta el valor calculado del tope (0xFFFFFFFF).
#define CALC_TICKS(ms) (0xFFFFFFFF - ((ms * (CLOCK_FREQ / 1000))))

void init_timer(void) {
    INTC_CONTROL = 0x1;          // Reinicia el controlador de interrupciones para limpiar estados previos
    INTC_MIR_CLEAR2 = (1 << 4);  // Desenmascara la interrupcion 68 (Timer 2) en el INTC

    TIMER2_TCLR = 0;             // Detiene el timer para configuracion
    
    // Carga los valores de inicio y auto-recarga basados en el Quantum definido
    TIMER2_TLDR = CALC_TICKS(QUANTUM_MS); 
    TIMER2_TCRR = CALC_TICKS(QUANTUM_MS); 
    
    TIMER2_TISR = 0x7;           // Limpia banderas de interrupciones residuales
    TIMER2_TIER = 0x2;           // Habilita interrupcion especifica por desbordamiento (Overflow)
    TIMER2_TCLR = 0x3;           // Inicia el timer con la funcion de auto-recarga habilitada, nunca inicia infinito proceso
}

void clear_timer_interrupt(void) {
    TIMER2_TISR = 0x2;   // Notifica al timer que la interrupcion fue atendida ,no genera la interrupcion
    INTC_CONTROL = 0x1;  // Envia la senal End-Of-Interrupt (EOI) al controlador principal
}