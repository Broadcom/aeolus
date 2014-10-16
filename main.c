#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "libfdt.h"

#define KSEG0			0x80000000
#define KSEG1			0xa0000000
#define __iomem
#define __force

#define REG_ADDR(x)		((void *)KSEG1 + (x))
#define BIT(x)			(1 << (x))

#define REG_DDR_WIDTH		REG_ADDR(0x150b5004)
#define REG_DDR_TECH		REG_ADDR(0x150b2000)
#define REG_SDRAM_SPACE		REG_ADDR(0x14e00284)

#define REG_USB_SETUP		REG_ADDR(0x15400200)
#define USB_SETUP_IPP		BIT(5)
#define USB_SETUP_IOC		BIT(4)

#define REG_USB_SWAP		REG_ADDR(0x1540020c)
#define USB_SWAP_DATA		(BIT(1) | BIT(4))
#define USB_SWAP_DESC		(BIT(0) | BIT(3))

static char newdtb[65536];

typedef void (*kernel_entry_t)(unsigned long zero,
			       unsigned long machine_type,
			       unsigned long dtb_pa);

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

void main(kernel_entry_t kernel_entry,
	  char *cmdline,
	  void *dtb_start,
	  int dtb_len)
{
	int memsize_mb = setup_ddr();

	setup_usb();

	if (fdt_open_into(dtb_start, newdtb, sizeof(newdtb)) < 0)
		die("can't open builtin DTB");
	if (*cmdline && set_bootargs(newdtb, cmdline) < 0)
		die("can't set bootargs");
	if (set_memory_node(newdtb, memsize_mb) < 0)
		die("can't set memory size");
	if (fdt_pack(newdtb) < 0)
		die("can't pack new dtb");
	memcpy(dtb_start, newdtb, fdt_totalsize(newdtb));

	kernel_entry(0, 0xffffffff, virt_to_phys(dtb_start));
}
