#ifndef __TEXTOS_H__
#define __TEXTOS_H__

#define __TEXT_OS__

#include <TextOS/Type.h>
#include <TextOS/Base.h>

#define KERN_PHY (0x100000) // 内核物理地址.
#define KERN_ATC (0x100000) // 内核附属空间大小.

#define KERN_BCFG  0xFFFF800000000000ULL

#define KERN_HEAPV 0xFFFF8A0000000000ULL
#define KERN_PV    0xFFFFF00000000000ULL

#define KERN_FB    0xFFFF8B0000000000ULL

#endif
