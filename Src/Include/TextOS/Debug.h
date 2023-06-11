#ifndef __DEBUG_H__
#define __DEBUG_H__

void DebugK (
        const char *File,
        const u64  Line,
        const char *Format,
        ...
        );

#ifdef __TEXTOS_DEBUG
    #define DEBUGK(Format, ARGS...) \
            DebugK (__FILE__, __LINE__, Format, ##ARGS)
#else
    #define DEBUGK(Format, ARGS...) 
#endif

#endif
