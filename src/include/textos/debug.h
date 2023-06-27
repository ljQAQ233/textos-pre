#ifndef __DEBUG_H__
#define __DEBUG_H__

void dprintk(int lv, const char *format, ...);

#define K_NONE  0x00 // none
#define K_LOGK  0x01 // log
#define K_WARN  0x02 // warn
#define K_ERRO  0x04 // error
#define K_SYNC  0x08 // sync to console

#define K_MM    0x10   // memory manager
#define K_TASK  0x20   // task manager
#define K_DEV   0x40   // device
#define K_FS    0x80   // file system
#define K_PIC   0x100  // pic / apic
#define K_INIT  0x200  // initializer

#define K_ALL   ((K_INIT << 1) - 1)

#define K_NMSK  0x20   // non maskable

int dprintk_set(int mask);

void debugk(int lv, const char *file, const int line, const char *format, ...);

#if !defined(kconf_release)
    #define DEBUGK(lv, format, ARGS...) \
            debugk(lv, __FILE__, __LINE__, format, ##ARGS)
#else
    #define DEBUGK(format, ARGS...) 
#endif

#endif
