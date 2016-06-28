
dnl Macro for creating a source compile rule for make
define(`C_SOURCE',
`$2.o: $1$2.c
	`$'(CC) -o $2.o -c $1$2.c `$'(CFLAGS) `$'(CINCLUDES) `$'(LDFLAGS)') dnl
dnl Macro for creating a source/header pair
define(`C_SOURCE_WITH_H',
`$2.o: $1$2.c $1$2.h
	`$'(CC) -o $2.o -c $1$2.c `$'(CFLAGS) `$'(CINCLUDES) `$'(LDFLAGS)') dnl
dnl Macro for creating ASM file source
define(`ASM_SOURCE',
`$2.o: $1$2.S
	`$'(AS) $1$2.S -o $2.o `$'(ASMFLAGS)') dnl

CC=i686-elf-gcc
AS=i686-elf-as
CFLAGS= -std=gnu99 -ffreestanding -fstack-protector -nostdlib -nostartfiles -Wall -Wextra -O1
CINCLUDES= -I$(CURDIR)/libk/include
LDFLAGS=-lgcc -g
OUT=rainos.elf
ISO=rainos.iso

LIBK=kstdio.o kstdlib.o kstring.o kstdlog.o
ARCH_DEP=start.o idt.o idt_asm.o fault.o vga.o ioport.o serial.o 8259.o 8042.o \
 ps2_kbd.o pit.o pci.o ata.o irq.o irq_asm.o pages.o vmm.o tss.o usermode.o \
 specifics.o

all: $(ARCH_DEP) stackguard.o main.o terminal.o ttys.o pmm.o kheap.o dev.o \
 disk.o vfs.o partition.o fat.o initrd.o keyboard.o $(LIBK)
	$(CC) -T linker.ld -o $(OUT) $(CFLAGS) $(CINCLUDES) -lgcc $^ $(LDFLAGS)

initrd: initrd.rain
	cp initrd.rain iso/boot

initrd.rain: initrd/*
	tar -cf initrd.rain initrd/

iso: all initrd
	cp $(OUT) iso/boot
	grub-mkrescue -o $(ISO) iso/

qemu: all
	qemu-system-i386 -kernel $(OUT) -m 8 -monitor stdio

clean: *.o
	rm -f *.o
	rm -f *.iso
	rm -f initrd.rain
	rm -f iso/boot/initrd.rain
	rm -f iso/boot/$(OUT)
	rm -f $(OUT)

ASM_SOURCE(kernel/arch/i386/,start)
C_SOURCE_WITH_H(kernel/arch/i386/devices/,vga)
C_SOURCE_WITH_H(kernel/arch/i386/devices/,ioport)
C_SOURCE_WITH_H(kernel/arch/i386/devices/,pit)
C_SOURCE_WITH_H(kernel/arch/i386/devices/,serial)
C_SOURCE_WITH_H(kernel/arch/i386/devices/,pci)
C_SOURCE_WITH_H(kernel/arch/i386/devices/,8259)
C_SOURCE_WITH_H(kernel/arch/i386/devices/,8042)
C_SOURCE_WITH_H(kernel/arch/i386/devices/,ps2_kbd)
C_SOURCE_WITH_H(kernel/arch/i386/devices/,ata)
C_SOURCE_WITH_H(kernel/arch/i386/,specifics)
C_SOURCE_WITH_H(kernel/arch/i386/,tss)
C_SOURCE_WITH_H(kernel/arch/i386/,idt)
C_SOURCE_WITH_H(kernel/arch/i386/,irq)
C_SOURCE_WITH_H(kernel/arch/i386/,vmm)
C_SOURCE_WITH_H(kernel/arch/i386/,pages)
C_SOURCE_WITH_H(kernel/,pmm)
ASM_SOURCE(kernel/arch/i386/,idt_asm)
ASM_SOURCE(kernel/arch/i386/,usermode)
ASM_SOURCE(kernel/arch/i386/,irq_asm)
C_SOURCE_WITH_H(kernel/arch/i386/,fault)
C_SOURCE_WITH_H(kernel/vfs/,vfs)
C_SOURCE_WITH_H(kernel/vfs/,partition)
C_SOURCE_WITH_H(kernel/vfs/,fat)
C_SOURCE_WITH_H(kernel/,terminal)
C_SOURCE_WITH_H(kernel/,ttys)
C_SOURCE_WITH_H(kernel/,kheap)
C_SOURCE_WITH_H(kernel/,dev)
C_SOURCE_WITH_H(kernel/,disk)
C_SOURCE_WITH_H(kernel/,initrd)
C_SOURCE_WITH_H(kernel/,keyboard)
C_SOURCE(kernel/,main)
C_SOURCE(kernel/,stackguard)

C_SOURCE(libk/,kstdio)
C_SOURCE(libk/,kstdlib)
C_SOURCE(libk/,kstring)
C_SOURCE(libk/,kstdlog)
