typedef void (*kernel_entry_t)(unsigned long zero,
			       unsigned long machine_type,
			       unsigned long dtb_pa);

#define KSEG0			0x80000000

static inline unsigned long virt_to_phys(void *ptr)
{
	return (unsigned long)ptr - KSEG0;
}

void main(kernel_entry_t kernel_entry,
	  char *cmdline,
	  void *dtb_start,
	  int dtb_len)
{
	kernel_entry(0, 0xffffffff, virt_to_phys(dtb_start));
}
