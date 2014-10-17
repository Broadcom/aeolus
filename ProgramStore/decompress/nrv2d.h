/*---------------------------------------------------------------------------
    CONFIDENTIAL AND PROPRIETARY
    Copyright (c) 2000, Broadcom Corporation (unpublished)       /\     
    All Rights Reserved.                                  _     /  \     _ 
    _____________________________________________________/ \   /    \   / \_
                                                            \_/      \_/  
    File: NRV2D Decompression H-File from UCL library

    Description: 

    History:
    -------------------------------------------------------------------------
    $Log: nrv2d.h $
    Revision 1.1  2004/09/20 12:47:24  msieweke
    Initial revision
    Revision 1.2  2004/03/03 17:52:35  msieweke
    Modified for one-pass build.
    Revision 1.1  2002/01/31 16:19:01  msieweke
    Initial revision
---------------------------------------------------------------------------*/
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
