#pragma once

#include <TextOS/Lib/List.h>

typedef struct {
    Dev_t  *Dev;
    List_t List;
} DevPri_t;

void __DevRegister (DevPri_t *Pri);

/* The null handler */
void __DevOptNone ();

