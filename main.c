void main(void (*kernel_entry)(void),
	  char *cmdline,
	  void *dtb_start,
	  int dtb_len)
{
	kernel_entry();
}
