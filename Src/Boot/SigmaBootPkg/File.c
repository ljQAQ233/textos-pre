#include <Uefi.h>
#include <Library/MemoryAllocationLib.h>
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
    DEBUG ((DEBUG_INFO ,"[DONE] LocateHandleBuffer - Count : %llu\n",HandleCount));

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
    DEBUG ((DEBUG_INFO ,"[DONE] Open FileProtocol\n"));
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
    DEBUG ((DEBUG_INFO ,"[DONE] Opened volume successfully\n"));

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
            Path,Mode &~ O_NAPEND,
            0
        );
    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_ERROR ,"[FAIL] Open File : %S (%llx) - Status : %r\n",Path,Mode,Status));
        return Status;
    }
    DEBUG ((DEBUG_INFO ,"[DONE] Open File : %S (%llx)\n",Path,Mode));

    if (Mode & O_NAPEND)
    {
        DEBUG ((DEBUG_INFO ,"[INFO] Re-create file : %s\n",Path));
        ERR_RETS(FileRemove(*File)); // This operation includes delete and close!!!

        Mode |= O_CREATE;
        Mode &= ~O_NAPEND;

        FileOpen (Path,Mode,File);
    }

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
    DEBUG ((DEBUG_INFO ,"[DONE] Read File - Siz : %llu\n",*Size));

    return Status;
}

EFI_STATUS FileAutoRead (
        IN     EFI_FILE_PROTOCOL *File,
           OUT VOID              **Data,
           OUT UINT64            *DataSize
        )
{
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_FILE_INFO *Info = NULL;

    Status = FileGetInfo (File,&Info);
    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_ERROR ,"[FAIL] Get file infomation failed - Status : %r\n",Status));
        return Status;
    }
    DEBUG ((DEBUG_INFO ,"[DONE] Get file infomation - %s\n",Info->FileName));

    /* Keep the `Info` unchanged */
    UINTN Size = Info->FileSize;
    *Data = AllocatePool (Size);
    if (Data == NULL)
    {
        DEBUG ((DEBUG_INFO, "[FAIL] Get memory space for the file is read\n"));
        return EFI_OUT_OF_RESOURCES;
    }
    
    Status = FileRead (File,*Data,&Size);
    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_ERROR ,"[FAIL] Read file automatically - Status : %r\n",Status));
        return Status;
    }
    DEBUG ((DEBUG_INFO ,"[DONE] Read file automatically\n"));
    
    if (DataSize)
    {
        *DataSize = Size;
    }

    FileDestroyInfo (&Info);

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
    DEBUG ((DEBUG_INFO ,"[DONE] Write File - Size : %llu\n",*Size));

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
    DEBUG ((DEBUG_INFO ,"[DONE] Flush file\n"));

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
    DEBUG ((DEBUG_INFO ,"[DONE] Set Position - Pos : %llu\n",Position));

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
    DEBUG ((DEBUG_INFO,"[DONE] Got position of this file - Pos : %llu\n",Position));

    return Position;
}

EFI_STATUS FileGetInfo (
        IN     EFI_FILE_PROTOCOL *File,
           OUT EFI_FILE_INFO     **Info
        )
{
    EFI_STATUS Status = EFI_SUCCESS;

    /* Make the buffer a NULL that it returns a Correct size of the file info so that we allocate memory for that */
    UINTN Size = 0;
    *Info = (EFI_FILE_INFO *)NULL;
    
    Status = File->GetInfo (File,&gEfiFileInfoGuid,&Size,(VOID *)*Info);
    if (Status != EFI_BUFFER_TOO_SMALL)
    {
        DEBUG ((DEBUG_ERROR ,"[FAIL] Get the size of the file info - Status : %r\n",Status));
        return Status;
    }

    *Info = (EFI_FILE_INFO *)AllocatePool (Size);
    if (*Info == NULL)
    {
        DEBUG ((DEBUG_ERROR ,"[FAIL] Allocate memory for info,function returned NULL\n"));
        return EFI_OUT_OF_RESOURCES;
    }

    Status = File->GetInfo (File,&gEfiFileInfoGuid,&Size,(VOID *)*Info);
    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_ERROR ,"[FAIL] Get the file info - Status : %r\n",Status));
        return Status;
    }

    DEBUG ((DEBUG_INFO ,"[DONE] Get file info : %llx\n",*Info));
    return Status;
}

EFI_STATUS FileSetInfo (
        IN EFI_FILE_PROTOCOL *File,
        IN UINTN             Size,
        IN EFI_FILE_INFO     *Info
        )
{
    EFI_STATUS Status = File->SetInfo (File,&gEfiFileInfoGuid,Size,Info);
    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_INFO ,"[FAIL] Set info for this file\n - Status : %r\n",Status));
        return Status;
    }

    DEBUG ((DEBUG_INFO ,"[DONE] Set info for this file - InfoSize : %llu\n",Size));
    return Status;
}

VOID FileDestroyInfo (EFI_FILE_INFO **Info)
{
    FreePool (*Info);
    *Info = NULL;
}

EFI_STATUS FileRemove (EFI_FILE_PROTOCOL *File)
{
    EFI_STATUS Status = File->Delete (File);
    
    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_ERROR ,"[FAIL] Remove File - Status : %r\n",Status));
        return Status;
    }
    DEBUG ((DEBUG_INFO ,"[DONE] Remove file\n"));

    return Status;
}

EFI_STATUS FileClose (EFI_FILE_PROTOCOL *File)
{
    EFI_STATUS Status = File->Close (File);

    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_ERROR ,"[FAIL] Close File - Status : %r\n",Status));
        return Status;
    }
    DEBUG ((DEBUG_INFO ,"[DONE] Close file\n"));

    return Status;
}

