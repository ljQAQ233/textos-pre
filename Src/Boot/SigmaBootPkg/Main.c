#include <Uefi.h>

#include <Library/UefiLib.h>

#include <Boot/Boot.h>
#include <Boot/File.h>
#include <Boot/Graphics.h>

EFI_STATUS EFIAPI UefiMain (
        IN EFI_HANDLE        ImageHandle,
        IN EFI_SYSTEM_TABLE  *SystemTable
        )
{
    Breakpoint();

    InitializeFileServices();
    InitializeGraphicsServices();

    return EFI_SUCCESS;
}
