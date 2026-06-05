ENTRY(start) /* Asumiendo que tu punto de entrada en root.s se llama start */

SECTIONS
{
    /* El OS arranca en esta dirección base */
    . = 0x82000000; 

    .text : {
        *(.text)
    }

    .data : {
        *(.data)
    }

    /* Definimos los límites del BSS para limpiarlo en root.s */
    . = ALIGN(4);
    bss_start = .;
    .bss : {
        *(.bss)
        *(COMMON)
    }
    . = ALIGN(4);
    bss_end = .;

    /* El stack del OS empieza en esta dirección (crece hacia abajo) */
    _os_stack_top = 0x82010000 + 0x2000; /* 0x82012000, 8KB */

    /* Stack del modo IRQ - separado porque ARM tiene SP/LR bancados por modo */
    _irq_stack_top = 0x82010000;

    /* PHASE 2: System-mode / idle stack (USR-banked SP).
       Placed above the OS SVC stack, well below P1 base (0x82100000). */
    _sys_stack_top = 0x82012000 + 0x2000; /* 0x82014000, 8KB */

    /* PHASE 2 (faults): per-mode stacks for Abort and Undefined modes. */
    _abt_stack_top = 0x82014000 + 0x1000; /* 0x82015000, 4KB */
    _und_stack_top = 0x82015000 + 0x1000; /* 0x82016000, 4KB */
}
