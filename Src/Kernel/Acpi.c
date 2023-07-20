#define RSDP_SIG SIGN_64('R','S','D',' ','P','T','R',' ')

/* Root System Description Pointer */
typedef struct _packed {
    u64  Sign;
    u8   CheckSum;
    char OEMID[6];
    u8   Revision;
    u32  RsdtAddr;
    u32  Length;
    u64  XsdtAddr;
    u8   ExtCheckSum;
    u8   Reserved[3];
} Rsdp_t;

#define XSDT_SIG SIGN_32('X','S','D','T')
#define MADT_SIG SIGN_32('A','P','I','C')

typedef struct _packed {
    u32  Sign;
    u32  Length;
    u8   Revision;
    u8   CheckSum;
    char OEMID[6];
    char OEMTabID[8];
    u32  OEMRevision;
    u32  CreatorID;
    u32  CreatorRevision;
} Hdr_t;

/* eXtended System Description Table */
typedef struct _packed {
    Hdr_t Hdr;
} Xsdt_t;

#include <Boot.h>
#include <TextOS/Panic.h>
#include <TextOS/Console/PrintK.h>

#include <string.h>

Xsdt_t *__gXsdt;

static bool _CheckSum (void * Buffer, size_t Siz)
{
    u8 Sum = 0;
    for (size_t i = 0; i < Siz; i++) {
        Sum += ((char *)Buffer)[i];
    }

    return (Sum == 0);
}

static void *_EntryGet (u32 Sign)
{
    size_t Num = (__gXsdt->Hdr.Length - sizeof(Xsdt_t)) / 8;

    Hdr_t *Ptr;
    u64 *Addrs = (void *)__gXsdt + sizeof(Xsdt_t);

    for (size_t i = 0; i < Num ;i++) {
        Ptr = (Hdr_t *)(Addrs[i]);
        if (Ptr->Sign == Sign) {
            if (_CheckSum((void *)Ptr, Ptr->Length))
                return (void *)Ptr;
        }
    }
    return NULL;
}

void MadtParser ();
void FadtParser ();

void InitializeAcpi ()
{
    Rsdp_t *Rsdp = _BootConfig.AcpiTab;

    /* This includes only the first 20 bytes of this Rsdp,
       including the checksum field. These bytes must sum
       to zero. 
       NOTE : Overflow to zero. (char/unsigned char/u8)   */
    if (Rsdp->Sign != RSDP_SIG || !_CheckSum(Rsdp, 20))
        PANIC ("Bad RSDP!!!\n");

    Xsdt_t *Xsdt = (Xsdt_t *)Rsdp->XsdtAddr;
    if (Xsdt->Hdr.Sign != XSDT_SIG || !_CheckSum(Xsdt, Xsdt->Hdr.Length))
        PANIC ("Bad Xsdt!!!");
    __gXsdt = Xsdt;

    MadtParser();
    FadtParser();
}

#define APIC_SIG SIGN_32('A','P','I','C')

/* Multiple APIC Description Table */
typedef struct _packed {
    Hdr_t Hdr;
    u32   LApicAddr;
    u32   Flgs;
} Madt_t;

typedef struct _packed {
    u8 Type;
    u8 Length;
} MadtIcs_t;

typedef struct _packed {
    MadtIcs_t Hdr;
    u8        AcpiProcessorUID;
    u8        ApicID;
    u32       Flgs;
} LApic_t;

enum IcsType
{
    ICS_LAPIC  = 0,
    ICS_IOAPIC = 1,
    ICS_ISO    = 2,
};

typedef struct _packed {
    MadtIcs_t Hdr;
    u8        IOApicID;
    u8        Reserved;
    u32       IOApicAddr;
    u32       GsiBase;
} IOApic_t;

typedef struct _packed {
    MadtIcs_t Hdr;
    u8        Bus;
    u8        Src;
    u32       Gsi;
    u16       Flgs;
} Iso_t;

extern void *LApic;
extern void *IOApic;

static u8 _Gsi[24];

/* Src 到 Gsi 的过程类似于一个重定向的过程 */
u8 __GsiGet (u8 Src)
{
    return _Gsi[Src] != 0xFF ? _Gsi[Src] : Src;
}

void MadtParser ()
{
    Madt_t *Madt = _EntryGet (APIC_SIG);
    if (Madt == NULL)
        PANIC ("Can not find madt!!!");
    
    int64 Len = Madt->Hdr.Length - sizeof(Madt_t);
    MadtIcs_t *Ics = OFFSET (Madt, sizeof(Madt_t));

    memset (_Gsi, 0xFF, 24);

    while (Len > 0) {
        switch (Ics->Type)
        {
        case ICS_LAPIC:
            // 写 `{}` 是明确变量的 life circle
            {
                LApic_t *_LApic = (LApic_t *)Ics;
                PrintK ("Local Apic -> Acpi Processor UID : %u\n"
                        "              Apic ID : %u\n"
                        "              Flags :   %x\n",
                        _LApic->AcpiProcessorUID,_LApic->ApicID,_LApic->Flgs);
            }
        break;
        
        case ICS_IOAPIC:
            {
                IOApic_t *_IOApic = (IOApic_t *)Ics;
                PrintK ("I/O Apic -> I/O Apic ID : %u\n"
                        "            I/O Apic Addr : %#x\n"
                        "            Global System Interrupt Base : %#x\n",
                        _IOApic->IOApicID,_IOApic->IOApicAddr,_IOApic->GsiBase);
                IOApic = (void *)(u64)_IOApic->IOApicAddr;
            }
        break;

        /* Interrupt Source Override */
        case ICS_ISO:
            {
                Iso_t *_Iso = (Iso_t *)Ics;
                PrintK ("Intr Src Override -> Bus : %u, Src : %u, Flgs : %x\n"
                        "                     Global sys intr : %u\n",
                        _Iso->Bus,_Iso->Src,_Iso->Flgs,_Iso->Gsi);
                _Gsi[_Iso->Src] = _Iso->Gsi;
            }
        break;
        }
        Len -= Ics->Length;
        Ics = OFFSET (Ics, Ics->Length);
    }

    LApic = (void *)(u64)Madt->LApicAddr;
}

typedef struct {
    Hdr_t Hdr;
} Fadt_t;

#define FADT_SIG SIGN_32('F','A','C','P')

void FadtParser ()
{
    Fadt_t *Fadt = _EntryGet (FADT_SIG);
}
