#ifndef __BOOT_H__
#define __BOOT_H__

typedef struct {
  UINT64 Hor;
  UINT64 Ver;
  UINT64 FrameBuffer;
  UINT64 FrameBufferSize;
} GRAPHICS_CONFIG;

typedef struct {
  VOID   *Map;
} MEMORY_CONFIG;

typedef struct {
  UINT64          Magic;
  GRAPHICS_CONFIG Graphics;
  MEMORY_CONFIG   Memory;
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
