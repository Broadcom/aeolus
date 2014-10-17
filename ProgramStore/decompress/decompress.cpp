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

#include <stdio.h>
#include "bcmtypes.h"
#include "decompress.h"
#include "crc.h"
#define __INCrc32364h
#define __INCarchMipsh
#include "../minilzo.h"
#include "nrv2d.h"
#include "../7z/7z.h"

//---------------------------------------------------------------------------
// Global variables
//---------------------------------------------------------------------------


static bool verifyCRC( BcmProgramHeader *pProgramHeader );

//---------------------------------------------------------------------------
// Convert an epoch time into a time string and print it.  The epoch time is a 
// count of the seconds since 00:00 January 1, 1970.  The result is in Zulu time.
//---------------------------------------------------------------------------
static void printctime( unsigned int uiTime )
{
    unsigned int    seconds;
    unsigned int    minutes;
    unsigned int    hours;
    unsigned int    days;
    unsigned int    months;
    unsigned int    years               = 1970;
    unsigned int    DaysPerYear[4]      = { 365, 365, 366, 365};
    unsigned int    DaysPerMonth[12]    = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const unsigned int       SecondsPerDay       = 24*60*60;
    const unsigned int       SecondsPerHour      = 60*60;
    unsigned int    SecondsPerMinute    = 60;
    unsigned int    i;
    unsigned int    t;

    // Convert to EST by subtracting 5 hours.  This will be wrong during daylight
    // savings time.
//    uiTime = uiTime = 5 * SecondsPerHour;
    // Find the number of full days.
    days    = uiTime / SecondsPerDay;
    // Find the number of full years.  Use a table of days-per-year to handle
    // leap years in a simple fashion.  Year 0 is 1970, so year 2 has 366 days.
    while ( 1 )
    {
        for ( i = 0; i < 4; i++ )
        {
            t = DaysPerYear[i];
            if ( days >= t )
            {
                days -= t;
                years++;
            }
            else
            {
                break;
            }
        }
        if ( i < 4 )
        {
            break;
        }
    }
    // If this year is a leap year, then February has 29 days.
    if ( (years % 4) == 0 )
    {
        DaysPerMonth[1] = 29;
    }
    // Find the number of full months.  Use a table of days-per-month to handle
    // the different months.
    months = 0;
    while ( 1 )
    {
        t = DaysPerMonth[months];
        if ( days >= t )
        {
            days -= t;
            months++;
        }
        else
        {
            break;
        }
    }
    // Since the tables and all values are zero-based, we have to add 1 to
    // the day and month to get a "normal" value.
    months++;
    days ++;

    // Finding hours, minutes, and seconds is simpler.
    // Find total seconds in the day under consideration.
    seconds = uiTime  % SecondsPerDay;

    // Find hours since midnight.
    hours   = seconds / SecondsPerHour;
    seconds = seconds % SecondsPerHour;

    // Find minutes since start of hour.
    minutes = seconds / SecondsPerMinute;
    seconds = seconds % SecondsPerMinute;

    printf( "%d/%d/%d %02d:%02d:%02d Z\n", years, months, days, hours, minutes, seconds );
}


/*---------------------------------------------------------------------------
    Name: PrintProgramHeader
 Purpose: Display the image header contents
  Inputs:
        BcmProgramHeader - pointer to a program header in memory
 Returns:
        Nothing
   Notes: 
---------------------------------------------------------------------------*/
static void PrintProgramHeader( BcmProgramHeader * pProgramHeader )
{
    printf("   Signature: %04x\n",      pProgramHeader->usSignature);
    printf("     Control: %04x\n",      pProgramHeader->usControl);
    printf("   Major Rev: %04x\n",      pProgramHeader->usMajorRevision);
    printf("   Minor Rev: %04x\n",      pProgramHeader->usMinorRevision);
    printf("  Build Time: ");
    printctime((unsigned int)pProgramHeader->ulcalendarTime);
    printf(" File Length: %ld bytes\n", pProgramHeader->ulTotalCompressedLength);
    printf("Load Address: %08lx\n",     pProgramHeader->ulProgramLoadAddress);
    printf("    Filename: %s\n",          pProgramHeader->cFilename);
    printf("         HCS: %04x\n",      pProgramHeader->usHcs);
    printf("         CRC: %08lx\n\n",     pProgramHeader->ulcrc);
} 


/*---------------------------------------------------------------------------
    Name: Decompression
 Purpose: Decompress an image
  Inputs:
        uint16 - Number of seconds to wait
        char * - String to display while waiting
 Returns:
        unsigned char *Source, - source of data 
        unsigned char * Dest,  - destination of data
        long SourceSize  - length of source data
   Notes: 
        Implements the updated LZRW1/KH algoritm which
        also implements  some RLE coding  which is usefull  when
        compress files  containing  a lot  of consecutive  bytes
        having the same value.   The algoritm is not as good  as
        LZH, but can compete with Lempel-Ziff.
---------------------------------------------------------------------------*/
static long Decompression( uint8 *Source, 
                           uint8 *Dest, 
                           int32  SourceSize )
{
    int32 X, Y, Pos;
    short Command, Size, K;
    unsigned char Bit;
    int32 SaveY;

    if ( Source[0] == FLAG_Copied )
    {
        printf("Copy Only!\n");
        SaveY = 0;
        for ( Y = 1; Y <= (SourceSize - 1); Y++ )
        {
            Dest[Y-1] = Source[Y];
            SaveY = Y;
        }
        Y = SaveY;

    }
    else
    {

        Y = 0;
        X = 3;
        Command = (Source[1] << 8) + Source[2];
        Bit = 16;

        while ( X < SourceSize )
        {
            if ( Bit == 0 )
            {
                Command = (Source[X] << 8) + Source[X+1];
                Bit = 16;
                X += 2;
            }

            if ( (Command & 0x8000) == 0 )
            {
                Dest[Y] = Source[X];
                X++;
                Y++;
            }
            else
            {    // command & 0x8000
                Pos = ( (Source[X] << 4) + (Source[X+1] >> 4) );
                if ( Pos == 0 )
                {
                    Size = (Source[X+1] << 8) + Source[X+2] + 15;

                    for ( K = 0; K <= Size; K++ )
                    {
                        Dest[Y+K] = Source[X+3];
                    }

                    X += 4;
                    Y += Size + 1;
                }
                else
                {    // pos == 0
                    Size = (Source[X+1] & 0x0f) + 2;

                    for ( K = 0; K <= Size; K++ )
                        Dest[Y+K] = Dest[Y-Pos+K];

                    X += 2;
                    Y += Size+1;
                }   //  pos == 0
            }   //  command & 0x8000

            Command = Command << 1;
            Bit--;
        }   // while x < sourcesize
    }

    return( Y );
}   //  decompression


/*---------------------------------------------------------------------------
    Name: DecompressFile
 Purpose: Expand an image into memory and execute it
  Inputs:
        BcmProgramHeader * - Pointer to a program header in memory
        int - The image number
 Returns:
        bool - false if unable to expand/execute image, otherwise this
               function jumps to the Program Load Address and never returns
   Notes: 
---------------------------------------------------------------------------*/
bool DecompressFile( BcmProgramHeader *pProgramHeader, 
                     void             *pDecompressLoc,
                     int              *pDecompressSize )
{
    byte             *pucImage;
    byte             *pucSrc;
    byte             *pucDst = (byte *) pDecompressLoc;
    int32             compressedLength;
    BcmProgramHeader *pDecompressHeader;
    int               compressionType;

    // The header checksum has been verified before we get here, so print the info.
    PrintProgramHeader( pProgramHeader );

    // peform CRC, if passes load this image and execute

    printf( "Performing CRC on Image...\n" );
    if ( verifyCRC(pProgramHeader)==true )
    {

        // Save a pointer to the header.  If this is a dual-header image, the
        // header will change.
        pucImage          = (byte *)((byte *)pProgramHeader+sizeof(BcmProgramHeader));
        pDecompressHeader = pProgramHeader;
        compressedLength  = pProgramHeader->ulTotalCompressedLength;

        compressionType  = pDecompressHeader->usControl & PGM_CONTROL_COMPRESSION_BITS;
        pucSrc           = (byte *)pucImage;

        // We decompress based on the Decompress Header, which may not be the
        // same as the overall header if it's a "dual header" file.  But we
        // pass a pointer to the overall header to the application.

        // compressed image - LZRW1/KH
        if ( compressionType == PGM_CONTROL_COMPRESSED_IMAGE_LZRW1_KH )
        {
            printf("Detected LZRW1/KH compressed image...decompressing...\n");
            *pDecompressSize = Decompression( pucSrc, 
                                  pucDst, 
                                  compressedLength );
            printf("Decompressed length: %ld\n", *pDecompressSize);
        }
        else if ( compressionType == PGM_CONTROL_COMPRESSED_IMAGE_MINILZO )
        {
            // compressed image - MINILZO
            int r;
            printf("Detected MINILZO compressed image...decompressing...\n");

            r = lzo1x_decompress(            pucSrc, 
                                  (uint32)   compressedLength, 
                                             pucDst, 
                                  (uint32 *) pDecompressSize, 
                                             NULL);
            if ( r == LZO_E_OK )
            {
                printf("Decompressed length: %ld\n", *pDecompressSize);
            }
            else
            {
                printf("Decompression failed...\n");
                return false;
            }
        }
        else if ( compressionType == PGM_CONTROL_COMPRESSED_IMAGE_NRV2D99 )
        {
            int r;
            printf("Detected NRV2D99 compressed image... decompressing... \n");

            r = ucl_nrv2d_decompress( (const unsigned char*) pucSrc,
                                      (unsigned int)         compressedLength,
                                      (unsigned char*)       pucDst,
                                      (unsigned int*)        pDecompressSize);
            if ( r == UCL_E_OK )
            {
                printf("Decompressed length: %ld\n", *pDecompressSize);
            }
            else
            {
                printf("Decompression failed... %i\n", r);
                return false;
            }
        }
        else if ( compressionType == PGM_CONTROL_COMPRESSED_IMAGE_LZMA )
        {
            bool r;
            printf("Detected LZMA compressed image... decompressing... \n");

            r = decompress_lzma_7z((unsigned char*) pucSrc,
                                   (unsigned int)   compressedLength,
                                   (unsigned char*) pucDst,
                                                    *pDecompressSize );
            if ( r == true )
            {
                printf("\nDecompressed length unknown.  Padded to %d bytes.\n", *pDecompressSize );
            }
            else
            {
                printf("\nDecompression failed... %i\n", r);
                return false;
            }
        }
        else
        {
            uint32      *pulSrc;
            uint32      *pulDst;
            int         i;

            // uncompressed image
            printf( "Loading non-compressed image...\n" );
//            printf( "Target Address: 0x%08X\n", (UINT) pucDst );
            printf( "Length: %ld\n", compressedLength );
            pulSrc = (uint32 *)pucImage;
            pulDst = (uint32 *)pucDst;
            for ( i=0; i<(compressedLength/4)+1; i++ )
                *pulDst++ = *pulSrc++;
        }

        return true;
    }
    else
    {
        printf( "Image %d CRC failed!\n" );
    }
    return false;
}


/*---------------------------------------------------------------------------
    Name: verifyCRC
 Purpose: Do a CRC calculation on the whole image.
  Inputs:
        BcmProgramHeader * - pointer to the image header
 Returns:
        true/false - if the CRC matches the one in the header
   Notes: 
---------------------------------------------------------------------------*/
static bool verifyCRC( BcmProgramHeader *pProgramHeader ) 
{
    byte   *pucImage;
    uint32  ulCrc;

    pucImage = (byte *)((byte *)pProgramHeader + sizeof(BcmProgramHeader));
//    ulCrc = FastCrc32( pucImage, pProgramHeader->ulTotalCompressedLength );
    compute_crc( pucImage, CRC_32, pProgramHeader->ulTotalCompressedLength, &ulCrc, 0 );

    if ( ulCrc == pProgramHeader->ulcrc )
    {
        return true;
    }
    else
    {
        return false;
    }
}
