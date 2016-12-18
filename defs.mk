CC=/opt/cross/bin/i686-elf-gcc
AS=/opt/cross/bin/i686-elf-as
CFLAGS= -std=gnu99 -ffreestanding -fstack-protector-all -nostdlib -nostartfiles -Wall
CINCLUDES= -I$(CURDIR)/libk/include -I$(CURDIR)/kernel/include
LDFLAGS=-lgcc -g
OUT=rainos.elf
ISO=rainos.iso
ARCH=i386
