#include <Uefi.h>

#include <Library/UefiLib.h>

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

    /* 1024 * 768 */
    /* 1024 * 600 */

    GraphicsResolutionSet (1024,684);
    GraphicsResolutionSet (1024,685);

    return EFI_SUCCESS;
}
