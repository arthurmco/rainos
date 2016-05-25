
   
CC=i686-elf-gcc
AS=i686-elf-as
CFLAGS= -std=gnu99 -ffreestanding -nostdlib -nostartfiles -Wall -Wextra
CINCLUDES= -I$(CURDIR)/libk/include
LDFLAGS=-lgcc -g
OUT=rainos.elf
ISO=rainos.iso

LIBK=kstdio.o kstdlib.o kstring.o

all: start.o main.o vga.o ioport.o idt.o idt_asm.o fault.o terminal.o $(LIBK)
	$(CC) -T linker.ld -o $(OUT) $(CFLAGS) $(CINCLUDES) -lgcc $^ $(LDFLAGS)

iso: $(OUT)
	cp $(OUT) iso/boot
	grub-mkrescue -o $(ISO) iso/

clean: *.o
	rm *.o
	rm $(OUT)

start.o: kernel/arch/i386/start.S
	$(AS) kernel/arch/i386/start.S -o start.o $(ASMFLAGS)
vga.o: kernel/arch/i386/devices/vga.c kernel/arch/i386/devices/vga.h
	$(CC) -o vga.o -c kernel/arch/i386/devices/vga.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
ioport.o: kernel/arch/i386/devices/ioport.c kernel/arch/i386/devices/ioport.h
	$(CC) -o ioport.o -c kernel/arch/i386/devices/ioport.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
idt.o: kernel/arch/i386/idt.c kernel/arch/i386/idt.h
	$(CC) -o idt.o -c kernel/arch/i386/idt.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
idt_asm.o: kernel/arch/i386/idt_asm.S
	$(AS) kernel/arch/i386/idt_asm.S -o idt_asm.o $(ASMFLAGS)
fault.o: kernel/arch/i386/fault.c kernel/arch/i386/fault.h
	$(CC) -o fault.o -c kernel/arch/i386/fault.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
terminal.o: kernel/terminal.c kernel/terminal.h
	$(CC) -o terminal.o -c kernel/terminal.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
main.o: kernel/main.c
	$(CC) -o main.o -c kernel/main.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)

kstdio.o: libk/kstdio.c
	$(CC) -o kstdio.o -c libk/kstdio.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
kstdlib.o: libk/kstdlib.c
	$(CC) -o kstdlib.o -c libk/kstdlib.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
kstring.o: libk/kstring.c
	$(CC) -o kstring.o -c libk/kstring.c $(CFLAGS) $(CINCLUDES) $(LDFLAGS)
