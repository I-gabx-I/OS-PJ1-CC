#include <stdarg.h>
#include "../hal/uart.h" // Importamos la conexion al hardware

// Mini funcion para imprimir enteros (%d)
void UART_putint(int num) {
    char buffer[10];
    int i = 0;
    if (num == 0) {
        UART_putc('0');
        return;
    }
    if (num < 0) {
        UART_putc('-');
        num = -num;
    }
    while (num > 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }
    while (i > 0) { // Imprimir al reves
        UART_putc(buffer[--i]);
    }
}

// Mini funcion para imprimir hexadecimales (%x)
void UART_puthex(unsigned int val) {
    char hbuf[8];
    int hi = 0;
    if (val == 0) { 
        UART_putc('0'); 
        return;
    }
    while (val > 0) {
        int nibble = val & 0xF; // Extraer los 4 bits menos significativos
        hbuf[hi++] = nibble < 10 ? '0' + nibble : 'a' + (nibble - 10);
        val >>= 4;              // Recorrer 4 bits a la derecha
    }
    while (hi > 0) { // Imprimir al reves
        UART_putc(hbuf[--hi]);
    }
}

// Tu nuevo PRINT funcional
void PRINT(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++; // Saltar el '%'
            if (*fmt == 'd') {
                int val = va_arg(args, int);
                UART_putint(val);
            } else if (*fmt == 'x') {  //  HEXADECIMAL
                unsigned int val = va_arg(args, unsigned int);
                UART_puthex(val);
            } else if (*fmt == 'c') {
                char val = (char)va_arg(args, int); // char se promueve a int en varargs
                UART_putc(val);
            } else if (*fmt == 's') {
                char *str = va_arg(args, char *);
                while (*str) { UART_putc(*str++); }
            } else {
                UART_putc('%'); // Si no reconoce, imprime literal
                UART_putc(*fmt);
            }
        } else {
            UART_putc(*fmt);
        }
        fmt++;
    }
    va_end(args);
}