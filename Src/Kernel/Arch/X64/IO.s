
; u8 InB (u16 Port);
global InB
InB:
    xor rax, rax
    mov dx,  di
    in  al,  dx
    ret

; u16 InW (u16 Port);
global InW
InW:
    xor rax, rax
    mov dx,  di
    in  ax,  dx
    ret

; u32 InDW (u16 Port);
global InDW
InDW:
    xor rax, rax
    mov dx,  di
    in  eax, dx
    ret

; void OutB (u16 Port, u8 Data);
global OutB
OutB:
    mov dx, di
    mov al, sil
    out dx, al
    ret

; void OutW (u16 Port, u16 Data);
global OutW
OutW:
    mov dx, di
    mov ax, si
    out dx, ax
    ret

; void OutDW (u16 Port, u32 Data);
global OutDW
OutDW:
    mov dx,  di
    mov eax, esi
    out dx,  eax
    ret
