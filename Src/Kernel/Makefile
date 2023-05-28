SHELL  := bash
OUTPUT := $(KERNEL_OUTPUT)

export OUTPUT

SRCS := Init.c

OBJS := $(SRCS:.c=.c.o)
OBJS := $(OBJS:.s=.s.o)
OBJS := $(OBJS:.asm=.asm.o)

OBJS := $(addprefix ${OUTPUT}/,${OBJS})

include ../Config/Kernel.mk

.PHONY: Kernel
Kernel: $(KERNEL_OUTPUT) $(KERNEL_EXEC)

$(KERNEL_EXEC): $(OBJS)
	@$(LD) $(LDFLAGS) $^ -T Linker.ld -o $@
	@echo -e "\033[032m   LD  \033[0m Linked all objects -> $@"

$(KERNEL_OUTPUT):
	@mkdir -p $(KERNEL_OUTPUT)
	@echo -e "\033[032m   MKD \033[0m Created directory $@"

.PHONY: clean
clean:
	if [[ "$(abspath $(OUTPUT))" != "$$(pwd)" ]];then \
	  rm -rf $(OUTPUT) \
	;fi
	if [[ "$(abspath $(KERNEL_OUTPUT))" != "$$(pwd)" ]];then \
	  rm -rf $(KERNEL_OUTPUT) \
	;fi
	find . -iname "*.s.o"   | xargs rm -f
	find . -iname "*.asm.o" | xargs rm -f
	find . -iname "*.c.o"   | xargs rm -f

# Make rules and printing formats

$(OUTPUT)/%.s.o: %.s
	@mkdir -p $(@D)
	@$(NASM) $(NFLAGS) $< -o $@
	@echo -e "\033[032m   ASM \033[0m $<"

$(OUTPUT)/%.asm.o: %.asm
	@mkdir -p $(@D)
	@$(NASM) $(NFLAGS) $< -o $@
	@echo -e "\033[032m   ASM \033[0m $<"

$(OUTPUT)/%.c.o: %.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo -e "\033[032m   CC  \033[0m $<"
