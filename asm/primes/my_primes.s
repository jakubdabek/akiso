    .intel_syntax noprefix
    .text
    .globl    is_prime
    .type    is_prime, @function
is_prime:
    push ebp
    mov ebp, esp
    sub esp, 4                  # make room for 4 bytes worth of local variables (int i)
    cmp DWORD PTR [ebp+8], 1
    jg is_prime_for_init        # if (a > 1) skip return
    mov eax, 0                  # return value = false
    jmp is_prime_exit
is_prime_for_init:
    mov DWORD PTR [ebp-4], 2    # i = 2
    jmp is_prime_for_condition
is_prime_for_body:
    mov eax, DWORD PTR [ebp+8]  # eax = a
    cdq                         # extend eax to edx:eax
    idiv DWORD PTR [ebp-4]      # divide edx:eax with remainder by i
    mov eax, edx                # put remainder in eax
    test eax, eax
    jne is_prime_for_increment  # skip return if remainder is not equal to 0
    mov eax, 0                  # return value = false
    jmp is_prime_exit
is_prime_for_increment:
    add DWORD PTR [ebp-4], 1    # i++
is_prime_for_condition:
    mov eax, DWORD PTR [ebp-4]  # eax = i
    cmp eax, DWORD PTR [ebp+8]  # cmp i, a
    jl is_prime_for_body        # if (i > a) continue for loop
    mov eax, 1                  # return value = true
is_prime_exit:
    leave
    ret

    .size is_prime, .-is_prime
    .section .rodata

format:
    .string "%d\n"

    .text
    .globl main
    .type main, @function
main:
    push ebp
    mov ebp, esp
    sub esp, 4                  # make room for 4 bytes worth of local variables (int i)
    mov DWORD PTR [ebp-4], 1    # i = 1
    jmp main_for_condition
main_for_body:
    push DWORD PTR [ebp-4]      # push i as argument
    call is_prime
    add esp, 4                  # restore stack pointer (1 argument)
    test al, al                 # test return value of is_prime (bool has size of a byte)
    je main_for_increment       # skip printf if i is not prime
    push DWORD PTR [ebp-4]      # push i as second argument to printf
    lea eax, format             # get address of format string
    push eax                    # push format as first argument to printf
    call printf
    add esp, 8                  # restore stack pointer (2 arguments)
main_for_increment:
    add DWORD PTR [ebp-4], 1    # i++
main_for_condition:
    cmp DWORD PTR [ebp-4], 9999 # cmp i, 9999
    jle main_for_body           # if (i <= 9999) continue for loop
    mov eax, 0                  # return value = 0
    leave
    ret

    .size    main, .-main
