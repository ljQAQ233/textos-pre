#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>

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

    GraphicsBmpDisplay (L"\\Sigma1.bmp",0,0);
    SystemTable->ConOut->ClearScreen (SystemTable->ConOut);
    GraphicsBmpDisplay (L"\\Sigma4.bmp",0,0);
    SystemTable->ConOut->ClearScreen (SystemTable->ConOut);
    GraphicsBmpDisplay (L"\\Sigma8.bmp",0,0);
    SystemTable->ConOut->ClearScreen (SystemTable->ConOut);
    GraphicsBmpDisplay (L"\\Sigma24.bmp",0,0);
    SystemTable->ConOut->ClearScreen (SystemTable->ConOut);

    return EFI_SUCCESS;
}
