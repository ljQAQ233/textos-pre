#ifndef __BOOT_H__
#define __BOOT_H__

typedef struct {
    UINT64 Magic;
} BOOT_CONFIG;

extern BOOT_CONFIG _BootConfig;

#endif
