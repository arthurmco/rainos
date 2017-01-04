#include <arch/i386/devices/8042.h>

static int iHead = 0, iTail = 0;
static uint8_t i8042Buffer[64];

static void i8042_interrupt_1(regs_t* r);

/* Create interrupt handlers and start the controller */
int i8042_init()
{
    irq_add_handler(1, &i8042_interrupt_1);

    /*  Disable interrupts for the ports of the controller, and
        disable scancode translation */

    i8042_wait_send();
    outb(I8042_CMD, 0x20);

    i8042_wait_recv();
    uint8_t confbyte = inb(I8042_DATA);

    confbyte = (confbyte & ~0x43);

    i8042_wait_send();
    outb(I8042_CMD, 0x60);
    i8042_wait_send();
    outb(I8042_DATA, confbyte);

    io_wait();


    /* Disable the ports */
    i8042_wait_send();
    outb(I8042_CMD, 0xAD);  // first
    i8042_wait_send();
    outb(I8042_CMD, 0xA7);  // second

    io_wait();

    knotice("i8042: configuring the device...");

    /* Tries to flush the out buffer */
    size_t t = 0;
    while (!i8042_check_recv()) {
        inb(I8042_DATA);
        t++;

        /*  This prevents infinite loops on machines where
            you don't have a i8042 ready */
        if (t > 16)
            break;
    }

    i8042_wait_send();
    outb(I8042_CMD, 0xAA);
    if (!i8042_wait_recv()) { kerror("i8042: timeout"); return 2; }

    if (inb(I8042_DATA) != 0x55) {
        kerror("i8042: error on initialzation");
	    return 0;
    }

    /* Test the ports */
    i8042_wait_send();
    outb(I8042_CMD, 0xAB);
    if (!i8042_wait_recv()) { kerror("i8042: timeout"); return 2; }

    uint8_t ret = inb(I8042_DATA);
    if (ret != 0x00) {
	kerror("i8042: first port returned status %x on test", ret);
   	return 2;
    }

    io_wait();
    /* Reenable the ports */
    i8042_wait_send();
    outb(I8042_CMD, 0xAE);

    io_wait();
    /* Restore interrupts */
    i8042_wait_send();
    outb(I8042_CMD, 0x20);
    i8042_wait_recv();
    confbyte = inb(I8042_DATA);

    confbyte = (confbyte | 0x3);
    i8042_wait_send();
    outb(I8042_CMD, 0x60);
    i8042_wait_send();
    outb(I8042_DATA, confbyte);

    if (i8042_check_recv())
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

int i8042_wait_send()
{
    int timeout = 0;
    while (inb(I8042_STATUS) & 0x2) {
        if (timeout > 10)
            return 0;

        asm volatile("nop");
        timeout++;
    }

    return 1;
}

int i8042_wait_recv()
{

    if (iHead != iTail) return 1;

    int timeout = 0;
    while (!(inb(I8042_STATUS) & 0x1)) {
        if (timeout > 100)
            return 0;

        asm volatile("nop");
        timeout++;
    }
    return 1;

}

int i8042_check_recv()
{
    if (iHead != iTail) return 1;

    return (inb(I8042_STATUS) & 0x1);
}

/* Send a byte through the controller */
void i8042_send(uint8_t byte, uint8_t port)
{
    i8042_wait_send();

    if (port == 1)
        outb(I8042_DATA, byte);
    else
        kerror("i8042: writes to port %d not supported by this driver", port);

}

/* Receive a byte through the controller */
uint8_t i8042_recv(uint8_t port)
{
	/* If both are equal, then wait until a message comes */
	if (iTail == iHead) {
        uint8_t timeout = 0;
		while (!i8042_wait_recv()){
            asm volatile("hlt");
            timeout++;

            if (timeout > 10000) {
                kwarn("i8042: recv timeout");
            }
        }
    }
    uint8_t ret = 0;
    if (iHead == iTail)
        ret = inb(I8042_DATA);
    else
        ret = i8042Buffer[iTail++];

    if (iTail >= 64)
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
    if ((iHead == 63 && iTail == 0) || (iHead == iTail-1)) {
        kerror("i8042: receive buffer full!");
        return;
    }

	uint8_t res = inb(I8042_DATA);
    i8042Buffer[iHead++] = res;

    if (iHead >= 64) {
        iHead = 0;
    }

}


/* Flush the receive buffer */
void i8042_flush_recv(uint8_t port)
{
    /* Receive until there's nothing to receive */
    while ((inb(I8042_STATUS) & 0x1)) {
        i8042_recv(port);
    }

    /* Flush OUR buffer too */
    while (iHead != iTail) {
        i8042_recv(port);
    }

    iHead = 0;
    iTail = 0;
}
