#include "elf.h"

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

    for (size_t i = 0; i < elf->hdr->shnum; i++) {
        knotice("\t#%d: name %s (off %d) type %x flags %x laddr %08x "
                "fileoff %x filesize %d", i,
            (elf->strtab) ? &elf->strtab[elf->sections[i].name_off] : "<NULL>",
            elf->sections[i].name_off, elf->sections[i].type,
            elf->sections[i].flags, elf->sections[i].addr,
            elf->sections[i].off, elf->sections[i].size);
    }


    return elf->hdr->shnum;

}
