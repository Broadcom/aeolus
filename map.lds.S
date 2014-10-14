/*
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
 * | 0x0000_d400 | Bootloader C code             |                            |
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
	.init 0x80000000 : {
		*(.init);
	}

	.main 0x8000d400 : {
		*(.text);
		*(.text.*);
		*(.rodata);
		*(.data);
	}

	.fill 0x8000fff8 : {
		/* FIXME: why does this take 8 bytes? */
		LONG(0x00);
	}

	_linux 0x80010000 : {
		_linux = .;
	}

	.bss 0x87000000 : {
		. = ALIGN(4);
		_fbss = .;
		*(.bss*);
		*(.sbss*);
		. = ALIGN(4);
		_ebss = .;
	} =0x00

	/DISCARD/ : {
		*(.MIPS.options)
		*(.options)
		*(.pdr)
		*(.reginfo)
		*(.eh_frame)
	}
};
