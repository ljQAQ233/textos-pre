#include <textos/debug.h>

void assertk (
        const char *file,
        const u64  line,
        const bool state,
        const char *expr
        )
{
    if (!state) {
        debugk(K_SYNC, file, line, "[%s:%d] Assert failed!!! -> %s\n", expr);
    } else {
        return;
    }

    while (true) ; // TODO : interrupt
}
