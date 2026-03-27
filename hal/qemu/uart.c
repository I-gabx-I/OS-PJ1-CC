#include "../uart.h"

#define UART0_BASE      0x101F1000
#define UART_DR         (*(volatile unsigned int *)(UART0_BASE + 0x00))
#define UART_FR         (*(volatile unsigned int *)(UART0_BASE + 0x18))
#define UART_FR_TXFF    (1 << 5) // Transmit FIFO Full

void UART_putc(char c) {
    // Polling para QEMU VersatilePB (1 = Lleno/Ocupado)
    while (UART_FR & UART_FR_TXFF);
    UART_DR = c;
}