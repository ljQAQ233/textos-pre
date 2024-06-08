SRC_DIR  := $(shell pwd)
# Set Source Root

DEBUG    := true

OUTPUT   := ../Build
OUTPUT   := $(abspath ${OUTPUT})
# Get the RealPath

BOOT_OUTPUT     := $(OUTPUT)/Boot
# For a Macro in .dsc file I defined,the output directory
ifeq (${ARCH},X64)
  BOOT_EXEC     := $(BOOT_OUTPUT)/BootX64.efi
else
  BOOT_EXEC     := $(BOOT_OUTPUT)/BootIa32.efi
endif
# Boot Built Executable EFI File Output File on Host

KERNEL_OUTPUT := $(OUTPUT)/Kernel
# Kernel Build Output Directory
KERNEL_EXEC   := $(KERNEL_OUTPUT)/Kernel.elf
# Kernel Built Executable ELF File Output File on Host

ifeq (${ARCH},X64)
  IMG_EFI = $(IMG_MDIR)/EFI/Boot/bootX64.efi
else
  $(error [ERR] Unsupported arch : $(ARCH))
  # IMG_EFI = $(IMG_MDIR)/EFI/Boot/bootia32.efi
  # QEMU    = $(QEMU_BINARY)/qemu-system-i386
endif

APP_OUTPUT     := $(OUTPUT)/App

SHELL  := bash
SUDO   := echo q | sudo -S
# including password !!!

BASE   := Base
UTILS  := Utils
BASE   := $(abspath ${BASE})
UTILS  := $(abspath ${UTILS})

export DEBUG
export CC NASM SHELL TERM SUDO
export SRC_DIR BASE UTILS
export BOOT_OUTPUT BOOT_EXEC KERNEL_OUTPUT KERNEL_EXEC APP_OUTPUT IMG_EFI
