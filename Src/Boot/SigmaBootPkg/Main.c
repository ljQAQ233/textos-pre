#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Boot/Boot.h>
#include <Boot/File.h>
#include <Boot/Logo.h>
#include <Boot/Config.h>
#include <Boot/Graphics.h>
#include <Boot/Memory.h>
#include <Boot/Kernel.h>

typedef struct {
  UINT64 Hor;
  UINT64 Ver;
  UINT64 FrameBuffer;
  UINT64 FrameBufferSize;
} GRAPHICS_CONFIG;

typedef struct {
  UINT64          Magic;
  GRAPHICS_CONFIG Graphics;
} BOOT_CONFIG;

/* From tanyugang's Code,and I modified it,very thanks! */

EFI_STATUS ExitBootServices (
        IN     EFI_HANDLE ImageHandle,
           OUT MAP_INFO   *Info
    )
{
    EFI_STATUS Status = EFI_SUCCESS;

    Status = MemoryGetMap (Info);
    ERR_RETS(Status);

    Status = gBS->ExitBootServices (
            ImageHandle,
            Info->MapKey
        );
    ERR_RETS(Status);

    Info->MapCount = Info->MapSiz / Info->DescSiz;

    return Status;
}

EFI_STATUS EFIAPI UefiMain (
        IN EFI_HANDLE        ImageHandle,
        IN EFI_SYSTEM_TABLE  *SystemTable
        )
{
    Breakpoint();

    InitializeGraphicsServices();
    InitializeFileServices();

    InitializeConfig();

    BOOT_CONFIG *Config = AllocateZeroPool (sizeof (BOOT_CONFIG));

    CHAR16 *KernelPath = ConfigGetStringChar16 ("Kernel", D_KERNEL_PATH);

    KERNEL_PAGE *KernelPages;
    EFI_PHYSICAL_ADDRESS KernelEntry;

    KernelLoad (KernelPath, &KernelEntry, &KernelPages);

    UINT64 PML4Addr;
    InitializePageTab (KernelPages, &PML4Addr);
    UpdateCr3 (PML4Addr,0);

    MAP_INFO Map;
    ExitBootServices (ImageHandle, &Map);

    Config->Magic = SIGNATURE_64('T', 'E', 'X', 'T', 'O', 'S', 'B', 'T');
    Config->Graphics.FrameBuffer     = gGraphicsOutputProtocol->Mode->FrameBufferBase;
    Config->Graphics.FrameBufferSize = gGraphicsOutputProtocol->Mode->FrameBufferSize;
    Config->Graphics.Hor             = gGraphicsOutputProtocol->Mode->Info->HorizontalResolution;
    Config->Graphics.Ver             = gGraphicsOutputProtocol->Mode->Info->VerticalResolution;

    UINT64 Ret = ((UINT64 (*)(BOOT_CONFIG *))KernelEntry)(Config); // A ptr to entry and call it to get status it returned
    IGNORE (Ret);

    return EFI_SUCCESS;
}
