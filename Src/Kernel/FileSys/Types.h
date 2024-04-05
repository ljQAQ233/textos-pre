#pragma once

#include <TextOS/Dev.h>

#define FS_INITIALIZER(Name) \
        void *Name(Dev_t *Hd, Record_t *Mbs, Partition_t *Partition)

typedef struct _packed
{
    u8   Bootable        ; // Bootable (Active) -> 0x80
    u8   HeadStart       ;
    u16  SecStart     :6 ;
    u16  ClinderStart :10;
    u8   SysId           ;
    u8   HeadEnd         ;
    u16  SecEnd       :6 ;
    u16  ClinderEnd   :10;
    u32  Relative        ;
    u32  Total           ;
} Partition_t;

typedef struct _packed
{
    u8          Others[446];
    Partition_t Partitions[4];
    u16         EndSym;
} Record_t;
