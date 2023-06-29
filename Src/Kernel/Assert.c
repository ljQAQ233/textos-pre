#include <TextOS/Console/PrintK.h>

#include <Cpu.h>

void AssertK (
        const char *File,
        const u64  Line,
        const bool State,
        const char *Expr
        )
{
    if (!State) {
        PrintK ("[%s:%d] Assert failed!!! -> %s\n",File,Line,Expr);
    } else {
        return;
    }

    while (true)
        Halt(); // TODO : interrupt
}
