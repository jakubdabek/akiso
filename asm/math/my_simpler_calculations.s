section .data
    saved_cw:   DW 0
    masked_cw   DW 0

section .text
    global main
    extern printf
    extern atof

    format1:    DB "sinh(%.12lf) = %.12lf",10,0
    format2:    DB "sinh^-1(%.12lf) = %.12lf",10,0

main:
    push ebp
    mov ebp, esp
    cmp DWORD [ebp+8], 1
    jg .do_it
    mov eax,1
    jmp .exit
.do_it:
    mov eax, [ebp+12]
    mov eax, [eax+4]
    push eax
    call atof
    add esp, 4
    fld st0
    call sinh
    lea esp, [esp-8]
    fstp QWORD [esp]
    lea esp, [esp-8]
    fst QWORD [esp]
    push format1
    call printf
    add esp,20
    fld st0
    call arcsinh
    lea esp, [esp-8]
    fstp QWORD [esp]
    lea esp, [esp-8]
    fst QWORD [esp]
    push format2
    call printf
    add esp,20
    mov eax,0
.exit:
    leave
    ret

two_pow_x:
    fstcw [saved_cw]
    fstcw [masked_cw]
    or WORD [masked_cw], 0x0C00
    fldcw [masked_cw]

    fld st0
    frndint         ; st0=int(x), st1=x
    fsub st1, st0   ; st0=int(x), st1=frac(x)
    fld1
    fscale
    fstp st1        ; st0=2^int(x), st1=frac(x)
    fxch
    f2xm1
    fld1
    faddp           ; st0=2^frac(x), st1=2^int(x)
    fmulp

    fldcw [saved_cw]
    ret

sinh:
    fldl2e
    fmulp           ; st0=x*log_2(e)
    fld st0
    call two_pow_x
    fxch
    fchs
    call two_pow_x  ; st0=e^(-x), st1=e^x
    fsubp           ; st0=e^x-e^(-x)
    fld1
    fadd st0, st0
    fdivp
    ret

arcsinh:
    fld st0
    fmul st0, st0
    fld1
    faddp
    fsqrt
    faddp
    fld1
    fxch
    fyl2x
    fldl2e
    fdivp
    ret