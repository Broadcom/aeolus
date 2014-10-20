Aeolus: a program to boot the Zephyr MIPS
=========================================

## Basic usage

Install a suitable
[32-bit MIPS BE toolchain](http://www.linux-mips.org/wiki/Toolchains) called
<code>mips-linux-gcc</code> in your PATH, and then run:

    git clone git://github.com/Broadcom/aeolus.git
    git clone -b bcm3384 git://github.com/cernekee/linux.git
    cd aeolus
    make zephyr.img

This will build a BCM3384 kernel with default options, targeted for the
BCM93384WVG reference platform, and using <code>mr-rootfs.cpio.xz</code>
as an initramfs.

## Boot sequence

The BCM3384 SoC has two onchip big-endian MIPS32R1 processors:

* One Viper (BMIPS4355) responsible for the cable modem (CM) subsystem.
This runs an RTOS (eCos).
* One Zephyr (BMIPS5000) application processor that can run SMP Linux, Samba
services, CUPS, netfilter based routing, etc.

When the chip is powered on, the Viper is released from reset and performs
a number of helpful tasks, including:

* Configure the DRAM controller, console UART, pinmux, and other basic
peripherals
* Figure out which CM image to load from flash, and start it up
* Copy the Linux image from flash to DRAM
* Write the user-configurable kernel command line to a fixed memory address
* Release the Zephyr CPU from reset

Aeolus implements a minimal bootloader for the Zephyr, handling the following
functions:

* Initialize L1/L2 caches and processor configuration
* Configure BCM3384-specific USB hardware
* Configure BCM3384-specific memory hardware
* Pass a device tree blob (DTB) describing the BCM3384 register layout to
the kernel
* Determine the Zephyr-owned memory ranges, and add them into the DTB
"memory@0" node
* Copy the kernel command line passed from CM into the DTB chosen bootargs
property

When you build zephyr.img, the bootloader image is concatenated with a kernel
image (linked to run at virtual address 0x8001_0000).  When the Zephyr
CPU is released from reset, it first runs Aeolus to perform basic setup and
then jumps into a mostly-stock MIPS Linux kernel.

A BCM3384 DTB blob is built into both Aeolus and the kernel.  The kernel's
builtin DTB is used as a fallback only.  The DTB provided by Aeolus will
accurately reflect the system's memory configuration and the kernel command
line arguments configured through the CM console.  It is also possible for
different builds of Aeolus for different boards to incorporate board-specific
changes in device tree (e.g. leaving out the OHCI/EHCI nodes if the board has
no USB ports).

The memory map is documented in [map.lds.S](map.lds.S).

## Licensing

Aeolus and ProgramStore are licensed under GPLv2.

This program incorporates portions of newlib (BSD), libfdt (GPLv2/BSD),
and gcc (GPLv3 + runtime exception).  Various macros and code snippets
were copied from the Linux tree (GPLv2).
