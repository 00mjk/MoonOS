CC = @cc
AS = @nasm
AR = @ar
LD = @ld

EMU = qemu-system-x86_64
EMU_OPTS = -m 2G -serial mon:stdio -hda $(KERNEL_HDD) -d cpu_reset
EMU_DEBUG_OPTS = -S -s -m 2G -serial mon:stdio -hda $(KERNEL_HDD) -d cpu_reset --no-reboot

LIB_DIR  = ../libs/

CFILES	 := $(shell find ./ -type f -name '*.c')
ASMFILES := $(shell find ./ -type f -name '*.s')
OBJ_C    := $(CFILES:.c=.o)
OBJ_ASM  := $(ASMFILES:.s=.o)
OBJ		 := $(OBJ_C) $(OBJ_ASM)

KLIBS 	 := ../libs

# Kernel compiler / linker flags
CFLAGS := 				 \
	-I ../libs/			 \
	-O2	-pipe -g		 \
	-ffreestanding       \
	-fno-stack-protector \
	-fno-pic             \
	-mno-80387           \
	-mno-mmx             \
	-mno-3dnow           \
	-mcmodel=kernel      \
	-mno-red-zone

ASMFLAGS = -felf64 -g -F dwarf
LDINTERNALFLAGS := \
	-Tlinker.ld    \
	-static        \
	-nostdlib      \
	-no-pie		   
