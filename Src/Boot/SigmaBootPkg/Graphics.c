#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Boot/Boot.h>
#include <Boot/Graphics.h>

EFI_GRAPHICS_OUTPUT_PROTOCOL *gGraphicsOutputProtocol = NULL;

EFI_STATUS EFIAPI InitializeGraphicsServices ()
{
    EFI_STATUS Status = EFI_SUCCESS;

    Status = gBS->LocateProtocol (
            &gEfiGraphicsOutputProtocolGuid,
            NULL,
            (VOID **)&gGraphicsOutputProtocol
        );
    ERR_RETS(Status);

    return Status;
}
