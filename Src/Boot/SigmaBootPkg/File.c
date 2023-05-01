#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Boot/Boot.h>
#include <Boot/File.h>

EFI_FILE_PROTOCOL *gFileProtocol = NULL;

EFI_STATUS InitializeFileServices ()
{
    EFI_STATUS Status = EFI_SUCCESS;

    UINTN      HandleCount = 0;
    EFI_HANDLE *HandleBuffer = NULL;

    Status = gBS->LocateHandleBuffer (
            ByProtocol,
            &gEfiSimpleFileSystemProtocolGuid,
            NULL,
            &HandleCount,&HandleBuffer
        );
    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_ERROR ,"[FAIL] LocateHandleBuffer : gEfiSimpleFileSystemProtocolGuid - Status : %r\n",Status));
        return Status;
    }
    DEBUG ((DEBUG_INFO ,"[ OK ] LocateHandleBuffer - Count : %llu\n",HandleCount));

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem = NULL;
    Status = gBS->OpenProtocol (
            HandleBuffer[0],
            &gEfiSimpleFileSystemProtocolGuid,
            (VOID**)&FileSystem,
            gImageHandle,
            NULL,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL
        );
    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_ERROR ,"[FAIL] OpenProtocol : File Protocol - Status : %r\n",Status));
        return Status;
    }
    DEBUG ((DEBUG_INFO ,"[ OK ] Open FileProtocol\n"));
    gBS->FreePool (HandleBuffer);

    Status = FileSystem->OpenVolume (
            FileSystem,
            &gFileProtocol
        );
    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_ERROR ,"[FAIL] Open volume - Status : %r\n",Status));
        return Status;
    }
    DEBUG ((DEBUG_INFO ,"[ OK ] Opened volume successfully\n"));

    return Status;
}

EFI_STATUS FileOpen (
        IN      CHAR16            *Path,
        IN      UINT64            Mode,
           OUT  EFI_FILE_PROTOCOL **File
    )
{
    EFI_STATUS Status = gFileProtocol->Open (
            gFileProtocol,
            File,
            Path,Mode,
            0
        );
    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_ERROR ,"[FAIL] Open File : %S - Status : %r\n",Path,Status));
        return Status;
    }
    DEBUG ((DEBUG_INFO ,"[ OK ] Open File : %S\n",Path));

    return Status;
}

EFI_STATUS FileRead (
        IN     EFI_FILE_PROTOCOL *File,
           OUT VOID              *Data,
        IN OUT UINTN             *Size
        )
{
    EFI_STATUS Status = File->Read (
            File,
            Size,
            Data
        );
    if (EFI_ERROR (Status))
    {
        DEBUG ((DEBUG_ERROR ,"[FAIL] Read File - Size later : %llu,Status : %r\n",*Size,Status));
        return Status;
    }
    DEBUG ((DEBUG_INFO ,"[ OK ] Read File - Siz : %llu\n",*Size));

    return Status;
}

EFI_STATUS FileWrite (
        IN     EFI_FILE_PROTOCOL *File,
        IN     VOID              *Buffer,
        IN OUT UINTN             *Size
        )
{
    EFI_STATUS Status = File->Write (
            File,
            Size,
            Buffer
        );
    if (EFI_ERROR (Status))
    {
        DEBUG ((DEBUG_ERROR ,"[FAIL] Write File - Status : %r\n",Status));
        return Status;
    }
    DEBUG ((DEBUG_INFO ,"[ OK ] Write File - Size : %llu\n",*Size));

    return Status;
}

EFI_STATUS FileFlush (EFI_FILE_PROTOCOL *File)
{
    EFI_STATUS Status = File->Flush (File);

    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_ERROR ,"[FAIL] Flush File - Status : %r\n",Status));
        return Status;
    }
    DEBUG ((DEBUG_INFO ,"[ OK ] Flush file\n"));

    return Status;
}

EFI_STATUS FileSetPosition (
        IN EFI_FILE_PROTOCOL    *File,
        IN UINT64               Position
    )
{
    EFI_STATUS Status = File->SetPosition (File,Position);
    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_ERROR ,"[FAIL] Set Position - Pos : %llu - Status : %r\n",Position,Status));
        return Status;
    }
    DEBUG ((DEBUG_INFO ,"[ OK ] Set Position - Pos : %llu\n",Position));

    return Status;
}

UINT64 FileGetPosition (
        IN EFI_FILE_PROTOCOL *File
        )
{
    UINT64 Position = 0;

    EFI_STATUS Status = File->GetPosition (File,&Position);
    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_INFO, "[FAIL] Failed to get position of the ptr of this file - Status : %r\n",Status));
        return 0;
    }
    DEBUG ((DEBUG_INFO,"[ OK ] Got position of this file - Pos : %llu\n",Position));

    return Position;
}

