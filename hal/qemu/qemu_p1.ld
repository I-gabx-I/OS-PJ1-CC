ENTRY(main)
SECTIONS
{
    . = 0x00100000;
    .text : { *(.text) }
    .data : { *(.data) }
    .bss  : { *(.bss) *(COMMON) }
    . = ALIGN(4);
    . = . + 0x2000;
    _p1_stack_top = .;
}