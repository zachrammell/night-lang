.text
.intel_syntax noprefix
.globl main
main:
mov eax, 4
test eax, eax
xor eax, eax
setz al
test eax, eax
xor eax, eax
setz al
ret
