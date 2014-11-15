#
# Aeolus - a program to boot the Zephyr MIPS
# Copyright (C) 2014 Broadcom Corporation
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#

# override this to use "make zephyr.img"
LINUXDIR	:= ../linux

ifneq ($(VIPER),1)
# pick your board from $LINUXDIR/arch/mips/boot/dts/
DEFAULT_BOARD	:= bcm93384wvg
MEM_START	:= 0x80000000
MIPS_INIT	:= bmips5000.o
VIPER		:= 0
else
# experimental Viper MIPS builds
DEFAULT_BOARD	:= bcm93384wvg_viper
MEM_START	:= 0x86000000
PHYSICAL_START	:= 0x86010000
MIPS_INIT	:= bmips4350.o
endif

# This is a slightly modified MetaROUTER OpenWRT rootfs (pretty much the
# bare minimum needed to boot the system).
# Source: http://openwrt.wk.cz/kamikaze/openwrt-mr-mips-rootfs-18961.tar.gz
# A few of the startup scripts were changed; the binaries are as-is from
# upstream.
# Leave this variable blank for no rootfs.
DEFAULT_ROOTFS	:= misc/mr-rootfs.cpio.xz

CROSS_COMPILE	:= mips-linux-
CC		:= $(CROSS_COMPILE)gcc
LD		:= $(CROSS_COMPILE)ld
CPP		:= $(CROSS_COMPILE)cpp
AR		:= $(CROSS_COMPILE)ar
RANLIB		:= $(CROSS_COMPILE)ranlib
OBJCOPY		:= $(CROSS_COMPILE)objcopy

PROGSTORE	:= ProgramStore/ProgramStore

NEWLIB_BUILD	:= newlib/build
NEWLIB_OBJS	:= $(NEWLIB_BUILD)/mips-none-elf/newlib/libc.a
NEWLIB_INCDIR	:= newlib/newlib/libc/include

LIBFDT_OBJS	:= libfdt/fdt.o libfdt/fdt_ro.o libfdt/fdt_rw.o libfdt/fdt_wip.o

COMMON_FLAGS	:= -mno-abicalls -Wall -fno-strict-aliasing \
		   -DMEM_START=$(MEM_START) -DVIPER=$(VIPER)
CFLAGS		:= $(COMMON_FLAGS) -ffreestanding -Os \
		   -nostdlib -nostdinc -I$(NEWLIB_INCDIR) \
		   -I. -Ilibfdt
AFLAGS		:= $(COMMON_FLAGS)

CORE_OBJS	:= init.o $(MIPS_INIT) main.o dtb.o
OBJS		:= $(CORE_OBJS) $(LIBFDT_OBJS) $(NEWLIB_OBJS)

aeolus.bin: aeolus.elf
	$(OBJCOPY) -O binary $< $@

zephyr.img: aeolus.bin $(PROGSTORE) $(LINUXDIR)/vmlinux
	$(OBJCOPY) -O binary $(LINUXDIR)/vmlinux vmlinux.bin
	cat aeolus.bin vmlinux.bin > concat.bin
	$(PROGSTORE) -f concat.bin -o $@ -c 4 -s 0x3384 -a 0 -v 003.000 \
		-f2 misc/dummy.txt
	rm -f vmlinux.bin concat.bin

$(LINUXDIR)/.linux-configured:
	$(MAKE) -C $(LINUXDIR) ARCH=mips bmips_be_defconfig
ifneq ($(DEFAULT_ROOTFS),)
	xz -d < $(DEFAULT_ROOTFS) > $(LINUXDIR)/rootfs.cpio
	cd $(LINUXDIR) && ./scripts/config \
		--set-str CONFIG_INITRAMFS_SOURCE rootfs.cpio \
		--set-val CONFIG_INITRAMFS_ROOT_UID 0 \
		--set-val CONFIG_INITRAMFS_ROOT_GID 0
endif
ifneq ($(PHYSICAL_START),)
	cd $(LINUXDIR) && ./scripts/config \
		--enable CONFIG_CRASH_DUMP \
		--enable CONFIG_PROC_VMCORE \
		--set-val CONFIG_PHYSICAL_START $(PHYSICAL_START)
endif
	$(MAKE) -C $(LINUXDIR) ARCH=mips dtbs
	touch $@

board.dtb: $(LINUXDIR)/.linux-configured
	cp -f $(LINUXDIR)/arch/mips/boot/dts/$(DEFAULT_BOARD).dtb board.dtb

.PHONY: $(LINUXDIR)/vmlinux
$(LINUXDIR)/vmlinux: $(LINUXDIR)/.linux-configured
	$(MAKE) -C $(LINUXDIR) ARCH=mips vmlinux

aeolus.elf: $(OBJS) map.lds
	$(LD) $^ -o $@ -T map.lds

dtb.o: board.dtb
	$(OBJCOPY) -I binary -O elf32-tradbigmips --rename-section .data=.dtb \
		-B mips $< $@

$(NEWLIB_BUILD)/Makefile:
	mkdir -p $(NEWLIB_BUILD)
	cd $(NEWLIB_BUILD) && ../configure \
		--target=mips-none-elf \
		CFLAGS_FOR_TARGET="-mno-abicalls -Os" \
		CC_FOR_TARGET=$(CC) \
		LD_FOR_TARGET=$(LD) \
		AR_FOR_TARGET=$(AR) \
		RANLIB_FOR_TARGET=$(RANLIB)

$(NEWLIB_OBJS): $(NEWLIB_BUILD)/Makefile
	$(MAKE) -C $(NEWLIB_BUILD)

%.lds: %.lds.S
	$(CPP) -DMEM_START=$(MEM_START) -P $< -o $@

%.o: %.S
	$(CC) -c $(AFLAGS) $< -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(PROGSTORE):
	$(MAKE) -C ProgramStore

.PHONY: clean
clean:
	rm -rf $(OBJS) map.lds aeolus.bin aeolus.elf $(NEWLIB_BUILD)
	$(MAKE) -C ProgramStore clean
