#pragma once

struct _FONT_INFO
{
  u8  *Base;   // 位图地址
  u32 AscOff;  // 第一个可显示字符的开始索引
  u8  Width;   // 宽
  u8  Height;  // 高
};

typedef struct _FONT_INFO FontInfo_t;

extern FontInfo_t *Font;

int FontShow (
  u8 Code,
  FontInfo_t *Font,
  u64 X, u64 Y,
  u32 FGColor, u32 BGColor
);
