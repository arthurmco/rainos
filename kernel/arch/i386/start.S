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

.section .phys_stack, "aw", @nobits
    .align 4
#; The kernel stack
phys_stack_bottom:
    .skip 64
phys_stack_top:


.section .stack, "aw", @nobits
    .align 4
#; The kernel stack
stack_bottom:
    .skip 1024
stack_top:

.section .pages, "aw", @nobits
  .set KERNEL_VADDR, 0xc0000000
  .align 4096
page_dir_table:
  # Page directories for the kernel
  .skip 4096
page_table_identity:
  # Reserve for page tables used for identity map the first 4MB of kernel
  # in 0x0-0x400000
  .skip 4096
page_table_kernel:
  # Reserve for page tables used for identity map the first 4MB of kernel
  # in KERNEL_VADDR-KERNEL_VADDR+0x400000
  .skip 4096

.section .phys_text, "ax"
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
movl $phys_stack_top, %esp
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
jmp setup_paging

setup_paging:
mov $page_dir_table, %ebx

#Setup the identity directory
mov $page_table_identity, %edx
orl $0x3, %edx
mov %edx, (%ebx)

# Setup the table list
mov $page_table_identity, %ebx
xor %ecx, %ecx
.page_identity_fill:
mov %ecx, %edx
imull $4096, %edx
orl $0x3, %edx    # The page is supervised, present and r/w
mov %edx, (%ebx,%ecx,4)

inc %ecx
cmp $1024, %ecx
jne .page_identity_fill


mov $page_dir_table, %ebx
#Setup the kernel directory
mov $page_table_kernel, %edx
orl $0x3, %edx
mov %edx, 0xc00(%ebx)

# Setup the kernel table list
mov $page_table_kernel, %ebx
xor %ecx, %ecx
.page_kernel_fill:
mov %ecx, %edx
imull $4096, %edx
orl $0x3, %edx    # The page is supervised, present and r/w
mov %edx, (%ebx,%ecx,4)

inc %ecx
cmp $1024, %ecx
jne .page_kernel_fill

# Set CR3 and enable paging
mov $page_dir_table, %eax
mov %eax, %cr3

mov %cr0, %eax
orl $0x80000000, %eax
mov %eax, %cr0

jmp call_main

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

.section .text
call_main:
.extern kernel_main

# Move contents between stack
pop %ebx
mov $stack_top, %esp

push $page_dir_table
push %ebx

call kernel_main
jmp halt

halt:
hlt
jmp halt
.long 0x0

.section .phys_data, "aw", @progbits

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
