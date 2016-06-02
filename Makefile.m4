
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
CFLAGS= -std=gnu99 -ffreestanding -nostdlib -nostartfiles -Wall -Wextra
CINCLUDES= -I$(CURDIR)/libk/include
LDFLAGS=-lgcc -g
OUT=rainos.elf
ISO=rainos.iso

LIBK=kstdio.o kstdlib.o kstring.o kstdlog.o

all: start.o main.o vga.o ioport.o idt.o idt_asm.o fault.o terminal.o serial.o \
 8259.o irq.o irq_asm.o ttys.o pmm.o $(LIBK)
	$(CC) -T linker.ld -o $(OUT) $(CFLAGS) $(CINCLUDES) -lgcc $^ $(LDFLAGS)

iso: all
	cp $(OUT) iso/boot
	grub-mkrescue -o $(ISO) iso/

qemu: all
	qemu-system-i386 -kernel $(OUT) -m 8 -monitor stdio

clean: *.o
	rm *.o
	rm $(OUT)

ASM_SOURCE(kernel/arch/i386/,start)
C_SOURCE_WITH_H(kernel/arch/i386/devices/,vga)
C_SOURCE_WITH_H(kernel/arch/i386/devices/,ioport)
C_SOURCE_WITH_H(kernel/arch/i386/devices/,serial)
C_SOURCE_WITH_H(kernel/arch/i386/devices/,8259)
C_SOURCE_WITH_H(kernel/arch/i386/,idt)
C_SOURCE_WITH_H(kernel/arch/i386/,irq)
ASM_SOURCE(kernel/arch/i386/,idt_asm)
ASM_SOURCE(kernel/arch/i386/,irq_asm)
C_SOURCE_WITH_H(kernel/arch/i386/,fault)
C_SOURCE_WITH_H(kernel/,terminal)
C_SOURCE_WITH_H(kernel/,pmm)
C_SOURCE_WITH_H(kernel/,ttys)
C_SOURCE(kernel/,main)

C_SOURCE(libk/,kstdio)
C_SOURCE(libk/,kstdlib)
C_SOURCE(libk/,kstring)
C_SOURCE(libk/,kstdlog)
