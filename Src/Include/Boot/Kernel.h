#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <Boot/Page.h>
#include <Boot/Elf.h>

EFI_STATUS
KernelLoad (
  IN     CHAR16               *Path,
     OUT EFI_PHYSICAL_ADDRESS *Addr,
     OUT KERNEL_PAGE          **Pages
  );

#endif
