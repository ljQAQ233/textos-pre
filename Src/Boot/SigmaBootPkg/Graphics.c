#include <Uefi.h>

#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Boot/Boot.h>
#include <Boot/Graphics.h>

EFI_GRAPHICS_OUTPUT_PROTOCOL *gGraphicsOutputProtocol = NULL;

EFI_STATUS InitializeGraphicsServices ()
{
    EFI_STATUS Status = EFI_SUCCESS;

    Status = gBS->LocateProtocol (
            &gEfiGraphicsOutputProtocolGuid,
            NULL,
            (VOID **)&gGraphicsOutputProtocol
        );
    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_ERROR ,"[FAIL] LocateHandle : gGrsphicsOutputProtocol - Status : %r\n",Status));
        return Status;
    }
    DEBUG ((DEBUG_INFO ,"[DONE] LocateHandle : gGrsphicsOutputProtocol\n"));

    return Status;
}

