ENTRY(main)
SECTIONS
{
    . = 0x00200000;
    .text : { *(.text) }
    .data : { *(.data) }
    .bss  : { *(.bss) *(COMMON) }
    . = ALIGN(4);
    . = . + 0x2000;
    _p2_stack_top = .;
}