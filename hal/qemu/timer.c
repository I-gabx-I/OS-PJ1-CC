#include "../timer.h"

// Direcciones de memoria para el controlador VIC y Timer SP804 (QEMU VersatilePB)
#define VIC_BASE        0x10140000
#define VIC_INTENABLE   (*(volatile unsigned int *)(VIC_BASE + 0x010)) 
#define VIC_VECTADDR    (*(volatile unsigned int *)(VIC_BASE + 0x030)) 

#define TIMER0_BASE     0x101E2000
#define TIMER0_LOAD     (*(volatile unsigned int *)(TIMER0_BASE + 0x00))
#define TIMER0_CTRL     (*(volatile unsigned int *)(TIMER0_BASE + 0x08))
#define TIMER0_INTCLR   (*(volatile unsigned int *)(TIMER0_BASE + 0x0C))
#define VIC_TIMER0_BIT  (1 << 4) // Bit correspondiente al Timer 0 en el VIC

// --- CONFIGURACION DEL QUANTUM (ROUND-ROBIN) ---
// Para cambiar el tiempo del context switch, modifique QUANTUM_MS.
#define QUANTUM_MS 1000            // Tiempo asignado a cada proceso (en milisegundos)
#define CLOCK_FREQ 1000000         // Frecuencia aproximada del reloj en QEMU (1 MHz)

// Macro de conversion: El SP804 cuenta hacia abajo, por lo que cargamos los ticks directamente.
#define CALC_TICKS(ms) (ms * (CLOCK_FREQ / 1000))

void init_timer(void) {
    TIMER0_CTRL = 0; // Detiene el timer para configuracion
    VIC_INTENABLE = VIC_TIMER0_BIT; // Habilita la linea del timer en el controlador de interrupciones
    
    // Carga el valor inicial basado en el Quantum definido
    TIMER0_LOAD = CALC_TICKS(QUANTUM_MS); 
    
    // Configuracion de bits: Start (7), Auto-reload (6), Enable Interrupt (5), 32-bit mode (1)
    TIMER0_CTRL = (1 << 7) | (1 << 6) | (1 << 5) | (1 << 1); 
}

void clear_timer_interrupt(void) {
    TIMER0_INTCLR = 1; // Limpia la bandera del Timer
    VIC_VECTADDR = 0;  // Notifica al VIC que la rutina de interrupcion ha finalizado
}