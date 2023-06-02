#ifndef __VIDEO_H__
#define __VIDEO_H__

union color {
  struct {
    u8 b, g, r;
    u8 rev;
  };
  u32 raw;
};

#define A_COLOR(color) \
        ((u32)(color & 0xff))

#define RGB_COLOR(r, g, b) \
        ((A_COLOR(b)) | (A_COLOR(g) << 8) | (A_COLOR(r) << 16) | A_COLOR(0) << 24)

typedef union color color_t;

void pixel_put (u32 x, u32 y, u32 color);

void block_put (u32 x, u32 y, u32 xe, u32 ye, u32 color);

void screen_clear ();

#endif

