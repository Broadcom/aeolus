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
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "libfdt.h"

#define REG_CHIP_ID		REG_ADDR(0x14e00000)
#define REG_DDR_WIDTH		REG_ADDR(0x150b5004)
#define REG_DDR_TECH		REG_ADDR(0x150b2000)
#define REG_SDRAM_SPACE		REG_ADDR(0x14e00284)
#define USB_BASE		0x15400200
#define UART_BASE		0x14e00520

#define KSEG0			0x80000000
#define KSEG1			0xa0000000
#define __iomem
#define __force

#define REG_ADDR(x)		((void *)KSEG1 + (x))
#define BIT(x)			(1 << (x))

#define REG_USB_SETUP		REG_ADDR(USB_BASE + 0x00)
#define USB_SETUP_IPP		BIT(5)
#define USB_SETUP_IOC		BIT(4)

#define REG_USB_SWAP		REG_ADDR(USB_BASE + 0x0c)
#define USB_SWAP_DATA		(BIT(1) | BIT(4))
#define USB_SWAP_DESC		(BIT(0) | BIT(3))

#define REG_UART_CONTROL	REG_ADDR(UART_BASE + 0x00)
#define UART_CONTROL_RXTIME_s	0
#define UART_CONTROL_STOP_s	8
#define UART_CONTROL_DBIT_s	12
#define UART_CONTROL_RXEN	BIT(21)
#define UART_CONTROL_TXEN	BIT(22)
#define UART_CONTROL_BAUDEN	BIT(23)

#define REG_UART_BAUD		REG_ADDR(UART_BASE + 0x04)

#define REG_UART_MISC		REG_ADDR(UART_BASE + 0x08)
#define UART_MISC_RXFIFO_s	8
#define UART_MISC_TXFIFO_s	12

#define REG_UART_EXTINPUT	REG_ADDR(UART_BASE + 0x0c)
#define REG_UART_INTSTAT	REG_ADDR(UART_BASE + 0x10)
#define REG_UART_FIFO		REG_ADDR(UART_BASE + 0x14)

static char newdtb[65536];

typedef void (*kernel_entry_t)(unsigned long zero,
			       unsigned long machine_type,
			       unsigned long dtb_pa,
			       unsigned long chip_id);

/* Utility functions */

static inline unsigned long virt_to_phys(void *ptr)
{
	return (unsigned long)ptr - KSEG0;
}

static inline uint32_t __raw_readl(const volatile void __iomem *addr)
{
	return *(const volatile uint32_t __force *) addr;
}

static inline void __raw_writel(uint32_t b, volatile void __iomem *addr)
{
	*(volatile uint32_t __force *) addr = b;
}

static void die(const char *msg)
{
	while (1)
		;
}

/* Device tree manipulation */

static int find_node(void *dtb, const char *match, int max_depth)
{
	int offset, depth;

	for (offset = 0, depth = 0; ; ) {
		const char *name;

		offset = fdt_next_node(dtb, offset, &depth);
		if (offset < 0)
			return -1;
		name = fdt_get_name(dtb, offset, NULL);
		if (name && !strcmp(name, match) && depth <= max_depth)
			return offset;
	}
}

static int find_or_create_node(void *dtb, const char *name)
{
	int offset = find_node(dtb, name, 1);
	if (offset > 0)
		return offset;

	return fdt_add_subnode(dtb, 0, name);
}

static int set_bootargs(void *dtb, const char *args)
{
	int offset = find_or_create_node(dtb, "chosen");
	if (offset < 0)
		return -1;
	if (fdt_setprop_string(dtb, offset, "bootargs", args) < 0)
		return -1;
	return 0;
}

static int set_memory_node(void *dtb, int dram_size_mb)
{
	int offset;

	offset = find_or_create_node(dtb, "memory@0");
	if (offset < 0)
		return -1;

	fdt_setprop_string(dtb, offset, "device_type", "memory");

	/* Linux always owns 0MB - 128MB */
	fdt_setprop_u32(dtb, offset, "reg", 0x0);
	fdt_appendprop_u32(dtb, offset, "reg", 128 << 20);

	if (dram_size_mb > 256) {
		/* skip CM @ 128MB and the memory hole @ 256MB */
		dram_size_mb -= 256;
		fdt_appendprop_u32(dtb, offset, "reg", 0x20000000);
		fdt_appendprop_u32(dtb, offset, "reg", dram_size_mb << 20);
	}

	return 0;
}

static int setup_ddr(void)
{
	/* 0x00 = 256 Mbit = 32 Mbyte */
	int shift = __raw_readl(REG_DDR_TECH) & 0x07;

	/* double it if there's a second chip (32b wide instead of 16b wide) */
	shift += __raw_readl(REG_DDR_WIDTH) & 0x01;

	__raw_writel(shift + 5, REG_SDRAM_SPACE);
	return 32 << shift;
}

static void setup_usb(void)
{
	uint32_t tmp;

	tmp = __raw_readl(REG_USB_SETUP);
	tmp &= ~USB_SETUP_IPP;	/* IPP=0 => active high */
	tmp |= USB_SETUP_IOC;	/* IOC=1 => active low */
	__raw_writel(tmp, REG_USB_SETUP);

	tmp = __raw_readl(REG_USB_SWAP);
	tmp &= ~USB_SWAP_DATA;
	tmp |= USB_SWAP_DESC;
	__raw_writel(tmp, REG_USB_SWAP);
}

static void setup_uart(void)
{
	/* port enabled, 8N1 */
	__raw_writel(UART_CONTROL_RXEN |
		     UART_CONTROL_TXEN |
		     UART_CONTROL_BAUDEN |
		     (1 << UART_CONTROL_RXTIME_s) |
		     (7 << UART_CONTROL_STOP_s) |
		     (3 << UART_CONTROL_DBIT_s), REG_UART_CONTROL);

	/* 115200bps */
	__raw_writel(0xe, REG_UART_BAUD);

	/* FIFO IRQ thresholds */
	__raw_writel((8 << UART_MISC_RXFIFO_s) |
		     (8 << UART_MISC_TXFIFO_s),
		     REG_UART_MISC);

	/* disable IRQs */
	__raw_writel(0, REG_UART_INTSTAT);
}

void main(kernel_entry_t kernel_entry,
	  char *cmdline,
	  void *dtb_start,
	  int dtb_len)
{
	int memsize_mb = setup_ddr();
	unsigned long chip_id = __raw_readl(REG_CHIP_ID);

	setup_uart();
	setup_usb();

	/* Let the kernel use its builtin DTB if ours is bogus */
	if (fdt_check_header(dtb_start) < 0)
		kernel_entry(0, 0, 0, chip_id);

	if (fdt_open_into(dtb_start, newdtb, sizeof(newdtb)) < 0)
		die("can't open builtin DTB");
	if (*cmdline && set_bootargs(newdtb, cmdline) < 0)
		die("can't set bootargs");
	if (set_memory_node(newdtb, memsize_mb) < 0)
		die("can't set memory size");
	if (fdt_pack(newdtb) < 0)
		die("can't pack new dtb");
	memcpy(dtb_start, newdtb, fdt_totalsize(newdtb));

	kernel_entry(0, 0xffffffff, virt_to_phys(dtb_start), chip_id);
}
