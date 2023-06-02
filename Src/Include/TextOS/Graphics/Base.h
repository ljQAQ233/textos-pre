#pragma once // 写头太麻烦了,我选择偷懒...

union _COLOR {
  struct {
    u8 Blue;
    u8 Green;
    u8 Red;
    u8 Reserved;
  };
  u32 Raw;
};

typedef union _COLOR Color_t;

extern u32 Hor;
extern u32 Ver;

extern u32 *FrameBuffer;
extern u64 FrameBufferSize;

void PixelPut (
  u32 X,u32 Y,
  u32 Color
  );

void BlockPut (
  u32 X,u32 Y,
  u32 eX,u32 eY,
  u32 Color
  );

void ScreenClear ();

