#ifndef __PANIC_H__
#define __PANIC_H__

void Panic (
        const char *File,
        const u64  Line,
        const char *Format,
        ...
        );

// 可选方案:
// #include <Cpu.h>
// #define Panic(File, Line, Format, ARGS)                        \
//         do {                                                   \
//             DebugK (File, Line, "Panic!!! ->" Format, ##ARGS)  \
//             while (true) Halt();                               \
//         while (false);

#define PANIC(Format, ARGS...) \
        Panic(__FILE__, __LINE__, Format, ##ARGS)

#endif
