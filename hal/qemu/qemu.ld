ENTRY(start)
SECTIONS
{
    . = 0x00000000;     /* Vectores en la direccion correcta para ARM */

    .text : {
        *(.text.boot)
        *(.text)
    }
    .data : { *(.data) }

    . = ALIGN(4);
    bss_start = .;
    .bss : { *(.bss) *(COMMON) }
    . = ALIGN(4);
    bss_end = .;

    . = ALIGN(8);
    . = . + 0x1000;
    _irq_stack_top = .;
    . = . + 0x2000;
    _os_stack_top = .;
}