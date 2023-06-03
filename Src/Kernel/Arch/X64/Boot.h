#ifndef __BOOT_H__
#define __BOOT_H__

typedef struct {
    VOID   *Data;
    UINT64 PgNum;
    UINT8  *Ptr;
} ARGS_STACK;

typedef struct {
  UINT64 Hor;
  UINT64 Ver;
  UINT64 FrameBuffer;
  UINT64 FrameBufferSize;
  void   *Font;
} GRAPHICS_CONFIG;

typedef struct {
  GRAPHICS_CONFIG Graphics;
  ARGS_STACK      Args;
} BOOT_CONFIG;

extern BOOT_CONFIG _BootConfig;

#endif
