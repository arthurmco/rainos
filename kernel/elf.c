#include <elf.h>

static int elf_check_ident(struct ELFHeader* ehdr);


elf_exec_t* elf_open_file(vfs_node_t* node)
{
    elf_exec_t* elf = (elf_exec_t*)kcalloc(sizeof(elf_exec_t), 1);

    elf->node = node;
    if (!elf->node) {
        kerror("elf_parser: invalid node");
        return NULL;
    }

    elf->hdr = (struct ELFHeader*)kcalloc(sizeof(struct ELFHeader), 1);
    vfs_read(elf->node, 0, sizeof(struct ELFHeader), elf->hdr);

    if (!elf_check_ident(elf->hdr))
        return NULL;

    if (elf->hdr->machine != 3) {
        kerror("elf_parser: invalid arch (expected 3 (EM_386), found %x)",
            elf->hdr->machine);
        return NULL;
    }

    knotice("elf_parser: loaded ELF file, entry point at offset %x",
        elf->hdr->entry);

    return elf;
}

static int elf_check_ident(struct ELFHeader* ehdr)
{
    if (ehdr->e_ident[E_MAG0] != 0x7f ||
        ehdr->e_ident[E_MAG1] != 'E' ||
        ehdr->e_ident[E_MAG2] != 'L' ||
        ehdr->e_ident[E_MAG3] != 'F')
        {
            kerror("elf_parser: invalid ELF magic");
            return 0;
        }

    if (ehdr->e_ident[E_CLASS] != 1) {
        kerror("elf_parser: invalid ELF class (%d, expected 1)",
            ehdr->e_ident[E_CLASS]);
        return 0;
    }

    if (ehdr->e_ident[E_DATA] != 1) {
        kerror("elf_parser: invalid endianness for the data");
        return 0;
    }

    return 1;
}

/*  Parse the program headers from the file
    Returns the number of phs or 0 if no phs    */
int elf_parse_phs(elf_exec_t* elf)
{
    knotice("elf_parser: program header table is at file off %x",
        elf->hdr->phoff);

    if (elf->hdr->phnum == 0) {
        kerror("elf_parser: no program headers found");
        return 0;
    }

    if (elf->hdr->phentsize < sizeof(struct ELFProgHeader)) {
        kerror("elf_parser: program header size is smaller than supported");
        return 0;
    }

    /* Load program hdrs into memory */
    elf->programs = (struct ELFProgHeader*)kcalloc(elf->hdr->phentsize,
        elf->hdr->phnum);
    vfs_read(elf->node, elf->hdr->phoff, (elf->hdr->phentsize * elf->hdr->phnum),
        elf->programs);
    elf->phdr_count = elf->hdr->phnum;

    knotice("elf_parse_phs: %d program segments detected", elf->phdr_count);

    for (int i = 0; i < elf->phdr_count; i++) {
        knotice("\t#%d: vaddr 0x%x, off %x, file size: %d, memory size %d, flags %x",
        i+1, elf->programs[i].vaddr, elf->programs[i].offset,
        elf->programs[i].file_size,
        elf->programs[i].memory_size, elf->programs[i].flags);
    }

    return elf->phdr_count;
}

/*  Parse the sections from the file
    Returns the number of sections, or 0 if no section */
int elf_parse_sections(elf_exec_t* elf)
{
    knotice("elf_parser: section header table is at file off %x",
        elf->hdr->shoff);
    if (elf->hdr->shentsize < sizeof(struct ELFSection)) {
        kerror("elf_parser: section size is smaller than supported");
        return 0;
    }

    if (elf->hdr->shnum == 0) {
        kerror("elf_parser: no sections found");
        return 0;
    }

    /* Load sections into memory */
    elf->sections = (struct ELFSection*)kcalloc(elf->hdr->shentsize,
            elf->hdr->shnum);
    vfs_read(elf->node, elf->hdr->shoff, (elf->hdr->shentsize * elf->hdr->shnum),
        elf->sections);
    elf->section_count = elf->hdr->shnum;

    /* Parse the string table */
    int stridx = elf->hdr->shstrndx;
    if (stridx > elf->hdr->shnum) {
        kwarn("elf_parser: no string table found");
    } else {
        knotice("elf_parser: string table is at section header #%d", stridx);
        elf->strtab = (char*)kcalloc(elf->sections[stridx].size,1);
        vfs_read(elf->node, elf->sections[stridx].off, elf->sections[stridx].size,
            elf->strtab);
    }

    knotice("elf_parser: sections for %s", elf->node->name);

    /* Ignore first entry, it contains nothing */
    for (size_t i = 1; i < elf->hdr->shnum; i++) {
        char* types;
        char flags[64];
        flags[0] = 0;

        switch (elf->sections[i].type & 0x7fffffff) {
            case  SHT_PROGBITS: types = "PROGBITS";  break;
            case  SHT_SYMTAB  : types = "SYMTAB";    break;
            case  SHT_STRTAB  : types = "STRTAB";    break;
            case  SHT_RELA    : types = "RELA";      break;
            case  SHT_HASH    : types = "HASH";      break;
            case  SHT_DYNAMIC : types = "DYNAMIC";   break;
            case  SHT_NOTE    : types = "NOTE";      break;
            case  SHT_NOBITS  : types = "NOBITS";    break;
            case  SHT_SHLIB   : types = "SHLIB";     break;
            case  SHT_DYNSYM  : types = "DYNSYM";    break;
            case  SHT_REL     : types = "REL";       break;
            default: types = "(unknown)"; break;
        }

        if (elf->sections[i].flags & SHF_WRITE) strcat(flags, "write ");
        if (elf->sections[i].flags & SHF_ALLOC) strcat(flags, "alloc ");
        if (elf->sections[i].flags & SHF_EXECINSTR) strcat(flags, "exec ");

        knotice("\t#%d: name %s (off %d) type %x (%s) flags %x (%s) "
                "laddr %08x fileoff %x filesize %d", i,
         (elf->strtab) ? &elf->strtab[elf->sections[i].name_off] : "<NULL>",
         elf->sections[i].name_off, elf->sections[i].type, types,
         elf->sections[i].flags, flags, elf->sections[i].addr,
         elf->sections[i].off, elf->sections[i].size);
         //elf_load_section(&elf->sections[i], elf->sections[i].addr);
    }


    return elf->hdr->shnum;

}


/* Load an section to some address */
uintptr_t elf_load_section(struct ELFSection* sec, uintptr_t addr)
{
    return vmm_map_page(addr, VMM_AREA_USER, (sec->size/4096)+1);
}

#define max(a, b) ((a > b) ? a : b)

/* Execute an ELF file */
int elf_execute_file(elf_exec_t* elf)
{
    for (int i = 0; i < elf->phdr_count; i++) {
        knotice("Loaded program segment %d at addr %x",
            i+1, elf->programs[i].vaddr);

        size_t pagect = (elf->programs[i].memory_size/VMM_PAGE_SIZE)+1;

        vmm_dealloc_page(elf->programs[i].vaddr, pagect);
        void* v = (void*)vmm_map_page(elf->programs[i].vaddr, VMM_AREA_USER,
            pagect);
        knotice("Allocating at addr %x", v);

        if(vfs_read(elf->node, elf->programs[i].offset,
            elf->programs[i].file_size, v) < 0) {
                panic("Failure while reading elf (ph segment %d)", i);
        };

    }

    return 1;

}
