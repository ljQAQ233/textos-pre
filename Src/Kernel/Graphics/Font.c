#include <TextOS/TextOS.h>

#include <TextOS/Graphics.h>

/*
  Show a letter by Code.

  @retval int  The index of the bitmap according to `Font`.
*/
int FontShow (
        u8 Code,
        FontInfo_t *Font,
        u64 X, u64 Y,
        u32 FGColor, u32 BGColor
        )
{
    u16 Siz = Font->Width * Font->Height / 8;
    u8 *Base = Font->Base + Code * Siz;

    u8 Source = 0;
    for (UINT32 iY = 0; iY < Font->Height; iY++)
    {
        for (UINT32 iX = 0; iX < Font->Width; iX++)
        {
            Source = Base[(iY * Font->Width + iX) / 8];

            if (Source >> (7 - iX % 8) & 0x1) {
                PixelPut (X + iX, Y + iY, FGColor);
            } else {
                PixelPut (X + iX, Y + iY, BGColor);
            }
        }
    }

    return Code;
}
