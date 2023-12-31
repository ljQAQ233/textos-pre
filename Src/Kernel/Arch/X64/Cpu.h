#ifndef __CPU_H__
#define __CPU_H__

typedef struct IntrFrame {
  u64 rip;
  u64 cs;
  u64 rflags;
  u64 rsp;
  u64 ss;
} IntrFrame_t;

/* 可以指向 r15 寄存器在内存中的位置,
   以获取发生异常时寄存器信息.        */
typedef struct RegFrame {
  u64 r15, r14, r13, r12, r11, r10, r9, r8;
  u64 rdi, rsi;
  u64 rbp;
  u64 rdx, rcx, rbx, rax;
} RegFrame_t;

void ReadGdt (void *Gdtr);

void LoadGdt (void *Gdtr);

void ReadIdt (void *Idtr);

void LoadIdt (void *Idtr);

void ReloadSegs ();

u64 ReadCr3 ();

u64 WriteCr3 ();

#define IA32_APIC_BASE 0x1B

u64 ReadMsr (u32 Idx);

void WriteMsr (u32 Idx, u64 Value);

#endif
