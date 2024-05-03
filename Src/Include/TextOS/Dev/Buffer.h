#pragma once

#include <TextOS/Dev.h>
#include <TextOS/Lib/RbTree.h>

typedef struct
{
    Dev_t    Dev;
    RbTree_t Record;
} BlkBuffer_t;

