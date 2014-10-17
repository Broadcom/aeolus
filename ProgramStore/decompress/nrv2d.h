/*
 * Copyright (C) 2000 Broadcom Corporation
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

#ifndef N2D_D_H

#define N2D_D_H

#define UCL_E_OK                    0
/* decompression errors */
#define UCL_E_INPUT_OVERRUN         (-201)
#define UCL_E_OUTPUT_OVERRUN        (-202)
#define UCL_E_LOOKBEHIND_OVERRUN    (-203)
#define UCL_E_EOF_NOT_FOUND         (-204)
#define UCL_E_INPUT_NOT_CONSUMED    (-205)
#define UCL_E_OVERLAP_OVERRUN       (-206)

#define fail(x,r)   if (x) { *dst_len = olen; return r; }

#define getbit(bb, src, ilen)  (bb*=2,bb&0xff ? (bb>>8)&1 : ((bb=src[ilen++]*2+1)>>8)&1)

#ifdef __cplusplus
extern "C"
{
#endif
int ucl_nrv2d_decompress( const unsigned char *src, unsigned int src_len,
    unsigned char *dst, unsigned int *dst_len );
int ucl_nrv2d_decompress_8( const unsigned char *src, unsigned int src_len,
    unsigned char *dst, unsigned int *dst_len );
#ifdef __cplusplus
}
#endif

#endif
