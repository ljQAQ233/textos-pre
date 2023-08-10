[bits 64]

global __TaskSwitch
__TaskSwitch:
    push rbp
    mov  rbp, rsp
    
    mov  [rsi], rsp
    mov  rsp, rdi

    pop  rbp
    ret

