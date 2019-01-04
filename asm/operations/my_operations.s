	.intel_syntax noprefix
	.text
	.globl	do_oper
	.type	do_oper, @function
do_oper:
    push ebp
    mov ebp, esp
    sub esp, 4
    mov al, [ebp+17]            # al = oper
    cmp eax, 43                 # cmp oper, '+'
    je case_plus
    cmp eax, 45                 # cmp oper, '-'
    je case_minus
    cmp eax, 42                 # cmp oper, '*'
    je case_multiply
    cmp eax, 47                 # cmp oper, '/'
    je case_divide
    jmp case_default
case_plus:
    fld DWORD PTR [ebp+8]
    fadd DWORD PTR [ebp+12]
    jmp do_oper_exit
case_minus:
    fld DWORD PTR [ebp+8]
    fsub DWORD PTR [ebp+12]
    jmp do_oper_exit
case_multiply:
    fld DWORD PTR [ebp+8]
    fmul DWORD PTR [ebp+12]
    jmp do_oper_exit
case_divide:
    fld DWORD PTR [ebp+8]
    fdiv DWORD PTR [ebp+12]
    jmp do_oper_exit
case_default:
    fldz
    fldz
    fdivp st(1), st
do_oper_exit:
    fstp DWORD PTR [ebp-4]
    mov eax, [ebp-4]
    leave
    ret

    .size do_oper, .-do_oper
    .section .rodata

scanf_format:
    .string "%f %c %f"
printf_format:
    .string "result = %f\n"

    .text
    .globl main
    .type main, @function
main:
    push ebp
    mov ebp, esp
    sub esp, 9                      # make room for 9 bytes worth of local variables (2 * float + char)
    lea eax, [ebp-8]
    push eax                        # push &right as last argument
    lea eax, [ebp-9]
    push eax                        # push &oper
    lea eax, [ebp-4]
    push eax                        # push &left
    lea eax, scanf_format           # get address of format string
    push eax                        # push format as first argument
    call __isoc99_scanf
    add esp, 16                     # restore stack pointer (4 arguments)
    # movzx eax, BYTE PTR [ebp-21]
    # movsx eax, al
    # fld DWORD PTR [ebp-16]
    # fld DWORD PTR [ebp-20]
    # sub esp, 4
    # push eax
    # lea esp, [esp-4]
    # fstp DWORD PTR [esp]
    # lea esp, [esp-4]
    # fstp DWORD PTR [esp]
    mov al, BYTE PTR [ebp-9]
    sub esp, 1
    mov [esp], al
    push DWORD PTR [ebp-8]          # push right
    push DWORD PTR [ebp-4]          # push left
    call do_oper
    add esp, 9                      # restore stack pointer (3 arguments)
    sub esp, 8
    mov DWORD PTR [esp], eax
    fld DWORD PTR [esp]
    fstp QWORD PTR [esp]
    lea eax, printf_format          # get address of format string
    push eax                        # push format as first argument to printf
    call printf
    add esp, 12                     # restore stack pointer (2 arguments)
    mov eax, 0                      # return value = 0
    leave
    ret
