/*
 * Aeolus - a program to boot the Zephyr MIPS
 * Copyright (C) 2014 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * BCM3384 BMIPS5000 memory map:
 *
 * +-------------+-------------------------------+----------------------------+
 * | Phys addr   | Bootloader usage              | Linux usage                |
 * +-------------+-------------------------------+----------------------------+
 * | 0x0000_0000 | Main CPU reset vector / entry | Secondary CPU reset vector |
 * |             | MIPS init code (asm)          | (CPU1)                     |
 * +-------------+-------------------------------+----------------------------+
 * | 0x0000_0f00 | Kernel command line (from CM) |                            |
 * +-------------+-------------------------------+----------------------------+
 * | 0x0000_1000 |                               | Relocated MIPS exception   |
 * |             |                               | vectors (CPU0 and CPU1)    |
 * +-------------+-------------------------------+----------------------------+
 * | 0x0000_1400 | Device tree blob              | Device tree blob           |
 * +-------------+-------------------------------+----------------------------+
 * | 0x0000_c400 | Bootloader C code             |                            |
 * +-------------+-------------------------------+----------------------------+
 * | 0x0001_0000 | Kernel image start,           | Kernel image start,        |
 * |             | kernel entry address          | kernel entry address       |
 * +-------------+-------------------------------+----------------------------+
 * | 0x0700_0000 | Scratch space (stack top,     |                            |
 * |             | dtb, staging area)            |                            |
 * +-------------+-------------------------------+----------------------------+
 */

ENTRY(_start)

SECTIONS {
	. = 0x80000000;
	.init : {
		*(.init);
		*(.mipsinit);
	}

	. = 0x80000f00;
	_cmdline = .;

	. = 0x80001400;
	_dtb_start = .;
	.dtb : {
		*(.dtb);
	}
	. = ALIGN(4);
	_dtb_end = .;

	. = 0x8000c400;
	.main : {
		*(.text*);
		*(.rodata*);
		*(.data);
	}

	. = 0x8000fff8;
	.fill : {
		/* FIXME: why does this take 8 bytes? */
		LONG(0);
	}

	. = 0x80010000;
	_linux = .;

	. = 0x87000000;
	_fbss = .;
	.bss : {
		*(.bss*);
		*(.sbss*);
	} =0x00
	. = ALIGN(4);
	_ebss = .;

	/DISCARD/ : {
		*(.MIPS.options)
		*(.options)
		*(.pdr)
		*(.reginfo)
		*(.eh_frame)
	}
};
