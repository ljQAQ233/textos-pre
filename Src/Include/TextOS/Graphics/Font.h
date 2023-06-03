#pragma once

struct _FONT_INFO
{
  u8  *Base;   // 位图地址
  u8  Width;   // 宽
  u8  Height;  // 高
};

typedef struct _FONT_INFO FontInfo_t;

/*
   Display a character `Code` on the screen,
   using the specific `Font`
*/
int FontShow (
  u8 Code,
  FontInfo_t *Font,
  u64 X, u64 Y,
  u32 FGColor, u32 BGColor
);

/* Get a font info from the built-in array */
FontInfo_t *FontGet (int Idx);
