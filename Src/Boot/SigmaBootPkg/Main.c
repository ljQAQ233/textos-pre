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

    CHAR16 *KernelPath = ConfigGetStringChar16 ("Kernel",D_KERNEL_PATH);

    EFI_PHYSICAL_ADDRESS KernelEntry;
    KernelLoad (KernelPath,&KernelEntry);

    MAP_INFO Map;
    ExitBootServices (ImageHandle,&Map);

    UINT64 Ret = ((UINT64 (*)(void))KernelEntry)(); // A ptr to entry and call it to get status it returned
    IGNORE (Ret);

    return EFI_SUCCESS;
}
