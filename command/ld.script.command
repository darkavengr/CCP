ENTRY("main")
SECTIONS
{
 . = 0x100;
 .text : { *(.text) }
 .data : { *(.data) }
 .rodata : { *(.rodata) }
 .bss : { *(.bss) }

 }
