
dnl Macro for creating a source compile rule for make
define(`C_SOURCE',
`$2.o: $1$2.c
	`$'(CC) -o $2.o -c $1$2.c `$'(CFLAGS)') dnl
dnl Macro for creating a source/header pair
define(`C_SOURCE_WITH_H',
`$2.o: $1$2.c $1$2.h
	`$'(CC) -o $2.o -c $1$2.c `$'(CFLAGS)') dnl
dnl Macro for creating ASM file source
define(`ASM_SOURCE',
`$2.o: $1$2.S
	`$'(AS) $1$2.S -o $2.o `$'(ASMFLAGS)') dnl

CC=i686-elf-gcc
AS=i686-elf-as
CFLAGS= -lgcc -std=gnu99 -ffreestanding -nostdlib -nostartfiles -Wall -Wextra
LDFLAGS= -g
OUT=rainos.elf
ISO=rainos.iso

all: start.o main.o vga.o
	$(CC) -T linker.ld -o $(OUT) $(CFLAGS) -lgcc $^ $(LDFLAGS)

iso: $(OUT)
	cp $(OUT) iso/boot
	grub-mkrescue -o $(ISO) iso/ 

ASM_SOURCE(kernel/arch/i386/,start)
C_SOURCE_WITH_H(kernel/arch/i386/devices/,vga)
C_SOURCE(kernel/,main)
