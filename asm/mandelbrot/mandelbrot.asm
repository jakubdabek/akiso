    lea di, Screen
    mov dword ptr [CntrA], -510*256
    mov word ptr [X], 0
@@LoopHoriz:
    mov dword ptr [CntrB], -270*256
    mov word ptr [Y], 200
    ; x <- x2 - y2 + C
    ; y <- 2xy + C
    xor ecx,ecx ;x=0
    xor edx,edx ;y=0
    mov si,32-1 ;kolor
@@LoopFractal:
    mov eax,ecx
    imul eax,eax ;x2
    mov ebx,edx
    imul ebx,ebx ;y2
    sub eax,ebx ;x2-y2
    add eax,dword ptr [CntrA] ;x2-y2+C
    mov ebx,ecx
    imul ebx,edx ;xy
    sal ebx,1 ;2xy
    add ebx, dword ptr[CntrB] ;2xy+C
    sar eax,8
    sar ebx,8
    mov ecx,eax
    mov edx,ebx
    imul eax,eax ;x2
    imul ebx,ebx ;y2
    add eax,ebx  ;x2+y2
    sar eax,8
    cmp eax,1024 ;if (x2+y2)>1024 then
    jg Break     ;break
    dec si       ;kolor--
    jnz @@LoopFractal

Break:
    mov ax,si
    mov byte ptr [di], al
    add dword ptr [CntrB],720
    add di,320
    dec word ptr [Y]
    jnz @@LoopVert
    add dword ptr [CntrA],568
    inc word ptr [x]
    lea di,Screen
    add di,word ptr [x]
    cmp word ptr [X],320
    jnz @@LoopHoriz
    ret