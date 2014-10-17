/*
 * Copyright (C) 1997 Broadcom Corporation
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

#ifndef crc_h
#define crc_h

#define PROCESSOR_TYPE MIPS

typedef enum {
   CRC_CCITT = 0,
   CRC_32
} PolyType;

/* This is a very fast, table-based implementation of CRC_32.  You should
   call this instead of compute_crc for CRC_32.  It returns the CRC, rather
   than requiring you to pass tons of arguments. */
unsigned long FastCrc32( const void *pData, 
                         unsigned int numberOfBytes );


int compute_crc( void         *in2_str,
                 PolyType      polynomial_name, 
                 int           Length, 
                 unsigned int *crc_word,
                 int           reverse );
void compute_dec_checksum( const char string[], 
                           int n, 
                           char * ch );

#endif

