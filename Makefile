CROSS_COMPILE	:= mips-linux-
CC		:= $(CROSS_COMPILE)gcc
LD		:= $(CROSS_COMPILE)ld
CPP		:= $(CROSS_COMPILE)cpp
AR		:= $(CROSS_COMPILE)ar
RANLIB		:= $(CROSS_COMPILE)ranlib
OBJCOPY		:= $(CROSS_COMPILE)objcopy

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

.PHONY: clean
clean:
	rm -rf $(OBJS) map.lds aeolus.bin aeolus.elf $(NEWLIB_BUILD)
