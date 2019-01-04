	.file	"primes.c"
	.intel_syntax noprefix
	.text
	.globl	is_prime
	.type	is_prime, @function
is_prime:
.LFB0:
	.cfi_startproc
	push	ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	mov	ebp, esp
	.cfi_def_cfa_register 5
	sub	esp, 16
	cmp	DWORD PTR [ebp+8], 1
	jg	.L2
	mov	eax, 0
	jmp	.L3
.L2:
	mov	DWORD PTR [ebp-4], 2
	jmp	.L4
.L6:
	mov	eax, DWORD PTR [ebp+8]
	cdq
	idiv	DWORD PTR [ebp-4]
	mov	eax, edx
	test	eax, eax
	jne	.L5
	mov	eax, 0
	jmp	.L3
.L5:
	add	DWORD PTR [ebp-4], 1
.L4:
	mov	eax, DWORD PTR [ebp-4]
	cmp	eax, DWORD PTR [ebp+8]
	jl	.L6
	mov	eax, 1
.L3:
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE0:
	.size	is_prime, .-is_prime
	.section	.rodata
.LC0:
	.string	"%d\n"
	.text
	.globl	main
	.type	main, @function
main:
.LFB1:
	.cfi_startproc
	lea	ecx, [esp+4]
	.cfi_def_cfa 1, 0
	and	esp, -16
	push	DWORD PTR [ecx-4]
	push	ebp
	.cfi_escape 0x10,0x5,0x2,0x75,0
	mov	ebp, esp
	push	ecx
	.cfi_escape 0xf,0x3,0x75,0x7c,0x6
	sub	esp, 20
	mov	DWORD PTR [ebp-12], 1
	jmp	.L8
.L10:
	push	DWORD PTR [ebp-12]
	call	is_prime
	add	esp, 4
	test	al, al
	je	.L9
	sub	esp, 8
	push	DWORD PTR [ebp-12]
	push	OFFSET FLAT:.LC0
	call	printf
	add	esp, 16
.L9:
	add	DWORD PTR [ebp-12], 1
.L8:
	cmp	DWORD PTR [ebp-12], 9999
	jle	.L10
	mov	eax, 0
	mov	ecx, DWORD PTR [ebp-4]
	.cfi_def_cfa 1, 0
	leave
	.cfi_restore 5
	lea	esp, [ecx-4]
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE1:
	.size	main, .-main
	.ident	"GCC: (Ubuntu 8.1.0-5ubuntu1~16.04) 8.1.0"
	.section	.note.GNU-stack,"",@progbits
