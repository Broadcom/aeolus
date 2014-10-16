CROSS_COMPILE	:= mips-linux-
CC		:= $(CROSS_COMPILE)gcc
LD		:= $(CROSS_COMPILE)ld
CPP		:= $(CROSS_COMPILE)cpp
OBJCOPY		:= $(CROSS_COMPILE)objcopy

COMMON_FLAGS	:= -mno-abicalls
CFLAGS		:= $(COMMON_FLAGS) -nostdlib -ffreestanding -Os
AFLAGS		:= $(COMMON_FLAGS)

CORE_OBJS	:= init.o main.o dtb.o
OBJS		:= $(CORE_OBJS)

aeolus.bin: aeolus.elf
	$(OBJCOPY) -O binary $< $@

aeolus.elf: $(OBJS) map.lds
	$(LD) $^ -o $@ -T map.lds

dtb.o: board.dtb
	$(OBJCOPY) -I binary -O elf32-tradbigmips --rename-section .data=.dtb \
		-B mips $< $@

%.lds: %.lds.S
	$(CPP) -P $< -o $@

%.o: %.S
	$(CC) -c $(AFLAGS) $< -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) map.lds aeolus.bin aeolus.elf
