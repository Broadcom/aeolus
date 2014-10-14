CROSS_COMPILE	:= mips-linux-
CC		:= $(CROSS_COMPILE)gcc
LD		:= $(CROSS_COMPILE)ld
CPP		:= $(CROSS_COMPILE)cpp
OBJCOPY		:= $(CROSS_COMPILE)objcopy

CFLAGS		:= -nostdlib -ffreestanding -Os
AFLAGS		:=

CORE_OBJS	:= init.o main.o
OBJS		:= $(CORE_OBJS)

aeolus.bin: aeolus.elf
	$(OBJCOPY) -O binary $< $@

aeolus.elf: $(OBJS) map.lds
	$(LD) $^ -o $@ -T map.lds

%.lds: %.lds.S
	$(CPP) -P $< -o $@

%.o: %.S
	$(CC) -c $(AFLAGS) $< -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f *.o map.lds
