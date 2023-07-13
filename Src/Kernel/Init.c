#include <TextOS/TextOS.h>

#include <Boot.h>

#include <string.h>

extern void __GraphicsInit (BOOT_CONFIG *Config);
extern void __MemoryInit (BOOT_CONFIG *Config);

extern void KernelMain();

BOOT_CONFIG _BootConfig;

static void _ConfigSave (BOOT_CONFIG *Src)
{
    memcpy (&_BootConfig, Src, sizeof (BOOT_CONFIG));
}

extern void __StackInit ();

void KernelInit (BOOT_CONFIG *Config)
{
    _ConfigSave (Config);

    __GraphicsInit (Config);
    __MemoryInit (Config);

    // To create a temporary stack
    __StackInit();

    KernelMain();
}

