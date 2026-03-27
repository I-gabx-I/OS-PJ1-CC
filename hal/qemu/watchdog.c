#include "../watchdog.h"

void disable_watchdog(void) {
    // QEMU VersatilePB no emula un watchdog por defecto que nos reinicie la placa.
    // Dejamos la funcion vacia para que el main() no tire error de compilacion.
}