#include <Intr.h>
#include <Irq.h>

#include <TextOS/Syscall.h>

extern void Syscall_Test (u64 rdi, u64 rsi, u64 rdx, u64 rcx, u64 r8, u64 r9);

static void *Syscalls[] = {
    [SYSCALL_TEST] = Syscall_Test,
};

#include <TextOS/Panic.h>

__INTR_FUNC (SyscallHandler)
{
    if (Reg->rax >= SYSCALL_MAX)
        PANIC("Syscall number was out of range!\n");

    __asm__ volatile (
            "movq %0, %%rdi\n" // arg0
            "movq %1, %%rsi\n" // arg1
            "movq %2, %%rdx\n" // arg2
            "movq %3, %%rcx\n" // arg3
            "movq %4, %%r8 \n" // arg4
            "movq %5, %%r9 \n" // arg5
            "callq %%rax"      // handler
            :
            : "m"(Reg->rdi), "m"(Reg->rsi), "m"(Reg->rdx), "m"(Reg->rcx), "m"(Reg->r8), "m"(Reg->r9),
              "a"(Syscalls[Reg->rax])                     // 存放处理函数
            : "%rdi", "%rsi", "%rdx", "rcx", "r8", "r9"); // 如果不告诉 gcc 我们改变了哪些寄存器, 它就有可能用这些寄存器进行寻址...
}

void SyscallInit ()
{
    IntrRegister (INT_SYSCALL, SyscallHandler);
}
