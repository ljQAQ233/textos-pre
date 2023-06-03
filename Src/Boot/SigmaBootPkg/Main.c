#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Boot/Boot.h>
#include <Boot/Logo.h>
#include <Boot/File.h>
#include <Boot/Config.h>
#include <Boot/Graphics.h>
#include <Boot/Memory.h>
#include <Boot/Kernel.h>
#include <Boot/Args.h>

typedef struct {
  UINT64 Hor;
  UINT64 Ver;
  UINT64 FrameBuffer;
  UINT64 FrameBufferSize;
  VOID   *Font;
} GRAPHICS_CONFIG;

typedef struct {
  GRAPHICS_CONFIG Graphics;
  ARGS_STACK      Args;
} BOOT_CONFIG;

/* From tanyugang's Code,and I modified it,very thanks! */

EFI_STATUS ExitBootServices (
        IN     EFI_HANDLE ImageHandle,
           OUT MAP_INFO   *Info
    )
{
    EFI_STATUS Status = EFI_SUCCESS;

    Status = MemoryGetMap (Info);
    if (EFI_ERROR (Status))
    {
        DEBUG ((DEBUG_ERROR ,"[FAIL] Failed to get memory map for exiting boot services - Status : %r\n",Status));
        return Status;
    }

    Status = gBS->ExitBootServices (
            ImageHandle,
            Info->MapKey
        );
    if (EFI_ERROR (Status))
    {
        DEBUG ((DEBUG_ERROR ,"[FAIL] Failed to exit boot services - Status : %r\n",Status));
        return Status;
    }
    Info->MapCount = Info->MapSiz / Info->DescSiz;

    return Status;
}

EFI_STATUS EFIAPI UefiMain (
        IN EFI_HANDLE        ImageHandle,
        IN EFI_SYSTEM_TABLE  *SystemTable
        )
{
    Breakpoint();

    InitializeFileServices();
    InitializeGraphicsServices();

    InitializeConfig();

    UINT64 ArgSize = 0;

    BOOT_CONFIG *Config = AllocateZeroPool (sizeof (BOOT_CONFIG));

    CHAR16 *KernelPath = ConfigGetStringChar16 ("Kernel",D_KERNEL_PATH);

    KERNEL_PAGE *KernelPages;
    EFI_PHYSICAL_ADDRESS KernelEntry;
    KernelLoad (KernelPath,&KernelEntry,&KernelPages);

    CHAR16 *FontPath = ConfigGetStringChar16 ("Font",D_FONT_PATH);
    FONT_CONFIG *Font = AllocateZeroPool (sizeof (FONT_CONFIG));
    FontLoad (FontPath,Font);
    
    ArgSize += sizeof(FONT_CONFIG) + Font->Size;
    InitializeArgs(&Config->Args, ArgSize);
    ArgsPush (&Config->Args, (VOID **)&Font, sizeof(FONT_CONFIG));
    ArgsPush (&Config->Args, (VOID **)&Font->Base, Font->Size);

    UINT64 PML4Addr;
    InitializePageTab (KernelPages,&PML4Addr);
    UpdateCr3 (PML4Addr,0);

    MAP_INFO Map;
    ExitBootServices (ImageHandle,&Map);

    // TODO: More args
    Config->Graphics.FrameBuffer = gGraphicsOutputProtocol->Mode->FrameBufferBase;
    Config->Graphics.FrameBufferSize = gGraphicsOutputProtocol->Mode->FrameBufferSize;
    Config->Graphics.Hor = gGraphicsOutputProtocol->Mode->Info->HorizontalResolution;
    Config->Graphics.Ver = gGraphicsOutputProtocol->Mode->Info->VerticalResolution;

    Config->Graphics.Font = Font;

    ((VOID (*)(BOOT_CONFIG *))KernelEntry)(Config);

    return EFI_SUCCESS;
}
