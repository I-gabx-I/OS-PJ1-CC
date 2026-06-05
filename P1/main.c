/*
 * P1 — user process (USR mode). Uses ONLY syscalls; never touches HW.
 * Repeatedly: write a digit line, then yield (cooperative scheduling).
 * Runs forever -> demonstrates "write + yield without instability".
 */
#include "../lib/user_syscalls.h"
 
void delay(volatile unsigned int count);
 
__attribute__((section(".text.boot")))
int main(void) {
    /* Pre-rendered lines "----From P1: d\n" — built once, in P1's region */
    char line[16];
    line[0]='-'; line[1]='-'; line[2]='-'; line[3]='-';
    line[4]='F'; line[5]='r'; line[6]='o'; line[7]='m';
    line[8]=' '; line[9]='P'; line[10]='1'; line[11]=':';
    line[12]=' '; line[13]='0'; line[14]='\n'; line[15]=0;
 
    int n = 0;
    while (1) {
        line[13] = (char)('0' + n);
        sys_write(1, line, 15);
        n = (n + 1) % 10;
 
        sys_yield();          /* cooperative: give the CPU to a peer */
        delay(20000);         /* small gap so the timer can also preempt */
    }
    return 0;
}
 
void delay(volatile unsigned int count) { while (count--); }
