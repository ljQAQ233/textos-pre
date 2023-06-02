#include <TextOS/TextOS.h>

#include <TextOS/Graphics.h>

u32 Hor;
u32 Ver;

u32 *FrameBuffer;
u64 FrameBufferSize;

void PixelPut (
        u32 X,u32 Y,
        u32 Color
        )
{
    if (X > Hor || Y > Hor) {
        return;
    }

    u32 *Pixel = FrameBuffer + X + Hor * Y;
    *Pixel = Color;
}

void BlockPut (
        u32 X,u32 Y,
        u32 eX,u32 eY,
        u32 Color
        )
{
    if (X > eX) {
        u32 Tmp = X;
        X = eX;
        eX = Tmp;
    }

    if (Y > eY) {
        u32 Tmp = Y;
        Y = eY;
        eY = Tmp;
    }

    for (u32 i = X;i < eX && i < Hor;i++) {
        for (u32 j = Y;j < eY && j < Ver;j++) {
            PixelPut (i,j,Color);
        }
    }
}

void ScreenClear ()
{
    u32 i = FrameBufferSize / 4;
    u32 *p = FrameBuffer;

    while (i-- && p)
    {
        *p++ = 0;
    }
}
