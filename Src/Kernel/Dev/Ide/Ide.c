#include <IO.h>
#include <Irq.h>
#include <Intr.h>
#include <TextOS/Debug.h>

#include <TextOS/Dev.h>
#include <TextOS/Dev/Ide.h>

#include <string.h>

#define IDE_P_BASE 0x1F0 // Prime
#define IDE_S_BASE 0x170 // Secondary

static u16 Port;

#define R_DATA (Port + 0) // 
#define R_ERRF (Port + 1) // Error or features
#define R_SECC (Port + 2) // Sector count
#define R_LBAL (Port + 3) // LBA low
#define R_LBAM (Port + 4) // LBA mid
#define R_LBAH (Port + 5) // LBA high
#define R_DEV  (Port + 6) // Device
#define R_SCMD (Port + 7) // Status or cmd

#define R_BCTL 0x206 // Offset of the block register

typedef struct {
    char  SerialNum[21];
    char  ModelNum[41];
    u64   Addr48;
    Dev_t Dev;
} Info_t;

static Info_t Info;

#define STAT_DF  (1 << 5)
#define STAT_BSY (1 << 7)

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

static void _ReadSector (u16 *Data)
{
    __asm__ volatile (
        "cld\n"
        "rep insw\n" 
        : : "c"(256), "d"(R_DATA), "D"(Data)
        : "memory"
        );
}

#include <Irq.h>

void IdeRead (u32 Lba, void *Data, u8 Cnt)
{
    UNINTR_AREA_START();

    /*
       我们先 使用 28 位 PIO 模式练练手.
    */
    Lba &= 0xFFFFFFF;

    while (InB(R_SCMD) & STAT_BSY) ;

    OutB(R_DEV,  DEV_PRI | DEV_LBA | (Lba >> 24)); // 选择设备并发送 LBA 的高4位
    OutB(R_SECC, Cnt);                             // 扇区数目
    OutB(R_LBAL, Lba & 0xFF);                      // LBA 0_7
    OutB(R_LBAM, (Lba >> 8) & 0xFF);               // LBA 8_15
    OutB(R_LBAH, (Lba >> 16) & 0xFF);              // LBA 16_23
    OutB(R_SCMD, CMD_READ);                        // 读取指令

    Curr = TaskCurr()->Pid;
    TaskBlock();

    for (int i = 0 ; i < Cnt ; i++) {
        _ReadSector (Data);
        Data += SECT_SIZ;
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

static void _IdeIdentify ()
{
    Info_t Info;
    memset (&Info, 0, sizeof(Info_t));

    u16 Buffer[256];

    OutB(R_DEV , SET_DEV(DEV_PRI));
    OutB(R_SCMD, 0xEC);

    for (int i = 0 ; i < 0xFFFF ; i++) ;

    _ReadSector (Buffer);
    
    for (int i = 0 ; i < 10 ; i++) {
        Info.SerialNum[i*2  ] = Buffer[ID_SN + i] >> 8;
        Info.SerialNum[i*2+1] = Buffer[ID_SN + i] &  0xFF;
    }

    for (int i = 0 ; i < 20 ; i++) {
        Info.ModelNum[i*2  ] = Buffer[ID_MODEL + i] >> 8;
        Info.ModelNum[i*2+1] = Buffer[ID_MODEL + i] &  0xFF;
    }

    DEBUGK ("Disk sn : %s\n", Info.SerialNum);
    DEBUGK ("Disk model : %s\n", Info.ModelNum);
}

/* TODO : Detect all devices */
void IdeInit ()
{
    Port = IDE_P_BASE;

    _IdeIdentify();

    OutB(0x3F6, 0);

    Dev_t *Dev = DevNew();
    Dev->Name = "ATA Device";
    Dev->Type = DEV_BLK;
    Dev->SubType = DEV_IDE;
    Dev->BlkRead = (void *)IdeRead; // 丝毫不费脑筋的,降低代码安全性的强制类型转换...
    DevRegister (Dev);

    IntrRegister (INT_MDISK, IdeHandler);
    IOApicRteSet (IRQ_MDISK, _IOAPIC_RTE(INT_MDISK));
}

