NASM_FLAGS = -f elf64 -DX86_64_TARGET -DUEFI
ASM_FLAGS = -O0 -g3 -mno-red-zone -nostdlib -ffreestanding -fno-builtin -m64 -fno-stack-protector -fno-exceptions
C_FLAGS = -O0 -g3 -mno-red-zone -nostdlib -ffreestanding -fno-builtin -m64 -fno-stack-protector -fno-exceptions -std=gnu11 -fshort-wchar -nostdinc -I ./include
CPP_FLAGS = -O0 -g3 -mno-red-zone -nostdlib -ffreestanding -fno-builtin -m64 -fno-stack-protector -fno-exceptions -fno-rtti -std=gnu++17 -fshort-wchar -nostdinc -fno-use-cxa-atexit -fno-threadsafe-statics -I ./include

DEBUG ?= 1
SMP ?= 4
SUDO ?= 0
KVM ?= 0

ifeq ($(DEBUG), 1)
DEBUG_FLAG = -s -S
endif

ifeq ($(SUDO), 1)
SUDO_CMD = sudo
endif

ifeq ($(KVM), 1)
KVM_FLAG = --enable-kvm
endif

BUILD_DIR := out

COMPILER_PREFIX ?= 

# FILES
NASM_FILES := $(shell find ./src ./demo-kernel -name "*.asm")
ASM_FILES := $(shell find ./src ./demo-kernel -name "*.S")
C_FILES := $(shell find ./demo-kernel -name "*.c")
CPP_FILES := $(shell find ./src ./demo-kernel -name "*.cpp")

export CC = $(COMPILER_PREFIX)gcc
export CPP = $(COMPILER_PREFIX)g++
export NASM = $(COMPILER_PREFIX)nasm
export LD = $(COMPILER_PREFIX)ld
export NM = $(COMPILER_PREFIX)nm
export OBJC = $(COMPILER_PREFIX)objcopy
export OBJD = $(COMPILER_PREFIX)objdump

NASM_OBJS := $(patsubst ./%.asm, ./$(BUILD_DIR)/%.o, $(NASM_FILES))
ASM_OBJS := $(patsubst ./%.S, ./$(BUILD_DIR)/%.o, $(ASM_FILES))
C_OBJS := $(patsubst ./%.c, ./$(BUILD_DIR)/%.o, $(C_FILES))
CPP_OBJS := $(patsubst ./%.cpp, ./$(BUILD_DIR)/%.o, $(CPP_FILES))

$(BUILD_DIR)/%.o: %.asm
	@mkdir -p $(@D)
	@$(NASM) $(NASM_FLAGS) $< -o $@
	@echo "NA $< -> $@"

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(@D)
	@$(CC) -mcmodel=large $(ASM_FLAGS) -c $< -o $@
	@echo "AS $< -> $@"

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	@$(CC) -fno-pic -fno-pie -mcmodel=kernel $(C_FLAGS) -c $< -o $@
	@echo "CC $< -> $@"

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	@$(CPP) -fno-pic -fno-pie -mcmodel=kernel  $(CPP_FLAGS) -c $< -o $@
	@echo "CPP $< -> $@"

compile: $(ASM_OBJS) $(C_OBJS) $(CPP_OBJS) $(NASM_OBJS)
# 针对BOOT和KERNEL进行特殊处理
	@x86_64-w64-mingw32-gcc -I ./include -Wextra -e efi_main -nostdinc -nostdlib -fno-builtin -Wl,--subsystem,10 -fshort-wchar -o $(BUILD_DIR)/BOOTX64.efi src/main.c

	@$(LD) -z muldefs -T linker.ld --static -o ./$(BUILD_DIR)/kernel.elf $(shell find ./out -name "*.o")

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)

cleardir:
	@mkdir -p dist
	@mkdir -p dist/EFI/BOOT
	@cp -a $(BUILD_DIR)/BOOTX64.efi dist/EFI/BOOT
	@cp -a $(BUILD_DIR)/kernel.elf dist

all: compile cleardir
