OUTPUT    ?= .
SHELL     := bash

-include ../Config/Boot.mk

ifeq (${BOOT_DEBUG},true)
 TARGET := DEBUG
else
 TARGET := RELEASE
endif

.PHONY: _Pre
_Pre:
	@ln -snf $(abspath SigmaBootPkg) Edk2/SigmaBootPkg

.PHONY: _OptsGen
_OptsGen:
	@echo -e "include" $(shell find ${BOOT_OUTPUT} -iname GNUMakefile | grep $(PROJ) | grep "$(TARGET)_$(TOOLCHAIN)" | awk 'NR==1{print $$0}') > /tmp/OptsGen.makefile
	@echo -e "WORKSPACE=$(abspath Edk2)" >> /tmp/OptsGen.makefile
	@echo -e "_main:" >> /tmp/OptsGen.makefile
	@echo -e "\t@\$$(CC) -dM \$$(CC_FLAGS) \$$(INC) -E - <<< \"#include <Uefi.h>\" > $(OUTPUT)/ImportUefi.h" >> /tmp/OptsGen.makefile
	@echo -e "\t@\$$(CC) \$$(CC_FLAGS) \$$(INC) -E - <<< \"#include <Uefi.h>\" >> $(OUTPUT)/ImportUefi.h" >> /tmp/OptsGen.makefile
	@make -f /tmp/OptsGen.makefile _main

.PHONY: Ovmf
Ovmf: export BOOT_OUTPUT:=$(BOOT_OUTPUT)/OVMF_$(ARCH)
Ovmf:
	@mkdir -p Edk2/Conf
	@echo -e "Update configures for compliler...\n"
	@export WORKSPACE=$(abspath Edk2) && \
		source Edk2/BaseTools/BuildEnv && \
		if ! build --help > /dev/null;then \
			rm -rf Conf/tools_def.txt \
			Conf/target.txt \
			Conf/build_rule.txt \
		;fi

	@echo -e "Start to build Boot Module...\n"
	@export WORKSPACE=$(abspath Edk2) && \
		source Edk2/BaseTools/BuildEnv > /dev/null && \
		build $(FLAGS) -p OvmfPkg/OvmfPkg.dsc \
		-a $(ARCH) \
		-t $(TOOLCHAIN) \
		-b $(TARGET) \
		-DOUTPUT=$(BOOT_OUTPUT) \
		-DARCH=$(ARCH)
	@cp -f `find $(BOOT_OUTPUT) -name "OVMF.fd" | grep "$(TARGET)_$(TOOLCHAIN)" | grep $(ARCH) | awk 'NR==1{print $$0}'` $(BASE)/OVMF_$(TARGET)_$(ARCH).fd
	@echo

.PHONY: Build
Build: _Pre
	@mkdir -p Edk2/Conf
	@echo -e "Update configures for compliler...\n"
	@export WORKSPACE=$(abspath Edk2) && \
	source Edk2/BaseTools/BuildEnv && \
		if ! build --help > /dev/null;then \
			rm -rf Conf/tools_def.txt \
			Conf/target.txt \
			Conf/build_rule.txt \
		;fi
	
	@echo -e "Start to build Boot Module...\n"
	@export WORKSPACE=$(abspath Edk2) && \
	source Edk2/BaseTools/BuildEnv > /dev/null && \
		build $(FLAGS) -p $(DSC) \
		-a $(ARCH) \
		-t $(TOOLCHAIN) \
		-b $(TARGET) \
		-DOUTPUT=$(BOOT_OUTPUT) \
		-DCFLAGS="$(CFLAGS)" 2>&1
	@TARGET=$(TARGET) $(UTILS)/CheckModifyUnlock.sh $(PROJ)
	@echo

.PHONY: Update
Update:
	@if find $(BOOT_OUTPUT) -iname $(_PLATFORM_NAME).efi 2>/dev/null | grep "$(TARGET)_$(TOOLCHAIN)" | grep "$(ARCH)" | awk 'NR==1{print $$0}' | xargs test -z || ! TARGET=$(TARGET) $(UTILS)/CheckModify.sh $(PROJ) >/dev/null 2>&1;then \
		make -C .. Boot \
	;fi
	@$(SUDO) cp -rf "`find $(BOOT_OUTPUT) -iname $(_PLATFORM_NAME).efi | grep "$(TARGET)_$(TOOLCHAIN)" | grep "$(ARCH)" | awk 'NR==1{print $$0}'`" $(BOOT_EXEC)
