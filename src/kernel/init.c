#include <textos/textos.h>
#include <textos/debug.h>

#include <boot.h>

extern void kernel_main();

extern void __video_pre (vconfig_t *v);

void kernel_init (bconfig_t *config)
{
    dprintk_set(K_ALL & ~K_SYNC);

    __video_pre (&config->video);

    kernel_main();
}

