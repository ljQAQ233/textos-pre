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

    EFI_PHYSICAL_ADDRESS KernelEntry;
    KernelLoad (L"\\Kernel.elf", &KernelEntry);

    MAP_INFO Map;
    ExitBootServices (ImageHandle, &Map);

    UINT64 Ret = ((UINT64 (*)(VOID))KernelEntry)(); // A ptr to entry and call it to get status it returned
    IGNORE (Ret);

    return EFI_SUCCESS;
}
