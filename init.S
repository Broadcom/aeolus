/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2014 Broadcom Corporation
 */

#include "regdef.h"

	.section .init,"ax"
	.global	_start
	.ent	_start
_start:
	la	t0, 1f
	jr	t0

1:
	bal	mips_init

	la	t0, _fbss
	move	sp, t0

	la	t1, _ebss
	beq	t0, t1, 2f
1:
	sw	zero, 0(t0)
	addiu	t0, 4
	bne	t0, t1, 1b

2:
	la	a0, _linux
	la	a1, _cmdline
	la	a2, _dtb_start
	la	a3, _dtb_end
	subu	a3, a2

	la	t0, main
	jr	t0

	.end	_start
