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

#include "regdef.h"

	.section .mipsinit,"ax"
	.global	mips_init
	.ent	mips_init
mips_init:

	/* run the cache init from kseg1 */
	la	k0, 1f
	li	k1, 0x20000000
	or	k0, k1
	jr	k0
1:

	/* set up CP0 STATUS; enable FPU */
	li	k0, 0x30000000
	mtc0	k0, CP0_STATUS
	BARRIER

	/* set local CP0 CONFIG to make kseg0 cacheable, write-back */
	mfc0	k0, CP0_CONFIG
	ori	k0, 0x07
	xori	k0, 0x04
	mtc0	k0, CP0_CONFIG

	/* initialize CPU1's local I-cache */
	li	k0, 0x80000000
	li	k1, 0x80010000
	mtc0	zero, $28
	mtc0	zero, $28, 1
	BARRIER

1:	cache	Index_Store_Tag_I, 0(k0)
	addiu	k0, 16
	bne	k0, k1, 1b

	jr	ra

	.end	mips_init
