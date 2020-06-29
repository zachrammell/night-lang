.text
.intel_syntax noprefix
.globl main
main:
mov eax, 3
test eax, eax
xor eax, eax
setz al
test eax, eax
xor eax, eax
setz al
not eax
neg eax
ret
