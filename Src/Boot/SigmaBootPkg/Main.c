#include <Uefi.h>

#include <Boot/Boot.h>
#include <Boot/Graphics.h>
#include <Boot/File.h>

EFI_STATUS EFIAPI UefiMain (
        IN EFI_HANDLE        ImageHandle,
        IN EFI_SYSTEM_TABLE  *SystemTable
        )
{
    Breakpoint();

    InitializeGraphicsServices();
    InitializeFileServices();

    return EFI_SUCCESS;
}
