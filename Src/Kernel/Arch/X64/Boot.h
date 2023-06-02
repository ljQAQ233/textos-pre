#ifndef __BOOT_H__
#define __BOOT_H__

typedef struct {
  UINT64 Hor;
  UINT64 Ver;
  UINT64 FrameBuffer;
  UINT64 FrameBufferSize;
} GRAPHICS_CONFIG;

typedef struct {
  UINT64          Magic;
  GRAPHICS_CONFIG Graphics;
} BOOT_CONFIG;

extern BOOT_CONFIG _BootConfig;

#endif
