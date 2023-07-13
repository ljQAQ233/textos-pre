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
  VOID   *Map;
} MEMORY_CONFIG;

typedef struct {
  GRAPHICS_CONFIG Graphics;
  MEMORY_CONFIG   Memory;
  ARGS_STACK      Args;
} BOOT_CONFIG;

typedef struct {
  VOID    *Maps;
  UINTN   MapSiz;
  UINTN   MapCount;
  UINTN   MapKey;
  UINTN   DescSiz;
  UINT32  DescVersion;
} MAP_INFO;

extern BOOT_CONFIG _BootConfig;

#endif
