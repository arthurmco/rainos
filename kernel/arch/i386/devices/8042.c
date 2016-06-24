#include "8042.h"

static int iHead = 0, iTail = 0;
static uint8_t i8042Buffer[64];

static void i8042_interrupt_1(regs_t* r);

/* Create interrupt handlers and start the controller */
int i8042_init()
{
    irq_add_handler(1, &i8042_interrupt_1);

    /*  Disable interrupts for the ports of the controller, and
        disable scancode translation */

    outb(I8042_CMD, 0x20);
    i8042_wait_send();
    uint8_t confbyte = inb(I8042_DATA);

    confbyte = (confbyte & ~0x43);
    outb(I8042_CMD, 0x60);
    i8042_wait_send();
    outb(I8042_DATA, confbyte);

    /* Disable the ports */
    outb(I8042_CMD, 0xAD);  // first
    outb(I8042_CMD, 0xA7);  // second

    /* Tries to flush the out buffer */
    for (int i = 0; i < 10; i++) {
        inb(I8042_DATA);
    }

    outb(I8042_CMD, 0xAA);
    i8042_wait_send();
    i8042_wait_recv();

    if (inb(I8042_DATA) != 0x55) {
        kerror("i8042: error on initialzation");
	    return 0;
    }

    /* Test the ports */
    outb(I8042_CMD, 0xAB);
    i8042_wait_send();
    i8042_wait_recv();

    uint8_t ret = inb(I8042_DATA);
    if (ret != 0x00) {
	kerror("i8042: first port returned status %x on test", ret);
   	return 0;
    }

    /* Reenable the ports */
    outb(I8042_CMD, 0xAE);
    i8042_wait_send();

    /* Restore interrupts */
    outb(I8042_CMD, 0x20);
    i8042_wait_send();
    confbyte = inb(I8042_DATA);

    confbyte = (confbyte | 0x3);
    outb(I8042_CMD, 0x60);
    i8042_wait_send();
    outb(I8042_DATA, confbyte);

	i8042_recv(1);

    /* Reset devices */
	i8042_send(0xff, 1);
	uint8_t res = i8042_recv(1);
	if (res == 0xaa || res == 0xfa) {
		knotice("i8042: port 1 ok!");
	} else if (res == 0xfc) {
		kwarn("i8042: port 1 failed");
	} else {
		kwarn("i8042: port 1 responded with status %02x", res);
	}

    knotice("i8042: initialization successful!");

    return 1;

}

void i8042_wait_send()
{
    while (inb(I8042_STATUS) & 0x2) {}
}

void i8042_wait_recv()
{
    while (!(inb(I8042_STATUS) & 0x1)) {}
}

/* Send a byte through the controller */
void i8042_send(uint8_t byte, uint8_t port)
{
    if (port == 1)
        outb(I8042_DATA, byte);
    else
        kerror("i8042: writes to port %d not supported by this driver", port);

    i8042_wait_send();
}

/* Receive a byte through the controller */
uint8_t i8042_recv(uint8_t port)
{

	/* If both are equal, then wait until a message comes */
	if (iTail == iHead)
		i8042_wait_recv();

    uint8_t ret = 0;
    if (iHead == iTail)
        ret = inb(I8042_DATA);
    else
        ret = i8042Buffer[iTail++];

    if (iTail > 64)
        iTail = 0;

    if (port == 1)
        return ret;
    else
        kerror("i8042: reads from port %d not supported by this driver", port);

    return 0;
}

/*  Interrupt handler from the first port of the
    8042, usually the keyboard. Always IRQ 1 */
static void i8042_interrupt_1(regs_t* r)
{
	uint8_t res = inb(I8042_DATA);
	knotice("i8042: %x", res);
    i8042Buffer[iHead++] = res;

    if (iHead > 64) {
        iHead = 0;
    }
}
