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

#include "../ProgramStore.h"

#ifndef DECOMPRESS_H
#define DECOMPRESS_H

/*---------------------------------------------------------------------*/
/* General Definitions                                                 */
/*---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/* Symbol Definitions                                                  */
/*---------------------------------------------------------------------*/

// Define bits in the compression mask.
#define COMPRESSION_LZRLE       0x01
#define COMPRESSION_MINILZ      0x02
#define COMPRESSION_Reserved    0x04 // Unused
#define COMPRESSION_NRV2D99     0x08
#define COMPRESSION_LZMA        0x10

#define  PGM_CONTROL_COMPRESSION_BITS              0x0007
#define  PGM_CONTROL_COMPRESSED_IMAGE_LZRW1_KH     0x0001
#define  PGM_CONTROL_COMPRESSED_IMAGE_MINILZO      0x0002
#define  PGM_CONTROL_COMPRESSED_IMAGE_NRV2D99      0x0004
#define  PGM_CONTROL_COMPRESSED_IMAGE_LZMA         0x0005
#define  PGM_CONTROL_DUAL_IMAGES                   0x0100

#define FLAG_Copied		0x80
bool DecompressFile( BcmProgramHeader *pProgramHeader, 
                     void             *pDecompressLoc,
                     int              *pDecompressSize );

#endif
