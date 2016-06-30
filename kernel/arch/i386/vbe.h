/*  VESA BIOS Extensions framebuffer driver

    Copyright (C) 2016 Arthur M
*/

#include "vmm.h"
#include "../../framebuffer.h"
#include <kstdlog.h>

#ifndef _VBE_H
#define _VBE_H

typedef struct _far {
    uint16_t seg, off;
} farptr_t;

struct VBEControllerInfo {
    char signature[4];   // VESA
    uint16_t version;
    farptr_t oem_name_ptr;
    uint8_t capabilities[4];
    farptr_t video_modes;
    uint16_t total_memory; // 64k blocks of memory
} __attribute__((packed));

struct VBEModeInfo {
    uint16_t attributes;
    uint8_t winA,winB;
    uint16_t granularity;
    uint16_t winsize;
    uint16_t segmentA, segmentB;
    uint32_t func_ptr;
    uint16_t pitch; // bytes per scanline

    uint16_t Xres, Yres;
    uint8_t Wchar, Ychar, planes, bpp, banks;
    uint8_t memory_model, bank_size, image_pages;
    uint8_t reserved0;

    uint8_t red_mask, red_position;
    uint8_t green_mask, green_position;
    uint8_t blue_mask, blue_position;
    uint8_t rsv_mask, rsv_position;
    uint8_t directcolor_attributes;

    uint32_t physbase;  // your LFB (Linear Framebuffer) address ;)
    uint32_t reserved1;
    uint16_t reserved2;
} __attribute__((packed));

/* Initialize the framebuffer */
int vbe_init(physaddr_t vbe_controller_info, physaddr_t vbe_mode_info);

/* Get the virtual address of the framebuffer */
void* vbe_get_framebuffer();

/* Get the physical address of the framebuffer */
physaddr_t vbe_get_framebuffer_phys();

int vbe_get_framebuffer_struct(framebuffer_t* fb);

#endif /* end of include guard: _VBE_H */
