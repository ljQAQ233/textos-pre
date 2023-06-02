#include <TextOS/TextOS.h>

#include <Boot.h>

extern void __GraphicsInit (BOOT_CONFIG *Config);

extern void KernelMain();

void KernelInit (BOOT_CONFIG *Config)
{
    __GraphicsInit (Config);

    KernelMain();
}

