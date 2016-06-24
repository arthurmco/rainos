# This is used to test binary program load from usermode

# Will be replaced by ELF program loading

.section .text
begin:
	movw $0x0a0a, %ax
	mov %eax, %ebx
	inc %eax
	mov $0xdeadbeef, %ebx
	jmp other

another_label:
	mov $0xcafebabe, %ebx
	push %ebx
	push %eax
	call base_function

other:
	cmp $0xdeadbeef, %ebx
	je another_label
	jmp halt

base_function:
	pop %ebx
	inc %ebx

halt:
	jmp halt	# HLT is prohibited, will cause GPF
