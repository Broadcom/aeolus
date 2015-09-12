Aeolus: a program to boot the Zephyr MIPS
=========================================

## Compiling

### Prerequisites

Install the <code>mips-linux-gcc</code> cross toolchain:

    wget https://www.broadcom.com/docs/support/stb/stbgcc-4.8-1.0.tar.bz2
    mkdir -p /opt/toolchains
    tar -C /opt/toolchains -jxf stbgcc-4.8-1.0.tar.bz2

(or pick any other suitable 32-bit MIPS BE toolchain
[from this list](http://www.linux-mips.org/wiki/Toolchains))

Install the Device Tree Compiler, either
[from source](http://www.devicetree.org/Device_Tree_Compiler) or from a
distribution package, e.g.

    apt-get install device-tree-compiler
    yum install dtc

Install the other miscellaneous development tools:

    apt-get install build-essential texinfo
    yum groupinstall "Development Tools"

Then clone the bootloader and kernel source trees:

    git clone git://github.com/Broadcom/aeolus.git
    git clone git://github.com/Broadcom/stblinux.git linux

### Build process

Typical usage:

    export PATH=/opt/toolchains/stbgcc-4.8-1.0/bin:$PATH
    cd aeolus
    make zephyr.img

This will build a BCM3384 kernel with default options, targeted for the
BCM93384WVG reference platform, and using <code>mr-rootfs.cpio.xz</code>
as an initramfs.

## Flashing images

After building zephyr.img, copy it to a TFTP server.  Then make sure your
serial ports are set up correctly:

* UART0 is the CM console.  Typically this is broken out on a daughtercard;
the daughtercard is connected to the mainboard with a 14-pin cable.  The
daughtercard should provide either a standard RS232 female DB-9, or a USB
'B' port connected to an FTDI converter chip.
* UART1 is the Linux console.  This might just be a 4-pin header with 3.3V,
ground, and 3.3V TX/RX lines.

Both ports should be configured for 115200 8N1, no flow control.

To flash a new zephyr.img:

1. Wait for the <code>Enter '1', '2', or 'p'</code> prompt on the CM
console while booting
2. Hit 'p' and then set up your IP addresses when prompted
3. For <code>Internal/External phy?</code> you can answer 'a' for "auto"
4. From the menu, choose 'd' for <code>Download and save to flash</code>
5. Enter TFTP server information
6. For <code>Destination image</code>, choose '3'
7. Answer 'y' to "Store parameters to flash"

After the board reboots, it should bring up the Zephyr CPU and you should
see Linux booting on UART1.

If you have [ip2ser](http://ip2ser.sf.net) set up to talk to the CM console
on localhost:2300 and control power to the target board, you can run
[misc/flash-3384.expect](misc/flash-3384.expect) to automate the flashing
process.  Edit the IP addresses at the top of the file to taste.

Sample boot logs: [CM console](misc/cm-log.txt),
[Linux console](misc/linux-log.txt).

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
