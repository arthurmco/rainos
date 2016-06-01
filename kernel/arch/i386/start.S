/*
    RainOS entry point for x86
*/

# Multiboot definitions
# The Multiboot is a protocol used for unify OS boot methods

.set PAGEALIGN, 1 << 0              #; Align our kernel in page (4K) boundaries
.set MEMINFO,   1 << 1              #; Provides memory map
.set FLAGS,     PAGEALIGN | MEMINFO #; Multiboot flags
.set MAGIC,     0x1BADB002          #; Multiboot magic number
.set CHECKSUM,  -(MAGIC + FLAGS)    #; Checksum. The checksum plus magic and flags
                                    #; needs to be zero.

#; Plot the Multiboot header in our executable
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .stack, "aw", @nobits
    .align 4
#; The kernel stack
stack_bottom:
    .skip 1024
stack_top:

.section .text
.global _kernel_entry
.type _kernel_entry, @function
.set MAGIC_MBOOT, 0x2BADB002

_kernel_entry:

/* This region will do simple things, like:
     - Save the GRUB pointers
     - Enable GDT
     - TODO: Enable paging
     - Call the main function
*/

#; Setup the stack
movl $stack_top, %esp
cld

# Push ebx, where the multiboot info pointer lies
push %ebx

#; Compare the magic numbers
cmp $MAGIC_MBOOT, %eax
je enable_gdt
push $mMagicError
call print_error
jmp halt

enable_gdt:
mov $dGDTPtr, %eax
lgdt (%eax)
jmp $0x08, $.enable_gdt_reload

.enable_gdt_reload:
movw $0x10, %ax
movw %ax, %ds
movw %ax, %es
movw %ax, %fs
movw %ax, %gs
movw %ax, %ss
jmp call_main

call_main:

.extern kernel_main
call kernel_main
jmp halt

# Get char pointer on stack
# (C prototype: void print_error(char* msg) )
print_error:
# Get VGA cursor position
mov $0xb8000, %ebx
movw $0x80, %cx
imulw (cur_y), %cx
addw %cx, (cur_x)
add %ebx, %ecx

mov 4(%esp), %esi
mov %ebx, %edi

.print_char:

movb $0x0c, %al	# Set the attribute color (light red text/black bg)

movsb 	# Move the text
stosb	# and attribute

movb (%esi), %al
cmpb $0, %al
jne .print_char

ret


halt:
hlt
jmp halt
.long 0x0

.section .data

cur_x: .word 0
cur_y: .word 5

# Messages
mMagicError: .asciz "Error: Wrong magic number"
mContinue: .asciz "Continue"

# GDT
.align 4
dGDT:
# index 0, always 0
.word 0x0
.word 0x0
.word 0x0
.word 0x0

# index 1: kernel code segment
# base 0x0, limit 0xffff, access 0x9a, flags 0xc
.word 0xffff
.word 0x0
.byte 0x0
.byte 0x9a
.byte 0xcf
.byte 0x0

# index 2: kernel data segment
# base 0x0, limit 0xffff, access 0x92, flags 0xc
.word 0xffff
.word 0x0
.byte 0x0
.byte 0x92
.byte 0xcf
.byte 0x0
dGDTEnd:
.long 0x0

# GDT pointer
.align 4
dGDTPtr:
        .word dGDTEnd-dGDT
        .long dGDT
