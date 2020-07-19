STD_OUTPUT_HANDLE   equ -11
NULL                equ 0

global main
extern _ExitProcess@4, _GetStdHandle@4, _WriteConsoleA@20

section .data
msg                 db "Hello, World!", 13, 10
msg.len             equ $ - msg

section .bss
dummy               resd 1

section .text
main: 
  mov	    rcx, STD_OUTPUT_HANDLE
  call    _GetStdHandle@4

  push    NULL
  mov	    r9d, NULL
  mov     r8d, msg.len
  lea	    rdx, msg
  mov	    rcx, rax
  call    _WriteConsoleA@20

  mov     rcx, 0
  call    _ExitProcess@4
