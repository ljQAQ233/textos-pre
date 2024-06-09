#include <TextOS/Console/PrintK.h>

void Syscall_Test (u64 rdi, u64 rsi, u64 rdx, u64 rcx, u64 r8, u64 r9)
{
    PrintK ("Syscall Test -> \n");
    PrintK (" Args : rdi=0x%016llx rsi=0x%016llx\n"
            "        rdx=0x%016llx rcx=0x%016llx\n"
            "        r8 =0x%016llx r9 =0x%016llx\n",
            rdi, rsi, rdx, rcx, r8, r9);
    PrintK ("Syscall Test <-\n");
}
