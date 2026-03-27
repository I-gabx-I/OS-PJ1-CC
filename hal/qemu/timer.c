#include "../timer.h"

// direcciones quemu
#define VIC_BASE        0x10140000
#define VIC_INTENABLE   (*(volatile unsigned int *)(VIC_BASE + 0x010)) 
#define VIC_VECTADDR    (*(volatile unsigned int *)(VIC_BASE + 0x030)) 

#define TIMER0_BASE     0x101E2000
#define TIMER0_LOAD     (*(volatile unsigned int *)(TIMER0_BASE + 0x00))
#define TIMER0_CTRL     (*(volatile unsigned int *)(TIMER0_BASE + 0x08))
#define TIMER0_INTCLR   (*(volatile unsigned int *)(TIMER0_BASE + 0x0C))
#define VIC_TIMER0_BIT  (1 << 4)

// quantum
#define QUANTUM_MS 1000            // Tiempo en milisegundos
#define CLOCK_FREQ 1000000         // 1 MHz aproximado en QEMU
// Formula: El SP804 cuenta hacia abajo, cargamos el valor directo
#define CALC_TICKS(ms) (ms * (CLOCK_FREQ / 1000))

void init_timer(void) {
    TIMER0_CTRL = 0; // Apagar timer
    VIC_INTENABLE = VIC_TIMER0_BIT; // Habilitar timer en el VIC (INTC de QEMU)
    
    // Inyectamos la matematica del Quantum
    TIMER0_LOAD = CALC_TICKS(QUANTUM_MS); 
    
    // Auto-reload (bit 6), enable interrupt (bit 5), 32-bit (bit 1), start (bit 7)
    TIMER0_CTRL = (1 << 7) | (1 << 6) | (1 << 5) | (1 << 1); 
}

void clear_timer_interrupt(void) {
    TIMER0_INTCLR = 1; // Limpiar bandera del Timer
    VIC_VECTADDR = 0;  // Avisar al VIC que ya procesamos la interrupcion
}