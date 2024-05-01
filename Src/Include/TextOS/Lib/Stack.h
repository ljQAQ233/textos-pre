#pragma once

typedef void (__StackPop_t)(void *Payload);
typedef void (__StackCleaner_t)(void *Payload);

struct _Elem
{
    void         *Payload;
    struct _Elem *Next;
};
typedef struct _Elem Elem_t;

typedef struct
{
    size_t           Siz;
    Elem_t           *Top;
    bool             Fixed;
    __StackPop_t     *Pop;
    __StackCleaner_t *Clearner;
} Stack_t;

Stack_t *StackInit (Stack_t *Stack);

void StackSet (Stack_t *Stack, void *Cleaner, void *Pop);

void StackFini (Stack_t *Stack);

void StackPush (Stack_t *Stack, void *Payload);

void *StackTop (Stack_t *Stack);

void StackPop (Stack_t *Stack);

void StackClear (Stack_t *Stack);

bool StackEmpty (Stack_t *Stack);

/* return the number of elements in this `Stack` */
size_t StackSiz (Stack_t *Stack);

