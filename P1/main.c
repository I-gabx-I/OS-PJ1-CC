// declaracion externa de la funcion de tu libreria
extern void PRINT(const char *fmt, ...);

// funcion de retardo basica gastando ciclos de reloj
void delay(volatile unsigned int count) {
    while(count--) {
        // el compilador no optimiza esto por el 'volatile'
    }
}

int main(void) {
    int n = 0;
    
    // ciclo infinito del proceso 1
    while (1) {
        PRINT("----From P1: %d\n", n); // formato exacto del pdf
        
        n++;
        if (n > 9) {
            n = 0; // reiniciar contador
        }
        
        // retardo para no inundar la terminal antes del context switch
        delay(500000); 
    }
    
    return 0; // por arquitectura nunca se llega aqui
}