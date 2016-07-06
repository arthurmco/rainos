/*  ELF file format parser

    Copyright (C) 2016 Arthur M */

#include <stdint.h>
#include <kstdlog.h>
#include "vfs/vfs.h"

#ifndef _ELF_H
#define _ELF_H

#define E_NIDENT 16

enum ELFIdent {
    E_MAG0,     //0x7f
    E_MAG1,     //'E'
    E_MAG2,     //'L'
    E_MAG3,     //'F'
    E_CLASS,
    E_DATA,     // Encoding
    E_VERSION,  // ELF version
    E_PAD
};

struct ELFHeader {
    uint8_t e_ident[E_NIDENT];   /* ELF identification data */
    uint16_t type;          /* ELF type */
    uint16_t machine;       /* Machine (processor) type */
    uint32_t version;       /* File version */
    uintptr_t entry;        /* Entry point offset */
    uint32_t phoff;         /* Program header file position */
    uint32_t shoff;         /* Section header file position */
    uint32_t flags;         /* File flags */
    uint16_t ehsize;
    uint16_t phentsize, phnum;
    uint16_t shentsize, shnum;
    uint16_t shstrndx;
};

struct ELFSection {
    uint32_t name_off;      /* Name offset into the string table */
    uint32_t type;          /* Section type */
    uint32_t flags;         /* Section flags */
    uintptr_t addr;         /* Section load address */
    uint32_t off, size;     /* Section file offset and file size */
    uint32_t link;          /* Section link */
    uint32_t info;          /* Section information */
    uint32_t addr_align;    /* Address alignment */
    uint32_t entsize;       /* Entry size */
};

typedef struct {
    vfs_node_t* node;
    struct ELFHeader* hdr;
    struct ELFSection* sections;
    char* strtab;
} elf_exec_t;

elf_exec_t* elf_open_file(vfs_node_t* node);

/*  Parse the sections from the file
    Returns the number of sections, or 0 if no section */
int elf_parse_sections(elf_exec_t* elf);

#endif /* end of include guard: _ELF_H */
