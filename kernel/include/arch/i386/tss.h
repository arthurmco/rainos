/*  Task State Segment configuration code

    Copyright (C) 2016 Arthur M

*/

#include <stdint.h>

#ifndef _TSS_H
#define _TSS_H

typedef struct {
    uint16_t link, rsvd0;
    uint32_t esp0;
    uint16_t ss0, rsvd1;
    uint32_t esp1;
    uint16_t ss1, rsvd2;
    uint32_t esp2;
    uint16_t ss2, rsvd3;
    uint32_t cr3, eip, eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp;
    uint32_t esi, edi;
    uint16_t es, rsvd4;
    uint16_t cs, rsvd5;
    uint16_t ss, rsvd6;
    uint16_t ds, rsvd7;
    uint16_t fs, rsvd8;
    uint16_t gs, rsvd9;
    uint16_t ldtr, rsvd10;
    uint16_t rsvd11, iopb_offset;
} __attribute__((packed)) tss_t;

typedef struct {
    unsigned int limit_low:16;
	unsigned int base_low : 24;
     //attribute byte split into bitfields
	unsigned int accessed :1;
	unsigned int read_write :1; //readable for code, writable for data
	unsigned int conforming_expand_down :1; //conforming for code, expand down for data
	unsigned int code :1; //1 for code, 0 for data
	unsigned int always_1 :1; //should be 1 for everything but TSS and LDT
	unsigned int DPL :2; //priviledge level
	unsigned int present :1;
     //and now into granularity
	unsigned int limit_high :4;
	unsigned int available :1;
	unsigned int always_0 :1; //should always be 0
	unsigned int big :1; //32bit opcodes for code, uint32_t stack for data
	unsigned int gran :1; //1 to use 4k page addressing, 0 for byte addressing
	unsigned int base_high :8;
} __attribute__((packed)) gdt_tss_t;

/*  Initialize the TSS struct.
    Make the GDT TSS part point to the TSS */
void tss_init(uintptr_t cr3);


/*  Use the 'ltr' instruction to load the TSS
    into the processor */
void tss_flush();


#endif /* end of include guard: _TSS_H */
