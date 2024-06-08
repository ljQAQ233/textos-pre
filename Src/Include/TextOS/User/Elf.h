#ifndef __ELF_H__
#define __ELF_H__

#define ELF_MAGIC 0x464C457F

/* Elf Classes
 * Like Cpu Bits or Word Length
 */
#define ELFCLASSNONE   0
#define ELFCLASS32     1
#define ELFCLASS64     2

/* Elf Data 
 * Data Encodeing Type
 */
#define ELFDATANONE    0
#define ELFDATA_LSB    1
#define ELFDATA_MSB    2

/*
 * Elf Version 
 * The Current Version is 1
 */
#define EV_NONE        0
#define EV_CURRENT     1

/* Elf for which OS - OSABI 
 * We mustn't use it now,so only define the valid Macro 'ELFOSABI_LINUX'
 */
#define ELFOSABI_NONE  0
#define ELFOSABI_LINUX 3

/* Elf Pad Area 
 * Fill the Pad Area
 */
#define ELFRESERVED    0

/* Elf Machines
 * The Bootloader Supports the following types
 */
#define EM_386         3
#define EM_X86_64      62
#define EM_ARM         40
#define EM_AARCH64     183

/* Elf File Type 
 * There are more types But we use only the following
 */
#define ET_NONE        0 // Invalid Type
#define ET_REL         1 // Relocatable File
#define ET_EXEC        2 // Executable File
#define ET_DYN         3 // Shared Obj File

/* Program Header Flags 
 * Segment flags - ELF_PHEADER.Flgs
 */
#define PF_X        (1 << 0)    // Segment is executable 
#define PF_W        (1 << 1)    // Segment is writable
#define PF_R        (1 << 2)    // Segment is readable

#pragma pack(1)

typedef struct {
    u32 Magic;
    u8  Class;
    u8  Data;
    u8  Version;
    u8  OSABI;
    u8  ABIVersion;
    u8  Pad[6];
    u8  Nident;
    u16 Type;
    u16 Machine;
    u32 ObjFileVersion;
    u64 Entry;
    u64 PhOffset;
    u64 ShOffset;
    u32 Flgs;
    u16 EhSiz;
    u16 PhentSiz;
    u16 PhNum;
    u16 ShentSiz;
    u16 ShNum;
    u16 ShStrIdx;
} Hdr64_t;

#pragma pack()

#define PT_NONE 0
#define PT_LOAD 1

#pragma pack(1)

typedef struct {
  u32 Type;          // Type of the segment,e.g. PT_LOAD
  u32 Flgs;          // Segment flags
  u64 Offset;        // The segment's offset in the file
  u64 VirtualAddr;
  u64 PhysicalAddr;
  u64 FileSiz;       // The size of the segment in the file
  u64 MemSiz;        // The size of the segment that used in memory space
  u64 Align;
} ProgHdr64_t;

#pragma pack()

/* For x86_64 */
#define ELF_SUPPORTED_ARCH  EM_X86_64
#define ELF_SUPPORTED_CLASS ELFCLASS64

#include <TextOS/FileSys.h>

typedef struct {
    u64    Entry;
    Node_t *Node;
} Exec_t;

int ElfLoad (char *Path, Exec_t *Info);

#endif

