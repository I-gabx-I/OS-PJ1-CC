#include "../uart.h"

#define UART0_BASE 0x44E09000
#define UART0_THR  (*((volatile unsigned int *)(UART0_BASE + 0x00)))
#define UART0_LSR  (*((volatile unsigned int *)(UART0_BASE + 0x14)))

void UART_putc(char c) {
    // Polling para BeagleBone (0 = Ocupado)
    while ((UART0_LSR & (1 << 5)) == 0); 
    UART0_THR = c;
}