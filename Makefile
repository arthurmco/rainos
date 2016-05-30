
   
CC=i686-elf-gcc
AS=i686-elf-as
CFLAGS= -std=gnu99 -ffreestanding -nostdlib -nostartfiles -Wall -Wextra
CINCLUDES= -I$(CURDIR)/libk/include
LDFLAGS=-lgcc -g
OUT=rainos.elf
ISO=rainos.iso

LIBK=kstdio.o kstdlib.o kstring.o

all: start.o main.o vga.o ioport.o idt.o idt_asm.o fault.o terminal.o serial.o \
 8259.o irq.o irq_asm.o ttys.o $(LIBK)
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
serial.o: kernel/arch/i386/devices/serial.c kernel/arch/i386/devices/serial.h
	$(CC) -o serial.o -c kernel/arch/i386/devices/serial.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
8259.o: kernel/arch/i386/devices/8259.c kernel/arch/i386/devices/8259.h
	$(CC) -o 8259.o -c kernel/arch/i386/devices/8259.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
idt.o: kernel/arch/i386/idt.c kernel/arch/i386/idt.h
	$(CC) -o idt.o -c kernel/arch/i386/idt.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
irq.o: kernel/arch/i386/irq.c kernel/arch/i386/irq.h
	$(CC) -o irq.o -c kernel/arch/i386/irq.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
idt_asm.o: kernel/arch/i386/idt_asm.S
	$(AS) kernel/arch/i386/idt_asm.S -o idt_asm.o $(ASMFLAGS)
irq_asm.o: kernel/arch/i386/irq_asm.S
	$(AS) kernel/arch/i386/irq_asm.S -o irq_asm.o $(ASMFLAGS)
fault.o: kernel/arch/i386/fault.c kernel/arch/i386/fault.h
	$(CC) -o fault.o -c kernel/arch/i386/fault.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
terminal.o: kernel/terminal.c kernel/terminal.h
	$(CC) -o terminal.o -c kernel/terminal.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
ttys.o: kernel/ttys.c
	$(CC) -o ttys.o -c kernel/ttys.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
main.o: kernel/main.c
	$(CC) -o main.o -c kernel/main.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)

kstdio.o: libk/kstdio.c
	$(CC) -o kstdio.o -c libk/kstdio.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
kstdlib.o: libk/kstdlib.c
	$(CC) -o kstdlib.o -c libk/kstdlib.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
kstring.o: libk/kstring.c
	$(CC) -o kstring.o -c libk/kstring.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
