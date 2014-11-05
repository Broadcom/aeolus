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
 * BCM3384 bootloader memory map:
 *   For Linux on Zephyr, the offsets are based on PA 0x0000_0000
 *   For Linux on Viper, the offsets are based on PA 0x0600_0000
 *
 * +-------------+-------------------------------+----------------------------+
 * | Offset      | Bootloader usage              | Linux usage                |
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
 * | 0x07f0_0000 | Scratch space (stack top,     |                            |
 * | (absolute)  | dtb, staging area)            |                            |
 * +-------------+-------------------------------+----------------------------+
 */

#define SCRATCH_START		0x87f00000

ENTRY(_start)

SECTIONS {
	. = MEM_START;
	.init : {
		*(.init);
		*(.mipsinit);
	}

	. = MEM_START + 0x0f00;
	_cmdline = .;

	. = MEM_START + 0x1400;
	_dtb_start = .;
	.dtb : {
		*(.dtb);
	}
	. = ALIGN(4);
	_dtb_end = .;

	. = MEM_START + 0xc400;
	.main : {
		*(.text*);
		*(.rodata*);
		*(.data);
	}

	. = MEM_START + 0xfff8;
	.fill : {
		/* FIXME: why does this take 8 bytes? */
		LONG(0);
	}

	. = MEM_START + 0x10000;
	_linux = .;

	. = SCRATCH_START;
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
