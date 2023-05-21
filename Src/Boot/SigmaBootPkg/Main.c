#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Boot/Boot.h>
#include <Boot/File.h>
#include <Boot/Logo.h>
#include <Boot/Config.h>
#include <Boot/Graphics.h>
#include <Boot/Kernel.h>

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
    KernelLoad (L"\\Kernel.bin", &KernelEntry);

    UINT32 Ret = ((UINT32 (*)(VOID))KernelEntry)(); // A ptr to entry and call it to get status it returned
    DEBUG ((DEBUG_INFO ,"[INFO] Kernel returned : %llu\n", Ret));

    return EFI_SUCCESS;
}
