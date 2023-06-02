#include <TextOS/TextOS.h>

#include <Boot.h>

#include <TextOS/Graphics.h>

void __GraphicsInit (BOOT_CONFIG *Config)
{
    Hor = Config->Graphics.Hor;
    Ver = Config->Graphics.Ver;
    FrameBuffer = (u32 *)Config->Graphics.FrameBuffer;
    FrameBufferSize = Config->Graphics.FrameBufferSize;
}

