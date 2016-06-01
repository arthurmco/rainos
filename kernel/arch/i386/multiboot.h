/* Multiboot structure definition

    Copyright 2015-2016 Arthur M */

#include <stdint.h>

#ifndef _MULTIBOOT_H
#define _MULTIBOOT_H

typedef struct mboot_info {
    uint32_t flags;

    uint32_t mem_lower;  //Memory below 640k hole
    uint32_t mem_upper;  //Memory above 640k hole

    uint32_t boot_device;    /* Boot device. Only present if booted from a BIOS-recognized disk
                            The first u8 is the drive
                            The second is the partition index
                            The third is the subpartition index, or 0xff if no subpartition
                            The fourth is the sub-subpartition index/0xff if no sub-subpartition */

    char* cmdline;  /* The command-line used to boot the system */

    uint32_t modules_count;
    uint32_t modules_addr;       /* Address to a list of modules loaded by bootloader */

    /*  Since this will be generated as an ELF file, I will show only the
        ELF header pointers generated by multiboot, not the a.out ones */
    struct {
        uint32_t num_entries;
        uint32_t entry_size;
        uint32_t entry_addr;
        uint32_t shdr_pointer;    /* Pointer to the ELF string table */
    } elf_data;

    uint32_t mmap_length;    /* Length of the memory map fields */
    uint32_t mmap_addr;      /* Address of the mmap fields */

    uint32_t drives_length;
    uint32_t drives_addr;

    uint32_t config_table;

    char* loader_name;

} multiboot_t;

typedef struct {
    uint32_t size;
    uint32_t addr_low;
    uint32_t addr_high;
    uint32_t len_low;
    uint32_t len_high;
    uint32_t type;
}  multiboot_mmap_t;



#endif /* end of include guard: _MULTIBOOT_H */
