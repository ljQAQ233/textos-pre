#include <Cpu.h>
#include <Intr.h>
#include <string.h>
#include <TextOS/Assert.h>
#include <TextOS/Console/PrintK.h>

typedef struct _packed {
    u16 Limit;
    u64 Base;
} Idtr_t;

typedef struct _packed {
    u16 OffsetLow : 16;
    u16 Selector  : 16;
    u8  IST       : 2 ;
    u8  Reserved  : 6 ;
    u8  Type      : 4 ;
    u8  Zero      : 1 ;
    u8  DPL       : 2 ;
    u8  Present   : 1 ;
    u16 OffsetMid : 16;
    u32 OffsetHgh : 32;
    u32 Reserved2 : 32;
} Idt_t;

#define IDT_MAX 256
#define IHT_SIZ 16

Idt_t Idts[IDT_MAX];
Idtr_t Idtr;

static void _IdtSetEntry (size_t i,u64 Offset, u16 Selector, u8 Type, u8 DPL, u8 Present)
{
    Idt_t *Ptr = &Idts[i];

    Ptr->OffsetLow = Offset & 0xffff;
    Ptr->OffsetMid = Offset >> 16 & 0xffff;
    Ptr->OffsetHgh = Offset >> 32;
    Ptr->Selector = Selector;

    Ptr->DPL  = DPL;
    Ptr->Type = Type;

    Ptr->IST  = 0;
    Ptr->Present = 1;
}

#define SELE_KERN 8 // For kernel code segment

#define GATE_INT  0b1110
#define GATE_TRAP 0b1111

#define _IDT_SET_ENTRY(i, Offset) \
    _IdtSetEntry (i, Offset, SELE_KERN, GATE_INT, 0, 1)

IntrCaller_t IntrPtr[IDT_MAX];

static char *ExptMessage[] = {
    "#DE Divide Error\0",
    "#DB Debug Exception\0",
    "--- NMI Interrupt\0",
    "#BP Breakpoint\0",
    "#OF Overflow\0",
    "#BR BOUND Range Exceeded\0",
    "#UD Invalid Opcode (Undefined Opcode)\0",
    "#NM Device Not Available (No Math Coprocessor)\0",
    "#DF Double Fault\0",
    "--- Coprocessor Segment Overrun (reserved)\0",
    "#TS Invalid TSS\0",
    "#NP Segment Not Present\0",
    "#SS Stack-Segment Fault\0",
    "#GP General Protection\0",
    "#PF Page Fault\0",
    "---  (Intel reserved. Do not use)\0",
    "#MF x87 FPU Floating-Point Error (Math Fault)\0",
    "#AC Alignment Check\0",
    "#MC Machine Check\0",
    "#XM SIMD Floating-Point Exception\0",
    "#VE Virtualization Exception\0",
    "#CP Control Protection Exception\0",
};

static char *RevMessage  = "--- Intel reserved. Do not use\0";
static char *UserMessage = "--- User defined (Non-reserved) Interrupts\0";

static inline char *MsgGet (u8 Vector)
{
    if (Vector < 22)
        return ExptMessage[Vector];
    else if (Vector > 31)
        return UserMessage;

    return RevMessage;
}

__INTR_FUNC (IntrCommon)
{
    PrintK ("--------------------------------\n");
    PrintK ("Interrupt occurred !!! - %03x -> %s\n", Vector, MsgGet (Vector));
    PrintK ("RAX=%016llx RBX=%016llx RCX=%016llx RDX=%016llx\n", Reg->rax, Reg->rbx, Reg->rcx, Reg->rdx);
    PrintK ("RSI=%016llx RDI=%016llx RBP=%016llx RSP=%016llx\n", Reg->rsi, Reg->rdi, Reg->rbp, Intr->rsp);
    PrintK ("--------------------------------\n");
    PrintK ("R8 =%016llx R9 =%016llx R10=%016llx R11=%016llx\n", Reg->r8 , Reg->r9 , Reg->r10, Reg->r11);
    PrintK ("R12=%016llx R13=%016llx R14=%016llx R15=%016llx\n", Reg->r12, Reg->r13, Reg->r14, Reg->r15);
    PrintK ("--------------------------------\n");
    PrintK ("RIP=%016llx RFL=%08llx ERR=%016llx\b", Intr->rip, Intr->rflags, ErrorCode);
    PrintK ("--------------------------------\n");
    
    while (true) ;
}

extern u8 IntrEntries; // a start point of the whole table

void InitializeIdt ()
{
    ASSERTK (sizeof(Idt_t) == 16);

    memset (Idts, 0, IDT_MAX * sizeof(Idt_t));

    for (size_t i = 0; i < IDT_MAX; i++)
    {
        _IDT_SET_ENTRY (i, (u64)(&IntrEntries + IHT_SIZ * i));
        IntrPtr[i] = (IntrCaller_t)IntrCommon;
    }

    _IdtSetEntry (INT_SYSCALL, (u64)(&IntrEntries + IHT_SIZ * INT_SYSCALL),
                 KERN_CODE_SEG << 3, GATE_INT, 3, 1);

    Idtr.Base = (u64)&Idts;
    Idtr.Limit = IDT_MAX * sizeof(Idt_t) - 1;

    LoadIdt (&Idtr);
}

void IntrRegister (u8 Vector, IntrCaller_t Func)
{
    IntrPtr[Vector] = Func;
}
