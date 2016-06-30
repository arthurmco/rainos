#include "vbe.h"

static struct VBEControllerInfo* vbe_controller;
static struct VBEModeInfo* vbe_mode;
static void* vbe_framebuffer;
static uint32_t vbe_fb_size;

/* Initialize the framebuffer */
int vbe_init(physaddr_t vbe_controller_info, physaddr_t vbe_mode_info)
{
    knotice("VBE: controller info at phys 0x%08x, mode info at phys 0x%08x",
        vbe_controller_info, vbe_mode_info);

    virtaddr_t vbe_c_virt = vmm_map_physical(VMM_AREA_KERNEL, vbe_controller_info & ~0xfff, 1);
    virtaddr_t vbe_m_virt;

    /* If both are in the same page, no need to map */
    if ((vbe_controller_info & ~0xfff) ==( vbe_mode_info & ~0xfff))
        vbe_m_virt = vbe_c_virt;
    else
        vbe_m_virt = vmm_map_physical(VMM_AREA_KERNEL, vbe_mode_info & ~0xfff, 1);

    vbe_controller = (struct VBEControllerInfo*)(vbe_c_virt + ((vbe_controller_info & 0xfff)));
    vbe_mode = (struct VBEModeInfo*)(vbe_m_virt + ((vbe_mode_info & 0xfff)));

    knotice("VBE: vesa signature: '%s'",
        vbe_controller->signature);

    if (strncmp("VESA", vbe_controller->signature, 4)) {
        kerror("VBE: wrong signature!");
        return 0;
    }

    knotice("VBE: version %d.%d",
        vbe_controller->version >> 8,
        vbe_controller->version & 0xff);
    knotice("VBE: detected video mode with %dx%d, %d bpp",
            vbe_mode->Xres, vbe_mode->Yres, vbe_mode->bpp);

    vbe_fb_size = vbe_mode->Yres * vbe_mode->pitch;
    knotice("VBE: framebuffer at 0x%08x physical, %d bytes",
        vbe_mode->physbase, vbe_fb_size);
    knotice("VBE: blue off:%d, mask:%x; green off:%d, mask %x, red off:%d, mask %x ",
        vbe_mode->blue_position, vbe_mode->blue_mask,
        vbe_mode->green_position, vbe_mode->green_mask,
        vbe_mode->red_position, vbe_mode->red_mask);

    vbe_framebuffer = (void*) vmm_map_physical(VMM_AREA_KERNEL,
        vbe_mode->physbase, (vbe_fb_size / VMM_PAGE_SIZE)+1);

    return 1;
}

/* Get the virtual address of the framebuffer */
void* vbe_get_framebuffer()
{
    return vbe_framebuffer;
}

/* Get the physical address of the framebuffer */
physaddr_t vbe_get_framebuffer_phys()
{
    return vbe_mode->physbase;
}

int vbe_get_framebuffer_struct(framebuffer_t* fb)
{
    if (!vbe_framebuffer) return 0;

    fb->red_mask = vbe_mode->red_mask;
    fb->red_off = vbe_mode->red_position;
    fb->green_mask = vbe_mode->green_mask;
    fb->green_off = vbe_mode->green_position;
    fb->blue_mask = vbe_mode->blue_mask;
    fb->blue_off = vbe_mode->blue_position;

    fb->width = vbe_mode->Xres;
    fb->height = vbe_mode->Yres;
    fb->bpp = vbe_mode->bpp;

    fb->pitch = vbe_mode->pitch;
    fb->fb_addr = vbe_framebuffer;

    return 1;
}
