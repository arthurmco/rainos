/***************************************
    This file is part of RainOS

    The linker
***************************************/

/* Entry point */
ENTRY(_kernel_entry)

/* File sections */
SECTIONS {
    /* 
        Begin sections at 1 MiB.
        This is where GRUB loads your code.
     */
     . = 1M;

    /* Kernel start */
    _kernel_start = .;

    .text : ALIGN(4K) 
    {
        *(.multiboot)
        *(.text)
    }

    /* Read-only data. */
    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    /* Read-write data (initialized) */
    .data : ALIGN(4K)
    {
        *(.data)
    }

    /* Read-write data (uninitialized) and stack */
    .bss : ALIGN(4K) 
    {
        *(COMMON)
        *(.bss)
        *(.stack)
        *(.page_dir)
    }

     /* Kernel end */
    _kernel_end = .;

}