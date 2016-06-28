# RainOS

RainOS is a simple OS with a monolithic kernel, designed for old computers.

## Setup
To run it, you need a cross compiler for the i686 target. [Here](http://wiki.osdev.org/GCC_Cross-Compiler) you'll find some directions

To compile it, just type

```
$ sh configure.sh
$ make
```

The makefile assumes your cross-compiler folder is in the PATH.

You can also type ```make iso``` to create an ISO with the system or ```make qemu``` to launch it on QEMU.

## Minimal Requirements
 - A P6-level processor
 - 2 MiB of RAM

## ToDo list

If you want, you can help me with these tasks

- [X] Jump to usermode and run some basic code
- [X] Improve VFS and create filesystem drivers, like
  - [X] a FAT driver
  - [ ] a ext driver
- [ ] Add virtual 8086 support
- [ ] Add VBE support (for creating framebuffers in BIOS systems)  
- [ ] Add DMA support in ATA driver
- [ ] Add an ELF parser
  - [ ] Launch ELF code in usermode
- [ ] Create some syscalls
- [ ] Support x86-64
- [ ] Support UEFI
  - [ ] Add GOP framebuffer support (framebuffers in UEFI)
- [ ] Support, at least, one ARM board, like
  - [ ] Integrator/CP
  - [ ] Beaglebone
  - [ ] Raspberry PI
- [ ] Add SATA disk support
- [ ] Add networking
- [ ] Add USB support
