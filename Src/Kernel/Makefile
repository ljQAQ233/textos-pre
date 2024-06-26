SHELL  := bash
OUTPUT := $(KERNEL_OUTPUT)

ARCH_DIR = Arch/$(ARCH)

export OUTPUT ARCH_DIR

SRCS := Init.c Main.c Debug.c Assert.c Panic.c Acpi.c Apic.c Task.c Task.s Dev.c

include Init/Init.mk
include Dev/Dev.mk
include Memory/Memory.mk
include Console/Console.mk
include Graphics/Graphics.mk

include FileSys/FileSys.mk

include User/User.mk

include Lib/Lib.mk
include Stdlib/Stdlib.mk

include $(ARCH_DIR)/Arch.mk

OBJS := $(SRCS:.c=.c.o)
OBJS := $(OBJS:.s=.s.o)
OBJS := $(OBJS:.asm=.asm.o)

OBJS := $(addprefix ${KERNEL_OUTPUT}/,${OBJS})

include ../Config/Kernel.mk

.PHONY: Kernel
Kernel: $(KERNEL_OUTPUT) $(KERNEL_EXEC)

$(KERNEL_EXEC): $(OBJS)
	@$(LD) $(LDFLAGS) $^ -T Linker.ld -o $@
	@echo -e "\033[032m   LD  \033[0m Linked all objects -> $@"
	@if [[ "$(DEBUG)" == "true" ]];then \
	  $(OBJCOPY) --only-keep-debug $@ $@.symbol \
	  ;echo -e "\033[032m   OCP \033[0m $@ -> ${@:$(KERNEL_OUTPUT)/%=%}.symbol" \
	;fi
	@$(OBJCOPY) --strip-debug $@ $@

$(KERNEL_OUTPUT):
	@mkdir -p $(KERNEL_OUTPUT)
	@echo -e "\033[032m   MKD \033[0m Created directory $@"

.PHONY: clean
clean:
	if [[ "$(abspath $(KERNEL_OUTPUT))" != "$$(pwd)" ]];then \
	  rm -rf $(KERNEL_OUTPUT) \
	;fi
	if [[ "$(abspath $(KERNEL_OUTPUT))" != "$$(pwd)" ]];then \
	  rm -rf $(KERNEL_OUTPUT) \
	;fi
	find . -iname "*.s.o"   | xargs rm -f
	find . -iname "*.asm.o" | xargs rm -f
	find . -iname "*.c.o"   | xargs rm -f

# Make rules and printing formats

$(KERNEL_OUTPUT)/%.s.o: %.s
	@mkdir -p $(@D)
	@$(NASM) $(NFLAGS) $< -o $@
	@echo -e "\033[032m   ASM \033[0m $<"

$(KERNEL_OUTPUT)/%.asm.o: %.asm
	@mkdir -p $(@D)
	@$(NASM) $(NFLAGS) $< -o $@
	@echo -e "\033[032m   ASM \033[0m $<"

$(KERNEL_OUTPUT)/%.c.o: %.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo -e "\033[032m   CC  \033[0m $<"

