is_prime:
  pushl %ebp
  movl %esp, %ebp
  subl $16, %esp
  cmpl $1, 8(%ebp)
  jg .L2
  movl $0, %eax
  jmp .L3
.L2:
  movl $2, -4(%ebp)
  jmp .L4
.L6:
  movl 8(%ebp), %eax
  cltd
  idivl -4(%ebp)
  movl %edx, %eax
  testl %eax, %eax
  jne .L5
  movl $0, %eax
  jmp .L3
.L5:
  addl $1, -4(%ebp)
.L4:
  movl -4(%ebp), %eax
  cmpl 8(%ebp), %eax
  jl .L6
  movl $1, %eax
.L3:
  leave
  ret
.LC0:
  .string "%d\n"
main:
  leal 4(%esp), %ecx
  andl $-16, %esp
  pushl -4(%ecx)
  pushl %ebp
  movl %esp, %ebp
  pushl %ecx
  subl $20, %esp
  movl $1, -12(%ebp)
  jmp .L8
.L10:
  pushl -12(%ebp)
  call is_prime
  addl $4, %esp
  testb %al, %al
  je .L9
  subl $8, %esp
  pushl -12(%ebp)
  pushl $.LC0
  call printf
  addl $16, %esp
.L9:
  addl $1, -12(%ebp)
.L8:
  cmpl $9999, -12(%ebp)
  jle .L10
  movl $0, %eax
  movl -4(%ebp), %ecx
  leave
  leal -4(%ecx), %esp
  ret