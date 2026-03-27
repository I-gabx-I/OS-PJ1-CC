#include "../lib/stdio.h"

void delay(volatile unsigned int count);

int main(void) {
    char c = 'a';
    int counter = 0;

    while (1) {
        PRINT("----From P2: %c\n", c);
        c++;
        if (c > 'z') c = 'a';

        counter++;
        if (counter >= 500) {
            PRINT("[P2 OK: sin corrupcion]\n");
            counter = 0;
        }

        delay(50000);
    }
    return 0;
}

void delay(volatile unsigned int count) {
    while(count--);
}