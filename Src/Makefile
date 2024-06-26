ARCH   := X64

export ARCH OUTPUT

include Config/Main.mk
include Config/Image.mk

include Config/Udk.mk

ifneq ($(findstring qemubg,${MAKECMDGOALS}),)
  # Get the path of pipes that are used to debug uefi
  PIPE := $(shell cat $(UDK_DBG_CFG) | grep -E "^[P ]ort" | awk '{print $$NF}')
  export BSRC_DEBUG:=true
endif

.PHONY: all
all: _BootUpdate Kernel App Image

include Config/Qemu.mk

include Config/Private.mk

.PHONY: Boot
Boot:
	@echo -e "Start to Compile Boot Module...\n"
	@make -C Boot Build
	@echo -e "Boot Module was completed\n"

ovmf:
	@echo -e "Start to Compile OVMF...\n"
	# @make -C Boot TARGET=RELEASE Ovmf
	# @make -C Boot TARGET=DEBUG FLAGS="-DDEBUG_ON_SERIAL_PORT=TRUE" Ovmf
	@make -C Boot TARGET=NOOPT FLAGS="-DDEBUG_ON_SERIAL_PORT=TRUE -DSOURCE_DEBUG_ENABLE=TRUE" Ovmf
	@echo -e "OVMF is up-to-date\n"

.PHONY: Kernel
Kernel:
	@echo -e "Start to Compile Kernel Module...\n"
	@make -C Kernel
	@echo -e "Kernel Module was completed\n"

.PHONY: App
App:
	@echo -e "Start to Compile App Module...\n"
	@make -C App App
	@echo -e "App Module was completed\n"

.PHONY: Image
Image:
	@mkdir -p $(IMG_MDIR)
	@if ! $(SUDO) mount $(IMG_OUTPUT) $(IMG_MDIR) > /dev/null 2>&1;then \
		echo -e "Create an image : $(IMG_OUTPUT)" \
		;dd if=/dev/zero of=$(IMG_OUTPUT) bs=1M count=$(IMG_SIZE) > /dev/null 2>&1 \
		;mkfs.fat -F 32 --mbr=y $(IMG_OUTPUT) > /dev/null 2>&1 \
		;$(SUDO) mount $(IMG_OUTPUT) $(IMG_MDIR) \
	;fi
	
	@echo "Copying files for booting..."
	@$(SUDO) mkdir -p $(IMG_MDIR)/EFI/Boot/
	@$(SUDO) cp $(BOOT_EXEC) $(IMG_MDIR)/EFI/Boot/$(OUT_EFI)
	@$(SUDO) cp -rf Resources/* $(IMG_MDIR)/
	@$(SUDO) cp $(APP_OUTPUT)/* $(IMG_MDIR)/
	@$(SUDO) cp $(KERNEL_EXEC) $(IMG_MDIR)/
	
	@echo "Image was done!"
	@while $(SUDO) umount $(IMG_MDIR) 2>/dev/null; \
		do sleep 1	\
	;done
	@$(SUDO) rm -rf $(IMG_MDIR)
	@$(SUDO) chmod a+rw $(IMG_OUTPUT)

.PHONY: compile_commands.json
compile_commands.json: clean
	@echo -e "Generating compile_commands.json ..."
	@bear --output compile_commands_raw.json -- make clean all
	@grep -v "\-maccumulate-outgoing-args" compile_commands_raw.json  > $(UTILS)/compile_commands.json
	@rm compile_commands_raw.json
	@echo "compile_commands.json was up-to-date"

.PHONY: _BootUpdate
_BootUpdate:
	@make -C Boot Update

.PHONY: qemu
qemu: all
	$(QEMU) $(QEMU_FLAGS_RUN)

.PHONY: qemubg
qemubg: all
	@mkfifo $(PIPE).in $(PIPE).out
	@echo "Created pipe : $(PIPE)"
	@$(UDK_DBG_EXEC) 2>&1 &
	@echo "Started UdkDebugger"
	@$(UTILS)/MonitorQemu.sh > /dev/null 2>&1 & disown -h
	@echo "Running Qemu"
	@$(QEMU) $(QEMU_FLAGS_BDBG)

.PHONY: qemug
qemug: export BOOT_DEBUG=false
qemug: all
	@echo "Starting kernel debugging..."
	@$(QEMU) $(QEMU_FLAGS_KDBG)

.PHONY: diskmu
diskmu:
	@mkdir -p $(IMG_MDIR)
	@if ! $(SUDO) mount $(IMG_OUTPUT) $(IMG_MDIR) > /dev/null 2>&1;then \
		$(SUDO) umount $(IMG_MDIR) \
		;rm -rf $(IMG_MDIR) \
		;echo "Umounted the disk and removed the dir of the disk." \
	;else \
		echo "Created an dir for the disk and mounted the disk." \
	;fi

.PHONY: clean
clean:
	find . -iname "*.status_log" | xargs rm -f
	find . -iname "*.status_lock" | xargs rm -f
	rm -rf Boot/Edk2/Conf
	rm -rf Boot/Edk2/SigmaBootPkg
	rm -rf $(OUTPUT)

