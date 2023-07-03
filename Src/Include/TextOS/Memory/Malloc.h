#pragma once

/*
   Allocate memory in kernel heap by Siz in bytes.

   @retval  void* The virtual address of the memory
*/
void *MallocK (size_t Siz);

/*
   Free memory.
*/
void FreeK (void *Addr);

/*
   Do not export.
*/
// void *SBrk (int64 Siz);
