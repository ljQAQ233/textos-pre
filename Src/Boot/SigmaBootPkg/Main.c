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

    /* 1024 * 768 */
    /* 1024 * 600 */

    GraphicsResolutionSet (1024,684);
    GraphicsResolutionSet (1024,685);

    return EFI_SUCCESS;
}
