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

    .phys_text : ALIGN(4K)
    {
        *(.multiboot)
        *(.phys_text)
    }

    .phys_data : ALIGN(4K)
    {
        *(.phys_data)
        *(.phys_stack)
        *(.pages)
    }

    .text : ALIGN(4K)
    {
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
        *(.pages)
        *(.stack)
        *(.bss)
        *(.page_dir)
    }

     /* Kernel end */
    _kernel_end = .;

}
