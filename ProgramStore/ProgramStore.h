/*
 * ProgramStore CM image generator
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

#ifndef PROGRAMSTORE_H
#define PROGRAMSTORE_H

typedef struct _BcmProgramHeader 
	{
	unsigned short usSignature; 	// the unique signature may be specified as a command
									// line option: The default is: 0x3350

	unsigned short usControl;		// Control flags: currently defined lsb=1 for compression
									// remaining bits are currently reserved

	unsigned short usMajorRevision; // Major SW Program Revision
	unsigned short usMinorRevision; // Minor SW Program Revision
									// From a command line option this is specified as xxx.xxx
									// for the Major.Minor revision (note: Minor Revision is 3 digits)

	unsigned long ulcalendarTime;	// calendar time of this build (expressed as seconds since Jan 1 1970)

	unsigned long ulTotalCompressedLength;	// length of Program portion of file

	unsigned long ulProgramLoadAddress; // Address where program should be loaded (virtual, uncached)

	char cFilename[48]; 			// NULL terminated filename only (not pathname)
    char pad[8];                    // For future use

    unsigned long ulCompressedLength1; // When doing a dual-compression for Linux,
    unsigned long ulCompressedLength2; // it's necessary to save both lengths.

	unsigned short usHcs;			// 16-bit crc Header checksum (CRC_CCITT) over the header [usSignature through cFilename]
	unsigned short reserved;		// reserved
	unsigned long ulcrc;			// CRC-32 of Program portion of file (following the header)

	} BcmProgramHeader;




#endif
