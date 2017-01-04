# RainOS makefile
# Copyright (C) 2016-2017 Arthur M
   
CC=/opt/cross/bin/i686-elf-gcc
AS=/opt/cross/bin/i686-elf-as
CFLAGS= -std=gnu99 -ffreestanding -fstack-protector-all -nostdlib -nostartfiles -Wall
CINCLUDES= -I$(CURDIR)/libk/include
LDFLAGS=-lgcc -g
OUT=rainos.elf
ISO=rainos.iso

LIBK=libk/kstdio.o libk/kstdlib.o libk/kstring.o libk/kstdlog.o

I386_ARCH_DEP=kernel/arch/i386/start.o kernel/arch/i386/idt.o \
 kernel/arch/i386/idt_asm.o kernel/arch/i386/fault.o \
 kernel/arch/i386/devices/vga.o kernel/arch/i386/devices/ioport.o \
 kernel/arch/i386/devices/serial.o kernel/arch/i386/devices/8259.o \
 kernel/arch/i386/devices/8042.o kernel/arch/i386/devices/ps2_kbd.o \
 kernel/arch/i386/devices/pit.o kernel/arch/i386/devices/pci.o \
 kernel/arch/i386/devices/ata.o kernel/arch/i386/irq.o \
 kernel/arch/i386/irq_asm.o kernel/arch/i386/pages.o kernel/arch/i386/vmm.o \
 kernel/arch/i386/tss.o kernel/arch/i386/usermode.o kernel/arch/i386/specifics.o \
 kernel/arch/i386/devices/floppy.o kernel/arch/i386/ebda.o kernel/arch/i386/taskswitch.o \
 kernel/arch/i386/devices/rtc.o

KERNEL_DEP=kernel/stackguard.o kernel/main.o kernel/terminal.o kernel/ttys.o \
 kernel/pmm.o kernel/kheap.o kernel/dev.o kernel/disk.o kernel/vfs/vfs.o \
 kernel/vfs/partition.o kernel/vfs/fat.o kernel/vfs/sfs.o kernel/initrd.o \
 kernel/keyboard.o kernel/kshell.o kernel/elf.o kernel/time.o kernel/task.o

all: $(I386_ARCH_DEP) $(KERNEL_DEP) $(LIBK)
	$(CC) -T linker.ld -o $(OUT) $(CFLAGS) $(CINCLUDES) -lgcc $^ $(LDFLAGS)

initrd: initrd.rain
	cp initrd.rain iso/boot

initrd.rain: initrd/*
	tar -cf initrd.rain initrd/

iso: all initrd
	cp $(OUT) iso/boot
	grub-mkrescue -o $(ISO) iso/

qemu: all initrd
	qemu-system-i386 -kernel $(OUT) -initrd initrd.rain -m 8 -monitor stdio

qemu-serial: all initrd
	qemu-system-i386 -kernel $(OUT) -initrd initrd.rain -m 8 -serial stdio

qemu-gdb: all initrd
	qemu-system-i386 -kernel $(OUT) -initrd initrd.rain -m 8 -serial stdio -S -s

clean: 
	find . -name "*.o" -type f -delete
	rm -f *.iso
	rm -f initrd.rain
	rm -f iso/boot/initrd.rain
	rm -f iso/boot/$(OUT)
	rm -f $(OUT)

%.o: %.S
	$(AS) -o $@ $< $(ASMFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS) $(CINCLUDES) $(LDFLAGS)

