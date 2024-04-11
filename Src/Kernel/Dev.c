#include <TextOS/Dev.h>
#include <TextOS/Dev/Private.h>
#include <TextOS/Panic.h>
#include <TextOS/Lib/List.h>
#include <TextOS/Memory/Malloc.h>

/*
  The flow to register:
    1. DevNew() to create new buffer
    2. Set it by device initializer itself
    3. DevRegister() to insert into the root list
*/

void __DevOptNone() { PANIC ("This opts is not supported!"); };

static DevPri_t DevRoot = {
    .List = LIST_INIT(DevRoot.List)
};

void DevInit ()
{
    Dev_t *Root = DevNew();
    
    Root->Name = "Dev Root";
    DevRoot.Dev = Root;
}

void __DevRegister (DevPri_t *Pri)
{
    ListInsertTail (&DevRoot.List, &Pri->List);
}

void DevRegister (Dev_t *Dev)
{
    DevPri_t *Pri = MallocK (sizeof (DevPri_t));
    Pri->Dev = Dev;

    __DevRegister (Pri);
}

Dev_t *DevNew ()
{
    Dev_t *Dev = MallocK (sizeof (Dev_t));
    
    Dev->Read = (void *)__DevOptNone;
    Dev->Write = (void *)__DevOptNone;
    Dev->BlkRead = (void *)__DevOptNone;
    Dev->BlkWrite = (void *)__DevOptNone;
    
    return Dev;
}

#define FOREACH_DEV() \
        for (List_t *i = DevRoot.List.Back ; i != &DevRoot.List ; i = i->Back)

#include <string.h>
#include <TextOS/Panic.h>

Dev_t *DevLookupByType (int Type, int SubType)
{
    FOREACH_DEV() {
        DevPri_t *Pri = CR (i, DevPri_t, List);
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
        DevPri_t *Pri = CR (i, DevPri_t, List);
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
        DevPri_t *Pri = CR (i, DevPri_t, List);
        PrintK ("Dev Index - %04d -> %s\n"  , Idx, Pri->Dev->Name);
        PrintK ("            Type -> %s\n"  , _DevTypeString(Pri->Dev->Type));
        PrintK ("            Opts -> %d%d\n",
                Pri->Dev->Read == NULL ? 0 : 1,
                Pri->Dev->Write == NULL ? 0 : 1);

        Idx++;
    }
}

