#include "../lib/stdio.h"

// funcion de retardo basica
void delay(volatile unsigned int count) {
    while(count--) {
    }
}

int main(void) {
    char c = 'a';
    
    // ciclo infinito del proceso 2
    while (1) {
        PRINT("----From P2: %c\n", c); // formato exacto del pdf
        
        c++;
        if (c > 'z') {
            c = 'a'; // reiniciar alfabeto
        }
        
        // retardo similar al de P1
        delay(500000); 
    }
    
    return 0;
}