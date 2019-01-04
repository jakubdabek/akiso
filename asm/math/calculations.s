    .intel_syntax noprefix

singleinput:     .long 0
singleoutput:    .long 0
SaveCW:          .value 0
MaskedCW:        .value 0

twotothepower:
    fstcw SaveCW
    fstcw MaskedCW
    mov ax, MaskedCW
    or ax, 0xC0
    mov MaskedCW, ax
    fldcw MaskedCW
    fld DWORD PTR [singleinput]
    fld st
    fld st
    frndint
    fxch 
    fsub st, st(1) 
    f2xm1 
    fld1
    faddp 
    fxch 
    fld1 
    fscale
    fstp st(1) 
    fmulp 
    fldcw [SaveCW] 
    fstp DWORD PTR [singleoutput]
    ret

sinh:
    push ebp
    mov ebp, esp
    sub esp, 8
    mov eax, DWORD [singleinput]
    mov [esp], eax


format:
    .string "%lf\n"
value:
    .long 1.541241

    .text
    .globl main
    .type main, @function
main:
    push ebp
    mov ebp, esp
    fld DWORD PTR [value]
    sub esp, 8
    fstp [esp]
    lea eax, format
    push eax
    call printf
    add esp, 12
