#pragma once
/*
 * user_syscalls.h  —  Phase 2, Section 4 syscall ABI (user side)
 *
 * User processes (P1/P2) include ONLY this header to request kernel
 * services. They never touch hardware directly anymore.
 *
 * Register ABI (ARM / SVC), per PDF Section 4.3:
 *   r0  = syscall ID on entry; return value (int32_t) on exit
 *   r1  = argument 1
 *   r2  = argument 2
 *   r3  = argument 3
 *   svc #0 = canonical trap (SVCall)
 *
 * Return convention (PDF Section 4.8):
 *   >= 0  success (byte count where applicable)
 *   -1    invalid syscall ID
 *   -2    invalid file descriptor or argument
 *   -3    invalid user pointer / protection violation
 */

#include <stdint.h>
#include <stddef.h>

/* Syscall numeric IDs (PDF Section 4.4) */
enum {
    SYS_YIELD = 0,   /* voluntary reschedule                  */
    SYS_EXIT  = 1,   /* terminate caller; does not return     */
    SYS_WRITE = 2,   /* write bytes to UART (fd must be 1)    */
};

/* Raw trap wrapper: load r0-r3, execute svc #0, return r0 */
static inline int32_t syscall3(uint32_t id,
                               uint32_t a1,
                               uint32_t a2,
                               uint32_t a3)
{
    register uint32_t r0 asm("r0") = id;
    register uint32_t r1 asm("r1") = a1;
    register uint32_t r2 asm("r2") = a2;
    register uint32_t r3 asm("r3") = a3;

    asm volatile(
        "svc #0\n"
        : "+r"(r0)
        : "r"(r1), "r"(r2), "r"(r3)
        : "memory"
    );
    return (int32_t)r0;
}

/* Yield the CPU; returns 0 on success. */
static inline int32_t sys_yield(void) {
    return syscall3(SYS_YIELD, 0, 0, 0);
}

/* Terminate the calling process; never returns. */
static inline void sys_exit(int32_t code) {
    (void)syscall3(SYS_EXIT, (uint32_t)code, 0, 0);
    while (1) { }   /* unreachable — kernel will not resume us */
}

/* Write len bytes from buf to fd (fd=1 = console). */
static inline int32_t sys_write(int32_t fd, const void *buf, size_t len) {
    return syscall3(SYS_WRITE,
                    (uint32_t)fd,
                    (uint32_t)(uintptr_t)buf,
                    (uint32_t)len);
}
