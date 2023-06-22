[bits 64]

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

