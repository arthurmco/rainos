
   
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

start.o: kernel/arch/i386/start.S
	$(AS) kernel/arch/i386/start.S -o start.o $(ASMFLAGS)
vga.o: kernel/arch/i386/devices/vga.c kernel/arch/i386/devices/vga.h
	$(CC) -o vga.o -c kernel/arch/i386/devices/vga.c $(CFLAGS)
main.o: kernel/main.c
	$(CC) -o main.o -c kernel/main.c $(CFLAGS)
