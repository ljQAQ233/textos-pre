#include <cpu.h>
#include <textos/debug.h>

void assertk (
        const char *file,
        const u64  line,
        const bool state,
        const char *expr
        )
{
    if (!state) {
        dprintk(K_SYNC, "assertion failed!!! -> %s\n", expr);
    } else {
        return;
    }

    while (true) halt(); // TODO : interrupt
}
