#ifndef __PANIC_H__
#define __PANIC_H__

void Panic (
        const char *File,
        const u64  Line,
        const char *Format,
        ...
        );

// 可选方案:
// #define Panic(File, Line, Format, ARGS) \
//         DebugK (File, Line, "Panic!!! ->" Format, ##ARGS)

#define PANIC(Format, ARGS...) \
        Panic(__FILE__, __LINE__, Format, ##ARGS)

#endif
