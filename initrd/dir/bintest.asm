
; bintest.asm - true test of our kernel real mode

[BITS 32]

start:
xor eax, eax
xor ebx, ebx
xor ecx, ecx
xor edx, edx
mov ax, 0xffff
mov ebx, 0xdeadbeef
jmp short label_3

label_2:
mov ebx, 0xdeadb00b
mov edx, (esp)
jmp short label_4

label_3:
mov ebx, 0xcafebabe
mov ecx, 0xf0f00f0f
push ecx
xor ecx, ecx
call label_2

label_4:
mov eax, 1
mov esi, _text
mov ebx, 0xbadca11
int 0x80
jmp infinite


infinite:
jmp short infinite	; HLT is prohibited

_text: db "Hello User World!",13,10,0
