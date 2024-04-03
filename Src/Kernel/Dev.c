#include <TextOS/Dev.h>
#include <TextOS/Panic.h>
#include <TextOS/Lib/List.h>
#include <TextOS/Memory/Malloc.h>

/*
  The flow to register:
    1. DevNew() to create new buffer
    2. Set it by device initializer itself
    3. DevRegister() to insert into the root list
*/

typedef struct {
    Dev_t  *Dev;
    List_t List;
} Private_t;

static void _DevOptNone() { PANIC ("This opts is not supported!"); };

#include <TextOS/Dev/Serial.h>

Private_t __DevSerial = {
    .Dev = &(Dev_t) {
        .Name = "Serial Port",
        .Read  = (void *)SerialRead,
        .Write = (void *)SerialWrite,
        .Type  = DEV_CHAR,
        .SubType = DEV_SERIAL,
        .BlkRead = (void *)_DevOptNone,
        .BlkWrite = (void *)_DevOptNone,
    },
};

#include <TextOS/Console/Read.h>
#include <TextOS/Console/Write.h>

Private_t __DevConsole = {
    .Dev = &(Dev_t) {
        .Name = "Console (kernel builtin)",
        .Read  = (void *)ConsoleRead,
        .Write = (void *)ConsoleWrite,
        .Type  = DEV_CHAR,
        .SubType = DEV_KNCON,
        .BlkRead = (void *)_DevOptNone,
        .BlkWrite = (void *)_DevOptNone,
    },
};

static Private_t DevRoot;

void DevInit ()
{
    Dev_t *Root = DevNew();
    
    Root->Name = "Dev Root";
    DevRoot.Dev = Root;

    ListInit (&DevRoot.List);
    ListInsert (&DevRoot.List, &__DevConsole.List);
    ListInsert (&DevRoot.List, &__DevSerial.List);
}

void DevRegister (Dev_t *Dev)
{
    Private_t *Pri = MallocK (sizeof (Private_t));
    Pri->Dev = Dev;

    ListInsertTail (&DevRoot.List, &Pri->List);
}

Dev_t *DevNew ()
{
    Dev_t *Dev = MallocK (sizeof (Dev_t));
    
    Dev->Read = (void *)_DevOptNone;
    Dev->Write = (void *)_DevOptNone;
    Dev->BlkRead = (void *)_DevOptNone;
    Dev->BlkWrite = (void *)_DevOptNone;
    
    return Dev;
}

#define FOREACH_DEV() \
        for (List_t *i = DevRoot.List.Back ; i != &DevRoot.List ; i = i->Back)

#include <string.h>
#include <TextOS/Panic.h>

Dev_t *DevLookupByType (int Type, int SubType)
{
    FOREACH_DEV() {
        Private_t *Pri = CR (i, Private_t, List);
        if (Pri->Dev->Type != Type)
            continue;
        if (Pri->Dev->SubType == SubType)
            return Pri->Dev;
    }

    return NULL;
}

Dev_t *DevLookupByName (const char *Name)
{
    FOREACH_DEV() {
        Private_t *Pri = CR (i, Private_t, List);
        if (strcmp (Pri->Dev->Name, Name) == 0)
            return Pri->Dev;
    }

    return NULL;
}

Dev_t *DevLookupByID (int Ident)
{
    PANIC ("Look up by LKUP_ID has not been implented yet\n");
}

#include <TextOS/Console/PrintK.h>

static char *_DevTypeString (int Type)
{
    if (Type == DEV_CHAR)
        return "Character device";
    if (Type == DEV_BLK)
        return "Block device";

    return "Unknown device";
}

void DevList ()
{
    int Idx = 0;

    FOREACH_DEV() {
        Private_t *Pri = CR (i, Private_t, List);
        PrintK ("Dev Index - %04d -> %s\n"  , Idx, Pri->Dev->Name);
        PrintK ("            Type -> %s\n"  , _DevTypeString(Pri->Dev->Type));
        PrintK ("            Opts -> %d%d\n",
                Pri->Dev->Read == NULL ? 0 : 1,
                Pri->Dev->Write == NULL ? 0 : 1);

        Idx++;
    }
}

