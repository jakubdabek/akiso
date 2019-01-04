
; nasm -fbin boot.asm
; qemu-system-i386 -drive format=raw,file=boot

bits 16
org 0x7c00

boot:
    mov ax, 0x2401
    int 0x15
    mov ax, 0x13        ; wlaczenie trybu graficznego 320x200
    int 0x10
    cli
    lgdt [gdt_pointer]  ; ustawienie tablicy GDT
    mov eax, cr0        ; wlaczenie trybu chronionego
    or eax,0x1
    mov cr0, eax
    jmp CODE_SEG:boot2
gdt_start:              ; tablica GDT
    dq 0x0
gdt_code:
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10011010b
    db 11001111b
    db 0x0
gdt_data:
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0
gdt_end:
gdt_pointer:
    dw gdt_end - gdt_start
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

bits 32
putpixel:
    push ebx
    mov ebx, DWORD [current_pixel]
    mov BYTE [ebx],al
    add ebx,1
    mov DWORD [current_pixel], ebx
    pop ebx
    ret
current_pixel dd 0xa0000

boot2:
    sub esp, 20
.L2:
    mov DWORD  [esp+8], esi
    fild DWORD  [esp+8]
    xor ebx, ebx
    fsub DWORD  [.LC4]
    fmul DWORD  [.LC2]
    fdiv DWORD  [.LC3]
.L8:
    mov DWORD  [esp+8], ebx
    fild DWORD  [esp+8]
    xor eax, eax
    fsub DWORD  [.LC1]
    fmul DWORD  [.LC2]
    fdiv DWORD  [.LC3]
    fldz
    fldz
    fldz
    fldz
    jmp .L5
.L20:
    cmp eax, 1024
    je .L21
    fxch st1
    fxch st2
    fxch st3
    fxch st2
.L5:
    fsubrp st1, st0
    add eax, 1
    fadd st0, st3
    fxch st1
    fadd st0, st0
    fmulp st2, st0
    fxch st1
    fadd st0, st3
    fld st1
    fmul st0, st2
    fld st1
    fmul st0, st2
    fld st1
    fadd st0, st1
    fld DWORD  [.LC2]
    fcomip st0, st1
    fstp st0
    jnb .L20
    fstp st0
    fstp st0
    fstp st0
    fstp st0
    fstp st0
    cmp eax, 1024
    je .L6
    fstp QWORD  [esp+8]
    sub esp, 4
    push eax
.L18:
    push esi
    push ebx
    add ebx, 1
    call putpixel
    add esp, 16
    fld QWORD  [esp+8]
    cmp ebx, 320
    jne .L8
    fstp st0
    add esi, 1
    cmp esi, 200
    jne .L2
    add esp, 20
    jmp .halt
.L21:
    fstp st0
    fstp st0
    fstp st0
    fstp st0
    fstp st0
.L6:
    fstp QWORD  [esp+8]
    sub esp, 4
    push 0
    jmp .L18
.halt:
    cli
    hlt
.LC1:
  dq 1126170624
.LC2:
  dq 1082130432
.LC3:
  dq 1134559232
.LC4:
  dq 1120403456

times 510 - ($-$$) db 0
dw 0xaa55
