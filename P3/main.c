/*
 * P3 — user process (USR mode). Demonstrates FAULT CONTAINMENT (Sec 5).
 * Participates normally for a few iterations (write + yield), then
 * executes an undefined instruction. The kernel must classify and
 * terminate P3, then keep scheduling the healthy peers (P1, and P2
 * until it exits). P3 must never run again.
 */
#include "../lib/user_syscalls.h"

#define RUN_BEFORE_FAULT 4

void delay(volatile unsigned int count);

__attribute__((section(".text.boot")))
int main(void) {
    const char tag[] = "----From P3: alive\n";

    for (int i = 0; i < RUN_BEFORE_FAULT; i++) {
        sys_write(1, tag, sizeof(tag) - 1);
        sys_yield();
        delay(20000);
    }

    /* Announce, then trigger an illegal instruction (USR mode). */
    {
        const char boom[] = "----From P3: triggering illegal instruction\n";
        sys_write(1, boom, sizeof(boom) - 1);
    }

    /* Permanently UNDEFINED instruction (ARM UDF encoding). */
    asm volatile(".word 0xe7f000f0");

    /* Should never reach here; if isolation failed we'd see this. */
    sys_exit(99);
    return 0;
}

void delay(volatile unsigned int count) { while (count--); }
