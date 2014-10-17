/* n2d_d.c -- implementation of the NRV2D decompression algorithm

   This file is part of the UCL real-time data compression library.

   Copyright (C) 1996-2000 Markus Franz Xaver Johannes Oberhumer

   The UCL library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   The UCL library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the UCL library; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Markus F.X.J. Oberhumer
   <markus.oberhumer@jk.uni-linz.ac.at>
   http://wildsau.idv.uni-linz.ac.at/mfx/ucl.html
 */


#include "nrv2d.h"
int
ucl_nrv2d_decompress( 
    const unsigned char *src, 
    unsigned int src_len,
    unsigned char *dst, 
    unsigned int *dst_len )
{
    unsigned int bb = 0;
    unsigned int ilen = 0, olen = 0, last_m_off = 1;
//    const unsigned int oend = *dst_len;

    for (;;)
    {
        unsigned int m_off, m_len;

        if (getbit(bb, src, ilen))
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
//            fail(olen >= oend, UCL_E_OUTPUT_OVERRUN);
            dst[olen++] = src[ilen++];
            continue;
        }
        m_off = 1;
        for (;;)
        {
            m_off = m_off*2 + getbit(bb, src, ilen);
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            fail(m_off > 0xffffff + 3, UCL_E_LOOKBEHIND_OVERRUN);
            if (getbit(bb, src, ilen)) break;
            m_off = (m_off-1)*2 + getbit(bb, src, ilen);
        }
        if (m_off == 2)
        {
            m_off = last_m_off;
            m_len = getbit(bb, src, ilen);
        }
        else
        {
            fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
            m_off = (m_off-3)*256 + src[ilen++];
            if (m_off == 0xffffffff)
                break;
            m_len = (m_off ^ 0xffffffff) & 1;
            m_off >>= 1;
            last_m_off = ++m_off;
        }
        m_len = m_len*2 + getbit(bb, src, ilen);
        if (m_len == 0)
        {
            m_len++;
            do {
                m_len = m_len*2 + getbit(bb, src, ilen);
                fail(ilen >= src_len, UCL_E_INPUT_OVERRUN);
//                fail(m_len >= oend, UCL_E_OUTPUT_OVERRUN);
            } while (!getbit(bb, src, ilen));
            m_len += 2;
        }
        m_len += (m_off > 0x500);
//        fail(olen + m_len > oend, UCL_E_OUTPUT_OVERRUN);
        fail(m_off > olen, UCL_E_LOOKBEHIND_OVERRUN);
        {
            const unsigned char *m_pos;
            m_pos = dst + olen - m_off;
            dst[olen++] = *m_pos++;
            do dst[olen++] = *m_pos++; while (--m_len > 0);
        }
    }
    *dst_len = olen;
    return ilen == src_len ? UCL_E_OK : (ilen < src_len ? UCL_E_INPUT_NOT_CONSUMED : UCL_E_INPUT_OVERRUN);
}


