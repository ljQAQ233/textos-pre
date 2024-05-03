#include <TextOS/Lib/Stack.h>
#include <TextOS/Memory/Malloc.h>

Stack_t *StackInit (Stack_t *Stack)
{
    if (!Stack) {
        Stack = MallocK (sizeof(Stack_t));
        Stack->Fixed = false;
    } else {
        Stack->Fixed = true;
    }

    Stack->Siz = 0;
    Stack->Top = NULL;

    Stack->Pop = NULL;
    Stack->Clearner = NULL;

    return Stack;
}

void StackSet (Stack_t *Stack, void *Cleaner, void *Pop)
{
    Stack->Pop = Pop;
    Stack->Clearner = Cleaner;
}

void StackFini (Stack_t *Stack)
{
    StackClear (Stack);
    if (!Stack->Fixed)
        FreeK (Stack);
}

void StackPush (Stack_t *Stack, void *Payload)
{
    Elem_t *Elem = MallocK (sizeof(Elem_t));
    Elem->Payload = Payload;
    Elem->Next = Stack->Top;

    // Replace the top
    Stack->Top = Elem;
    Stack->Siz++;
}

void *StackTop (Stack_t *Stack)
{
    if (StackEmpty (Stack))
        return NULL;

    return Stack->Top->Payload;
}

void StackPop (Stack_t *Stack)
{
    if (StackEmpty (Stack))
        return;

    Elem_t *Top = Stack->Top;
    Stack->Top = Top->Next;

    if (Stack->Pop)
        Stack->Pop (Top);
    Stack->Siz--;
}

void StackClear (Stack_t *Stack)
{
    while (!StackEmpty (Stack))
    {
        if (Stack->Clearner)
            Stack->Clearner (Stack->Top);
        StackPop (Stack);
    }

    Stack->Siz = 0;
}

bool StackEmpty (Stack_t *Stack)
{
    return !Stack->Top;
}

size_t StackSiz (Stack_t *Stack)
{
    return Stack->Siz;
}

//

StackIter_t *StackIter (Stack_t *Stack)
{
    return (StackIter_t *)Stack->Top;
}

void *StackIterDef (StackIter_t *Iter)
{
    Elem_t *Elem = (Elem_t *)Iter;
    return Elem->Payload;
}

StackIter_t *StackIterNext (StackIter_t **Iter)
{
    Elem_t *Elem = (Elem_t *)*Iter;
    *Iter = (StackIter_t *)Elem->Next;
    return *Iter;
}

