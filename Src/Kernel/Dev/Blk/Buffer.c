#include <TextOS/Dev.h>
#include <TextOS/Dev/Buffer.h>

BlkBuffer_t *BlkBufferInit (BlkBuffer_t *Buffer)
{
}

void BlkRead (Dev_t *Dev, u64 Idx, void *Buffer, size_t Count)
{
    Dev->BlkRead (Dev, Idx, Buffer, Count);
}

