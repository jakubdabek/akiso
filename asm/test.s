format DB "%c%c xddddddd",10,0

section .text
    global main
    extern printf

main:
    push ebp
    mov ebp, esp
    ;mov al, '5'
    ;lea esp, [esp-1]
    ;mov BYTE [esp], al
    push DWORD [format+1]
    push DWORD [format]
    push format
    call printf
    leave
    ret