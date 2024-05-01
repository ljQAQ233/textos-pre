#include <IO.h>
#include <Irq.h>
#include <Intr.h>
#include <TextOS/Debug.h>

#include <TextOS/Dev.h>
#include <TextOS/Dev/Ide.h>
#include <TextOS/Memory/Malloc.h>

#include <string.h>

/*
   4 disks ->
      - Primary master
      - Primary slave
      - Primary master
      - Primary slave
*/

#define IDE_P_BASE 0x1F0 // Prime
#define IDE_S_BASE 0x170 // Secondary

#define R_DATA (0) // 
#define R_ERRF (1) // Error or features
#define R_SECC (2) // Sector count
#define R_LBAL (3) // LBA low
#define R_LBAM (4) // LBA mid
#define R_LBAH (5) // LBA high
#define R_DEV  (6) // Device
#define R_SCMD (7) // Status or cmd

#define R_BCTL 0x206 // Offset of the block register

typedef struct {
    char  SerialNum[21];
    char  ModelNum[41];

    u16   Port;
    u8    Dev;
} Private_t;

#define STAT_ERR   (1 << 0) // Error
#define STAT_IDX   (1 << 2) // Index
#define STAT_CD    (1 << 3) // Corrected data
#define STAT_RQRDY (1 << 4) // Data request ready
#define STAT_WF    (1 << 5) // Drive write fault
#define STAT_RDY   (1 << 6) // Drive ready
#define STAT_BSY   (1 << 7) // Drive Busy

//

#define DEV_PRI (0 << 4)
#define DEV_SEC (1 << 4)

#define DEV_CHS (0 << 6)
#define DEV_LBA (1 << 6)

/* Macro to set the device register, and
   the last binary to set bits which are always `1` */
#define SET_DEV(Opt) ((u8)(Opt | DEV_LBA | 0b10100000))

#define CMD_IDENT 0xE0
#define CMD_READ  0x20
#define CMD_WRITE 0x30

#include <TextOS/Task.h>

/* TODO: Lock device */

static int Curr = -1;

__INTR_FUNC(IdeHandler)
{
    LApic_SendEOI ();

    if (Curr < 0)
        return;

    DEBUGK ("Read opt has finished, waking proc... -> %d\n", Curr);

    TaskUnblock (Curr);
}

static void _ReadSector (u16 Port, u16 *Data)
{
    __asm__ volatile (
        "cld\n"
        "rep insw\n" 
        : : "c"(256), "d"(Port + R_DATA), "D"(Data)
        : "memory"
        );
}

static void _WriteSector (u16 Port, u16 *Data)
{
    __asm__ volatile (
        "cld\n"
        "rep outsw\n" 
        : : "c"(256), "d"(Port + R_DATA), "D"(Data)
        : "memory"
        );
}

#include <Irq.h>

void IdeRead (Dev_t *Dev, u32 Lba, void *Data, u8 Cnt)
{
    UNINTR_AREA_START();

    /*
       我们先 使用 28 位 PIO 模式练练手.
    */
    Lba &= 0xFFFFFFF;
    
    Private_t *Pri = Dev->Private;

    while (InB(R_SCMD + Pri->Port) & STAT_BSY) ;

    OutB(Pri->Port + R_DEV,  SET_DEV(Pri->Dev) | DEV_LBA | (Lba >> 24)); // 选择设备并发送 LBA 的高4位
    OutB(Pri->Port + R_SECC, Cnt);                                       // 扇区数目
    OutB(Pri->Port + R_LBAL, Lba & 0xFF);                                // LBA 0_7
    OutB(Pri->Port + R_LBAM, (Lba >> 8) & 0xFF);                         // LBA 8_15
    OutB(Pri->Port + R_LBAH, (Lba >> 16) & 0xFF);                        // LBA 16_23
    OutB(Pri->Port + R_SCMD, CMD_READ);                                  // 读取指令

    Curr = TaskCurr()->Pid;
    TaskBlock();

    for (int i = 0 ; i < Cnt ; i++) {
        _ReadSector (Pri->Port, Data);
        Data += SECT_SIZ;
    }
    
    UNINTR_AREA_END();
}

void IdeWrite (Dev_t *Dev, u32 Lba, void *Data, u8 Cnt)
{
    UNINTR_AREA_START();

    Lba &= 0xFFFFFFF;
    
    Private_t *Pri = Dev->Private;

    while (InB(R_SCMD + Pri->Port) & STAT_BSY) ;

    OutB(Pri->Port + R_DEV,  SET_DEV(Pri->Dev) | DEV_LBA | (Lba >> 24)); // 选择设备并发送 LBA 的高4位
    OutB(Pri->Port + R_SECC, Cnt);                                       // 扇区数目
    OutB(Pri->Port + R_LBAL, Lba & 0xFF);                                // LBA 0_7
    OutB(Pri->Port + R_LBAM, (Lba >> 8) & 0xFF);                         // LBA 8_15
    OutB(Pri->Port + R_LBAH, (Lba >> 16) & 0xFF);                        // LBA 16_23
    OutB(Pri->Port + R_SCMD, CMD_WRITE);                                 // 写入指令

    for (int i = 0 ; i < Cnt ; i++) {
        _WriteSector (Pri->Port, Data);
        Data += SECT_SIZ;

        Curr = TaskCurr()->Pid;
        TaskBlock();
    }
    
    UNINTR_AREA_END();
}

// void IdeRead (u32 Lba, void *Data, u8 Cnt)
// {
// }

/* Details in `/usr/include/linux/hdreg.h` on your pc [doge] */

#define ID_CONFIG   0   // Bit flags                  1  (word)
#define ID_SN       10  // Serial number              10 (word)
#define ID_FWVER    23  // Firmware version           8  (word)
#define ID_MODEL    27  // Model number               20 (word)
#define ID_SUPPORT  49  // Capabality                 1  (byte)
#define ID_ADDR_28  60  // 28位可寻址的全部逻辑扇区数 2  (word)
#define ID_ADDR_48  100 // 28位可寻址的全部逻辑扇区数 3  (word)
#define ID_MSN      176 // Current media sn           60 (word)

#define TRY(Count, Opts)                            \
    for ( int __try_count__ = Count ;               \
              __try_count__ > 0 || Count == 0 ;     \
              __try_count__-- )                     \
        Opts

static inline void _IdeWait ()
{
    while (InB(R_SCMD) & STAT_BSY) ;
}

#include <TextOS/Debug.h>

static bool _IdeIdentify (Private_t **Pri, int Idx)
{
    Private_t Info;
    memset (&Info, 0, sizeof(Private_t));

    u16 Buffer[256];

    u8  Dev  = Idx < 2 ? DEV_PRI : DEV_SEC;
    u16 Port = Idx & 1 ? IDE_S_BASE : IDE_P_BASE;

    OutB(Port + R_DEV , SET_DEV(Dev));
    OutB(Port + R_SCMD, 0xEC);

    for (int i = 0 ; i < 0xFFFF ; i++) ;

    u8 Stat = InB(Port + R_SCMD);
    if (Stat == 0 || Stat & STAT_ERR) // 没有这个设备, 或者错误
        return false;                 // 中断识别

    _ReadSector (Port, Buffer);
    
    for (int i = 0 ; i < 10 ; i++) {
        Info.SerialNum[i*2  ] = Buffer[ID_SN + i] >> 8;
        Info.SerialNum[i*2+1] = Buffer[ID_SN + i] &  0xFF;
    }

    for (int i = 0 ; i < 20 ; i++) {
        Info.ModelNum[i*2  ] = Buffer[ID_MODEL + i] >> 8;
        Info.ModelNum[i*2+1] = Buffer[ID_MODEL + i] &  0xFF;
    }

    Info.Dev = Dev;
    Info.Port = Port;

    DEBUGK ("Disk sn : %s\n", Info.SerialNum);
    DEBUGK ("Disk model : %s\n", Info.ModelNum);
    DEBUGK ("Disk port base : %#x\n", Info.Port);
    DEBUGK ("Disk dev : %d\n", Info.Dev);

    *Pri = MallocK (sizeof(Private_t));
    memcpy (*Pri, &Info, sizeof(Private_t));

    return true;
}

/* TODO : Detect all devices */
void IdeInit ()
{
    OutB(0x3F6, 0);

    Dev_t *Dev;
    Private_t *Pri;

    int i = 0;
    for ( ; i < 2 ; i++) {
        if (_IdeIdentify(&Pri, i)) {
            Dev = DevNew();
            Dev->Name = Pri->ModelNum;
            Dev->Type = DEV_BLK;
            Dev->SubType = DEV_IDE;
            Dev->BlkRead = (void *)IdeRead; // 丝毫不费脑筋的,降低代码安全性的强制类型转换...
            Dev->BlkWrite = (void *)IdeWrite;
            Dev->Private = Pri;
            DevRegister (Dev);
        }
    }
    
    IntrRegister (INT_MDISK, IdeHandler);
    IOApicRteSet (IRQ_MDISK, _IOAPIC_RTE(INT_MDISK));
    
    for ( ; i < 4 ; i++) {
        if (_IdeIdentify(&Pri, i)) {
            Dev = DevNew();
            Dev->Name = Pri->ModelNum;
            Dev->Type = DEV_BLK;
            Dev->SubType = DEV_IDE;
            Dev->BlkRead = (void *)IdeRead; // 丝毫不费脑筋的,降低代码安全性的强制类型转换...
            Dev->BlkWrite = (void *)IdeWrite;
            DevRegister (Dev);
        }
    }

    IntrRegister (INT_SDISK, IdeHandler);
    IOApicRteSet (IRQ_SDISK, _IOAPIC_RTE(INT_SDISK));
}

