SECTIONS {
  . = 0x7C00;
  .mbr16 : { *(.mbr16) }
  . = 0x8000;
  .entry16 : { *(.entry16) }
  .entry32 : { *(.entry32) }
  .text : { *(.text) }
  .data : { *(.data) }
  .bss : { *(.bss) }
}
