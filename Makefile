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

# pick your board from $LINUXDIR/arch/mips/boot/dts/
DEFAULT_BOARD	:= bcm93384wvg

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

COMMON_FLAGS	:= -mno-abicalls -Wall -fno-strict-aliasing
CFLAGS		:= $(COMMON_FLAGS) -ffreestanding -Os \
		   -nostdlib -nostdinc -I$(NEWLIB_INCDIR) \
		   -I. -Ilibfdt
AFLAGS		:= $(COMMON_FLAGS)

CORE_OBJS	:= init.o bmips5000.o main.o dtb.o
OBJS		:= $(CORE_OBJS) $(LIBFDT_OBJS) $(NEWLIB_OBJS)

aeolus.bin: aeolus.elf
	$(OBJCOPY) -O binary $< $@

zephyr.img: aeolus.bin $(PROGSTORE) $(LINUXDIR)/vmlinux
	$(OBJCOPY) -O binary $(LINUXDIR)/vmlinux vmlinux.bin
	cat aeolus.bin vmlinux.bin > concat.bin
	$(PROGSTORE) -f concat.bin -o $@ -c 4 -s 0x3384 -a 0 -v 003.000 \
		-f2 dummy.txt
	rm -f vmlinux.bin concat.bin

$(LINUXDIR)/.config:
	$(MAKE) -C $(LINUXDIR) ARCH=mips bcm3384_defconfig

board.dtb: $(LINUXDIR)/.config
	$(MAKE) -C $(LINUXDIR) ARCH=mips dtbs
	cp -f $(LINUXDIR)/arch/mips/boot/dts/$(DEFAULT_BOARD).dtb board.dtb

.PHONY: $(LINUXDIR)/vmlinux
$(LINUXDIR)/vmlinux: $(LINUXDIR)/.config
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
	$(CPP) -P $< -o $@

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
