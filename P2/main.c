/*
 * P2 — user process (USR mode). Uses ONLY syscalls.
 *  1. Runs a few negative-path write checks (bad fd, bad pointer).
 *  2. Loops: write a letter line, then yield.
 *  3. After EXIT_AFTER full a-z cycles, calls sys_exit(0).
 * This shows write (valid + error codes), yield, and exit; afterwards
 * P1 keeps running alone (scheduler skips the terminated P2).
 */
#include "../lib/user_syscalls.h"
 
#define EXIT_AFTER 3          /* full alphabet cycles before exit */
 
void delay(volatile unsigned int count);
 
__attribute__((section(".text.boot")))
int main(void) {
    /* --- Negative-path checks (PDF Example B) --- */
    (void)sys_write(9, "X\n", 2);                 /* bad fd  -> rc=-2 */
    (void)sys_write(1, (void *)0xFFFFFFFF, 8);    /* bad ptr -> rc=-3 */
 
    char line[16];
    line[0]='-'; line[1]='-'; line[2]='-'; line[3]='-';
    line[4]='F'; line[5]='r'; line[6]='o'; line[7]='m';
    line[8]=' '; line[9]='P'; line[10]='2'; line[11]=':';
    line[12]=' '; line[13]='a'; line[14]='\n'; line[15]=0;
 
    char c = 'a';
    int cycles = 0;
    while (1) {
        line[13] = c;
        sys_write(1, line, 15);
 
        if (c == 'z') { c = 'a'; if (++cycles >= EXIT_AFTER) sys_exit(0); }
        else          { c++; }
 
        sys_yield();
        delay(20000);
    }
    return 0;
}
 
void delay(volatile unsigned int count) { while (count--); }
