[bits 64]

%define IDT_MAX 256
%define IDT_EXP 32
%define IHT_SIZ 16

extern IntrPtr
extern IntrCaller

section .text

%macro HANDLE_ENTRY 2
section .text

IntrHandler%1:
    ; SS -> RSP (original RSP) -> RFLAGS -> CS -> RIP
    ; - 1 - Vector
    ; - 2 - ErrorCode / Reserved only
    %%Head:
    %ifn %2
      push QWORD 2333
    %endif
    push QWORD %1
    jmp  _IntrCaller

    times IHT_SIZ - ($ - %%Head) db 0 ; Zero to fill

%endmacro

_IntrCaller:
    push  rax ; 保存寄存器 & 为后做铺垫
    push  rbx
    push  rcx
    push  rdx
    push  rbp
    push  rsi
    push  rdi
    push  r8
    push  r9
    push  r10
    push  r11
    push  r12
    push  r13
    push  r14
    push  r15

    mov  rdi, [rsp + 15 * 8] ; Vector
    mov  rsi, [rsp + 16 * 8] ; ErrorCode
    mov  rdx, rsp
    add  rdx, 17 * 8         ; Interrupt frame
    mov  rcx, rsp            ; Registers

    mov  rax, IntrPtr
    lea  rax, [rax + rdi * 8]
    call [rax]

    pop  r15 ; 恢复寄存器
    pop  r14
    pop  r13
    pop  r12
    pop  r11
    pop  r10
    pop  r9
    pop  r8
    pop  rdi
    pop  rsi
    pop  rbp
    pop  rdx
    pop  rcx
    pop  rbx
    pop  rax

    add     rsp, 16        ; 修正栈指针，跳过 Vector 和 ErrorCode
    iretq                  ; 中断返回

global IntrEntries
IntrEntries:
    HANDLE_ENTRY 0,  0
    HANDLE_ENTRY 1,  0
    HANDLE_ENTRY 2,  0
    HANDLE_ENTRY 3,  0
    HANDLE_ENTRY 4,  0
    HANDLE_ENTRY 5,  0
    HANDLE_ENTRY 6,  0
    HANDLE_ENTRY 7,  0
    HANDLE_ENTRY 8,  1
    HANDLE_ENTRY 9,  0
    HANDLE_ENTRY 10, 1
    HANDLE_ENTRY 11, 1
    HANDLE_ENTRY 12, 1
    HANDLE_ENTRY 13, 1
    HANDLE_ENTRY 14, 1
    HANDLE_ENTRY 15, 0
    HANDLE_ENTRY 16, 0
    HANDLE_ENTRY 17, 1
    HANDLE_ENTRY 18, 0
    HANDLE_ENTRY 19, 0
    HANDLE_ENTRY 20, 0
    HANDLE_ENTRY 21, 1
    HANDLE_ENTRY 22, 0
    HANDLE_ENTRY 23, 0
    HANDLE_ENTRY 24, 0
    HANDLE_ENTRY 25, 0
    HANDLE_ENTRY 26, 0
    HANDLE_ENTRY 27, 0
    HANDLE_ENTRY 28, 0
    HANDLE_ENTRY 29, 0
    HANDLE_ENTRY 30, 0
    HANDLE_ENTRY 31, 0

    %assign i IDT_EXP
    %rep IDT_MAX - IDT_EXP
        HANDLE_ENTRY i, 0
        %assign i (i + 1)
    %endrep
    
