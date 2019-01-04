    .intel_syntax noprefix

two_pow_x:
    push ebp
    mov ebp, esp
    sub esp, 12
    mov eax, DWORD PTR [ebp+8]
    mov DWORD PTR [ebp-8], eax
    mov eax, DWORD PTR [ebp+12]
    mov DWORD PTR [ebp-4], eax
    fld QWORD PTR [ebp-8]
    fstcw WORD PTR [ebp-10]
    fstcw WORD PTR [ebp-12]
    or WORD PTR [ebp-12], 0xC00
    fldcw [ebp-12]
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
    fldcw [ebp-10]
    leave
    ret
sinh:
    push ebp
    mov ebp, esp
    sub esp, 8
    mov eax, DWORD PTR [ebp+8]
    mov DWORD PTR [ebp-8], eax
    mov eax, DWORD PTR [ebp+12]
    mov DWORD PTR [ebp-4], eax
    fld QWORD PTR [ebp-8]
    fldl2e
    fmulp
    lea esp, [esp-8]
    fstp QWORD PTR [esp]
    call two_pow_x
    fstp QWORD PTR [ebp-16]
    fld QWORD PTR [ebp-8]
    fchs
    fldl2e
    fmulp
    lea esp, [esp-8]
    fstp QWORD PTR [esp]
    call two_pow_x
    add esp, 8
    fld QWORD PTR [ebp-16]
    fsubrp
    fld1
    fld1
    faddp
    fdivp
    leave
    ret
sinh_inv:
    push ebp
    mov ebp, esp
    sub esp, 8
    mov eax, DWORD PTR [ebp+8]
    mov DWORD PTR [ebp-8], eax
    mov eax, DWORD PTR [ebp+12]
    mov DWORD PTR [ebp-4], eax
    fld QWORD PTR [ebp-8]
    fld st
    fmul st, st(1)
    fld1
    faddp
    fsqrt
    faddp
    fld1
    fxch
    fyl2x
    fstp st(1)
    fldl2e
    fdivp
    leave
    ret
.format1:
  .string "sinh(%.12lf) = %.12lf\n"
.format2:
  .string "sinh^-1(%.12lf) = %.12lf\n"
    .text
    .globl main
    .type main, @function
main:
    lea ecx, [esp+4]
    and esp, -16
    push DWORD PTR [ecx-4]
    push ebp
    mov ebp, esp
    push ecx
    sub esp, 20
    mov eax, ecx
    cmp DWORD PTR [eax], 1
    jg .L8
    mov eax, 1
    jmp .L9
.L8:
    mov eax, DWORD PTR [eax+4]
    add eax, 4
    mov eax, DWORD PTR [eax]
    sub esp, 12
    push eax
    call atof
    add esp, 16
    fstp QWORD PTR [ebp-16]
    sub esp, 8
    push DWORD PTR [ebp-12]
    push DWORD PTR [ebp-16]
    call sinh
    add esp, 16
    sub esp, 12
    lea esp, [esp-8]
    fstp QWORD PTR [esp]
    push DWORD PTR [ebp-12]
    push DWORD PTR [ebp-16]
    push OFFSET FLAT:.format1
    call printf
    add esp, 32
    sub esp, 8
    push DWORD PTR [ebp-12]
    push DWORD PTR [ebp-16]
    call sinh_inv
    add esp, 16
    sub esp, 12
    lea esp, [esp-8]
    fstp QWORD PTR [esp]
    push DWORD PTR [ebp-12]
    push DWORD PTR [ebp-16]
    push OFFSET FLAT:.format2
    call printf
    add esp, 32
    mov eax, 0
.L9:
    mov ecx, DWORD PTR [ebp-4]
    leave
    lea esp, [ecx-4]
    ret
    ret
  .size    main, .-main

.LC1:
  .long 996432413
  .long 1079953375
.LC2:
  .long 0
  .long 1073741824
.LC3:
  .long 0
  .long 1075052544
