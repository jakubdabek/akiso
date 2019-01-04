    .intel_syntax noprefix

two_pow_x:
  push ebp
  mov ebp, esp
  sub esp, 8
  mov eax, DWORD PTR [ebp+8]
  mov DWORD PTR [ebp-8], eax
  mov eax, DWORD PTR [ebp+12]
  mov DWORD PTR [ebp-4], eax
  fld QWORD PTR [ebp-8]
  fadd st, st(0)
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
  fld QWORD PTR .LC1
  fdivp st(1), st
  leave
  ret
.LC3:
  .string "sinh(%lf) = %lf\n"
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
  fld QWORD PTR .LC2
  fstp QWORD PTR [ebp-16]
  push DWORD PTR [ebp-12]
  push DWORD PTR [ebp-16]
  call sinh
  add esp, 8
  sub esp, 12
  lea esp, [esp-8]
  fstp QWORD PTR [esp]
  push DWORD PTR [ebp-12]
  push DWORD PTR [ebp-16]
  push OFFSET FLAT:.LC3
  call printf
  add esp, 32
  mov eax, 0
  mov ecx, DWORD PTR [ebp-4]
  leave
  lea esp, [ecx-4]
  ret
  .size    main, .-main

.LC1:
  .long 0
  .long 1073741824
.LC2:
  .long 0
  .long 1075052544
