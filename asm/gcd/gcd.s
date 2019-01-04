.LC0:
  .ascii "Enter two integers: \000"
.LC1:
  .ascii "%d %d\000"
.LC2:
  .ascii "GCD of %d and %d is %d\000"
main:
  push {fp, lr}
  add fp, sp, #4
  sub sp, sp, #16
  ldr r0, .L7
  bl puts
  sub r2, fp, #20
  sub r3, fp, #16
  mov r1, r3
  ldr r0, .L7+4
  bl __isoc99_scanf
  mov r3, #1
  str r3, [fp, #-8]
  b .L2
.L5:
  ldr r3, [fp, #-16]
  ldr r1, [fp, #-8]
  mov r0, r3
  bl __aeabi_idivmod
  mov r3, r1
  cmp r3, #0
  bne .L3
  ldr r3, [fp, #-20]
  ldr r1, [fp, #-8]
  mov r0, r3
  bl __aeabi_idivmod
  mov r3, r1
  cmp r3, #0
  bne .L3
  ldr r3, [fp, #-8]
  str r3, [fp, #-12]
.L3:
  ldr r3, [fp, #-8]
  add r3, r3, #1
  str r3, [fp, #-8]
.L2:
  ldr r3, [fp, #-16]
  ldr r2, [fp, #-8]
  cmp r2, r3
  bgt .L4
  ldr r3, [fp, #-20]
  ldr r2, [fp, #-8]
  cmp r2, r3
  ble .L5
.L4:
  ldr r1, [fp, #-16]
  ldr r2, [fp, #-20]
  ldr r3, [fp, #-12]
  ldr r0, .L7+8
  bl printf
  mov r3, #0
  mov r0, r3
  sub sp, fp, #4
  pop {fp, pc}
.L7:
  .word .LC0
  .word .LC1
  .word .LC2