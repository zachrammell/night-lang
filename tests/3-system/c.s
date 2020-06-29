	.text
	.def	 @feat.00;
	.scl	3;
	.type	0;
	.endef
	.globl	@feat.00
.set @feat.00, 0
	.intel_syntax noprefix
	.file	"translated.c"
	.def	 main;
	.scl	2;
	.type	32;
	.endef
	.globl	main                    # -- Begin function main
	.p2align	4, 0x90
main:                                   # @main
.seh_proc main
# %bb.0:
	sub	rsp, 40
	.seh_stackalloc 40
	.seh_endprologue
	mov	ecx, -11
	call	qword ptr [rip + __imp_GetStdHandle]
	mov	qword ptr [rsp + 32], 0
	lea	rdx, [rip + "??_C@_0O@NFOCKKMG@Hello?5World?$CB?6?$AA@"]
	mov	rcx, rax
	mov	r8d, 13
	xor	r9d, r9d
	call	qword ptr [rip + __imp_WriteConsoleA]
	xor	eax, eax
	add	rsp, 40
	ret
	.seh_handlerdata
	.text
	.seh_endproc
                                        # -- End function
	.section	.rdata,"dr",discard,"??_C@_0O@NFOCKKMG@Hello?5World?$CB?6?$AA@"
	.globl	"??_C@_0O@NFOCKKMG@Hello?5World?$CB?6?$AA@" # @"??_C@_0O@NFOCKKMG@Hello?5World?$CB?6?$AA@"
"??_C@_0O@NFOCKKMG@Hello?5World?$CB?6?$AA@":
	.asciz	"Hello World!\n"

	.section	.drectve,"yn"
	.ascii	" /DEFAULTLIB:uuid.lib"
	.ascii	" /DEFAULTLIB:uuid.lib"
	.addrsig
