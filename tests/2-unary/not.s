.text
.intel_syntax noprefix
.globl main
main:
mov eax, 0
test eax, eax
xor eax, eax
setz al
ret
