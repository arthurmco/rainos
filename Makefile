
   
CC=i686-elf-gcc
AS=i686-elf-as
CFLAGS= -std=gnu99 -ffreestanding -nostdlib -nostartfiles -Wall -Wextra -O
CINCLUDES= -I$(CURDIR)/libk/include
LDFLAGS=-lgcc -g
OUT=rainos.elf
ISO=rainos.iso

LIBK=kstdio.o kstdlib.o kstring.o kstdlog.o
ARCH_DEP=start.o idt.o idt_asm.o fault.o vga.o ioport.o serial.o 8259.o pit.o \
 pci.o ata.o irq.o irq_asm.o pages.o vmm.o

all: $(ARCH_DEP) main.o  terminal.o ttys.o pmm.o kheap.o dev.o $(LIBK)
	$(CC) -T linker.ld -o $(OUT) $(CFLAGS) $(CINCLUDES) -lgcc $^ $(LDFLAGS)

iso: all
	cp $(OUT) iso/boot
	grub-mkrescue -o $(ISO) iso/

qemu: all
	qemu-system-i386 -kernel $(OUT) -m 8 -monitor stdio

clean: *.o
	rm *.o
	rm $(OUT)

start.o: kernel/arch/i386/start.S
	$(AS) kernel/arch/i386/start.S -o start.o $(ASMFLAGS)
vga.o: kernel/arch/i386/devices/vga.c kernel/arch/i386/devices/vga.h
	$(CC) -o vga.o -c kernel/arch/i386/devices/vga.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
ioport.o: kernel/arch/i386/devices/ioport.c kernel/arch/i386/devices/ioport.h
	$(CC) -o ioport.o -c kernel/arch/i386/devices/ioport.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
pit.o: kernel/arch/i386/devices/pit.c kernel/arch/i386/devices/pit.h
	$(CC) -o pit.o -c kernel/arch/i386/devices/pit.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
serial.o: kernel/arch/i386/devices/serial.c kernel/arch/i386/devices/serial.h
	$(CC) -o serial.o -c kernel/arch/i386/devices/serial.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
pci.o: kernel/arch/i386/devices/pci.c kernel/arch/i386/devices/pci.h
	$(CC) -o pci.o -c kernel/arch/i386/devices/pci.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
8259.o: kernel/arch/i386/devices/8259.c kernel/arch/i386/devices/8259.h
	$(CC) -o 8259.o -c kernel/arch/i386/devices/8259.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
ata.o: kernel/arch/i386/devices/ata.c kernel/arch/i386/devices/ata.h
	$(CC) -o ata.o -c kernel/arch/i386/devices/ata.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
idt.o: kernel/arch/i386/idt.c kernel/arch/i386/idt.h
	$(CC) -o idt.o -c kernel/arch/i386/idt.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
irq.o: kernel/arch/i386/irq.c kernel/arch/i386/irq.h
	$(CC) -o irq.o -c kernel/arch/i386/irq.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
vmm.o: kernel/arch/i386/vmm.c kernel/arch/i386/vmm.h
	$(CC) -o vmm.o -c kernel/arch/i386/vmm.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
pages.o: kernel/arch/i386/pages.c kernel/arch/i386/pages.h
	$(CC) -o pages.o -c kernel/arch/i386/pages.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
pmm.o: kernel/pmm.c kernel/pmm.h
	$(CC) -o pmm.o -c kernel/pmm.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
idt_asm.o: kernel/arch/i386/idt_asm.S
	$(AS) kernel/arch/i386/idt_asm.S -o idt_asm.o $(ASMFLAGS)
irq_asm.o: kernel/arch/i386/irq_asm.S
	$(AS) kernel/arch/i386/irq_asm.S -o irq_asm.o $(ASMFLAGS)
fault.o: kernel/arch/i386/fault.c kernel/arch/i386/fault.h
	$(CC) -o fault.o -c kernel/arch/i386/fault.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
terminal.o: kernel/terminal.c kernel/terminal.h
	$(CC) -o terminal.o -c kernel/terminal.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
ttys.o: kernel/ttys.c kernel/ttys.h
	$(CC) -o ttys.o -c kernel/ttys.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
kheap.o: kernel/kheap.c kernel/kheap.h
	$(CC) -o kheap.o -c kernel/kheap.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
dev.o: kernel/dev.c kernel/dev.h
	$(CC) -o dev.o -c kernel/dev.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
main.o: kernel/main.c
	$(CC) -o main.o -c kernel/main.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)

kstdio.o: libk/kstdio.c
	$(CC) -o kstdio.o -c libk/kstdio.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
kstdlib.o: libk/kstdlib.c
	$(CC) -o kstdlib.o -c libk/kstdlib.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
kstring.o: libk/kstring.c
	$(CC) -o kstring.o -c libk/kstring.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
kstdlog.o: libk/kstdlog.c
	$(CC) -o kstdlog.o -c libk/kstdlog.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
