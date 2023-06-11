ARCH    ?= X64

# Commands
CC      := gcc
LD      := ld
NASM    := nasm
OBJCOPY := objcopy

# Include path
INCLUDE := \
  $(SRC_DIR)/include \
  $(SRC_DIR)/include/textos/klib \
  $(abspath arch/$(ARCH))

CFLAGS := \
  -static -nostdlib -nostdinc -g \
  -std=c11 -O0 -fshort-wchar -ffreestanding \
  -fno-builtin -fno-stack-check -fno-stack-protector \
  -include $(SRC_DIR)/include/textos/textos.h \
  $(addprefix -I,${INCLUDE})

CFLAGS   += -D__TEXTOS_DEBUG

# Nasm flags
NFLAGS := \
  -f elf64 -g

LDFLAGS := \
  -static -nostdlib \
  -z max-page-size=0x1000 \
  -Map=$(KERNEL_OUTPUT)/system.map

