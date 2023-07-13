[bits 64]

; void Halt ();
global Halt
Halt:
    hlt
    ret

; void ReadGdt (void *Gdtr);
global ReadGdt
ReadGdt:
    sgdt [rdi]
    ret

; void LoadGdt (void *Gdtr);
global LoadGdt
LoadGdt:
    lgdt [rdi]
    ret

; void ReadIdt (void *Idtr);
global ReadIdt
ReadIdt:
    sidt [rdi]
    ret

; void LoadIdt (void *Idtr);
global LoadIdt
LoadIdt:
    lidt [rdi]
    ret

; void ReloadSegs ();
global ReloadSegs
ReloadSegs:
  lea  rdi, [rdi * 8]
  lea  rsi, [rsi * 8]

  mov ax, 0x00
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax

  ; mov  rax, [rsp]
  mov  rbx, rsp
  push rdi      ; ss
  push rbx      ; rsp
  pushf         ; rflags
  push rsi      ; cs
  lea  rax, [rel .ret]
  push rax      ; rip

  iretq
  .ret: ; iretq jump here
    ret ; recovery rip

; u64 ReadCr3 ();
global ReadCr3
ReadCr3:
    mov rax, cr3
    ret

; void WriteCr3 ();
global WriteCr3
WriteCr3:
    mov cr3, rdi
    ret

; void __StackInit ();
global __StackInit
__StackInit:
    pop  rbx
    lea  rax, [rel __StackTop]
    mov  rbp, rax
    mov  rsp, rax
    push rbx
    ret

section .data
global __Stack
__Stack:
    times 0x1000 dq 0
    __StackTop:
