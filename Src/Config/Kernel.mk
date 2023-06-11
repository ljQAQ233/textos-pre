ARCH         ?= X64

# Commands
CC      := gcc
LD      := ld
NASM    := nasm
OBJCOPY := objcopy

# Include path
INCLUDE := $(SRC_DIR)/Include
INCLUDE += $(SRC_DIR)/Include/TextOS/Stdlib
INCLUDE += $(abspath Arch/$(ARCH))

CFLAGS   := -static
CFLAGS   += -nostdlib
CFLAGS   += -nostdinc
CFLAGS   += -fshort-wchar
CFLAGS   += -fno-builtin
CFLAGS   += -ffreestanding
CFLAGS   += -fno-stack-protector
CFLAGS   += $(addprefix -I,${INCLUDE})
CFLAGS   += -include $(SRC_DIR)/Include/TextOS/TextOS.h
CFLAGS   += -g -O0
CFLAGS   += -std=c11

ifeq (${DEBUG},true)
	CFLAGS += -D__TEXTOS_DEBUG
endif

# Nasm flags
NFLAGS   := -f elf64

LDFLAGS  := -static
LDFLAGS  += -nostdlib
LDFLAGS  += -z max-page-size=0x1000
LDFLAGS  += -Map=$(KERNEL_OUTPUT)/System.map


