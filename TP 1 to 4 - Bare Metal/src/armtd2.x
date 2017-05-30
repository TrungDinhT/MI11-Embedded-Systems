OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm",
	      "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
  . = 0x0;
  .text           : {
  		*(.vector)
  		*(.text)
  }
  .rodata           : {
  		*(.rodata)
  		. = ALIGN(4);
   }
  rodata_end = LOADADDR(.rodata) + SIZEOF(.rodata);
  .data		0x008000000 : AT(rodata_end) {*(.data) }
  data_VMA_start = ADDR(.data);
  data_start = LOADADDR(.data);
  data_end = data_start + SIZEOF(.data);
  .bss           :
  { 
  	*(.bss)
  	*(COMMON) 
  }
  bss_start = ADDR(.bss);
  bss_end = bss_start + SIZEOF(.bss);
  .hash           : { *(.hash) }
}
