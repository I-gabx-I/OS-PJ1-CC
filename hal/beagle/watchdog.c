#include "../watchdog.h"

#define WDT_BASE 0x44E35000
#define WDT_WSPR (*((volatile unsigned int *)(WDT_BASE + 0x48)))
#define WDT_WWPS (*((volatile unsigned int *)(WDT_BASE + 0x34)))

void disable_watchdog(void) {
    WDT_WSPR = 0xAAAA;
    while (WDT_WWPS != 0);
    WDT_WSPR = 0x5555;
    while (WDT_WWPS != 0);
}