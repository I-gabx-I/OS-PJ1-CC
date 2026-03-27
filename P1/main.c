#include "../lib/stdio.h"

// 1. Declaramos que la funcion existe
void delay(volatile unsigned int count);

// 2. EL MAIN DEBE SER EL REY (Hasta arriba)
int main(void) {
    int n = 0;
    int counter = 0;

    while (1) {
        PRINT("----From P1: %d\n", n);
        n++;
        if (n > 9) n = 0;

        counter++;
        if (counter >= 500) {   
            PRINT("[P1 OK: sin corrupcion]\n");
            counter = 0;
        }

        delay(50000);
    }
    return 0;
}

// 3. El delay va hasta abajo, castigado.
void delay(volatile unsigned int count) {
    while(count--);
}