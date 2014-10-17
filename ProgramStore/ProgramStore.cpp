//****************************************************************************
//
// Copyright (c) 2000-2014 Broadcom Corporation
//
// This program is the proprietary software of Broadcom Corporation and/or
// its licensors, and may only be used, duplicated, modified or distributed
// pursuant to the terms and conditions of a separate, written license
// agreement executed between you and Broadcom (an "Authorized License").
// Except as set forth in an Authorized License, Broadcom grants no license
// (express or implied), right to use, or waiver of any kind with respect to
// the Software, and Broadcom expressly reserves all rights in and to the
// Software and all intellectual property rights therein.  IF YOU HAVE NO
// AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY,
// AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE
// SOFTWARE.
//
// Except as expressly set forth in the Authorized License,
//
// 1.     This program, including its structure, sequence and organization,
// constitutes the valuable trade secrets of Broadcom, and you shall use all
// reasonable efforts to protect the confidentiality thereof, and to use this
// information only in connection with your use of Broadcom integrated circuit
// products.
//
// 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
// "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS
// OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
// RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
// IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR
// A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
// ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME
// THE ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
//
// 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM
// OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
// INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
// RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
// HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN
// EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1,
// WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY
// FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
//
//****************************************************************************
//
//    Filename:      ProgramStore.cpp
//    Author:        Kevin Peterson
//    Creation Date: 17-Apr-2000
//
//**************************************************************************
//    Description:
//
//		Cablemodem V2 code
//
//    Program file download/storage
//
//**************************************************************************

#include "StdAfx.h"

#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "ProgramStore.h"
#include "minilzo.h"
#include "ucl.h"
#include "7z/7z.h"
#include "decompress/decompress.h"

#define ProgramstoreRevision "1.0.11"

#define NoCompression         0x0000
#define UseLZCompression      0x0001
#define UseminiLZOCompression 0x0002
#define ReservedCompression   0x0003  // Skipped
#define UseNRV2D99Compression 0x0004
#define UseLZMACompression    0x0005
#define UseDualFiles          0x0100

// Command line arguments (in alphabetical order)
#define CLI_ARG_LOAD_ADDRESS    'a'
#define CLI_ARG_COMPRESSION     'c'
#define CLI_ARG_DEBUG           'd'
#define CLI_ARG_FILENAME        'f'
#define CLI_ARG_OUTPUT_FILENAME 'o'
#define CLI_ARG_PAD             'p'
#define CLI_ARG_REVISION        'r'
#define CLI_ARG_SIGNATURE       's'
#define CLI_ARG_TIMESTAMP       't'
#define CLI_ARG_VERSION         'v'
#define CLI_ARG_DECOMPRESSION   'x'

int nrv2d99_compression_level  = 10;  // Min=1  Max=10
int lzma_compression_level = 1;       // Min=1  Max=?
int lzma_algo_g      = 0;
int lzma_dictsize_g  = 0;
int lzma_fastbytes_g = 0;

typedef enum
{
    CRC_CCITT = 0,
    CRC_32,
} PolyType;


int compute_crc( void          *in2_str,
                 PolyType       polynomial_name,
                 int            Length,
                 unsigned int  *crc_word,
                 int            reverse);
void compute_dec_checksum(char  string[],
                          int   n,
                          char *ch);
void byte_swap( unsigned char *inbytes,
                int            numbytes);

//FUNCTION  Compression    (    Source,Dest    : BufferPtr;
//                              SourceSize     : BufferSize   )    : BufferSize;
long Compression( unsigned char *Source,
                  unsigned char *Dest,
                  long           BufferSize );

//FUNCTION  Decompression  (    Source,Dest    : BufferPtr;
//                              SourceSize     : BufferSize   )    : BufferSize;
long Decompression( unsigned char *Source,
                    unsigned char *Dest,
                    long           BufferSize );

#define HEAP_ALLOC(var,size) \
	long __LZO_MMODEL var [ ((size) + (sizeof(long) - 1)) / sizeof(long) ]


static HEAP_ALLOC(wrkmem,LZO1X_1_MEM_COMPRESS);
static void print_help(char * source);
static void fail( const char * ErrorString );
static unsigned int swap2bytes( unsigned int inValue );
static unsigned long int swap4bytes( unsigned long int inValue );
static unsigned long CompressFile( FILE *infile,
                                   char *indata,
                                   unsigned long infilelength,
                                   char *compdata,
                                   unsigned long complength,
                                   int   compressionType );

#define MAX_FILENAME_LEN 256

int main(int argc, char* argv[])
{
    char            *indata;
    char            *compdata;
    unsigned long    compdatasize;
    char             infilename1[MAX_FILENAME_LEN+1];
    char             infilename2[MAX_FILENAME_LEN+1];
    char             outfilename[MAX_FILENAME_LEN+1];
    FILE            *infile1;
    FILE            *infile2;
    unsigned long    filelength1;
    unsigned long    filelength2;
    unsigned long    complength1;
    unsigned long    complength2;
    unsigned long    totalLength;
    int              i;
    int              padParameter = 0;
    int              padLength;
    bool             debug_on = false;
    bool             dual_file = false;
    BcmProgramHeader proghead;
    FILE            *outfile;
    unsigned int     crc_word;
    long             opMode = 5;

    // Clear the header ...
    memset( &proghead, 0, sizeof(BcmProgramHeader) );
    infilename1[0] = 0;
    infilename2[0] = 0;
    outfilename[0] = 0;

    // Set default signature.
    proghead.usSignature = 0x3350;

    // Get the current time and insert it into the header.
    proghead.ulcalendarTime = (unsigned long)time( NULL );

    /*
    * Step 1: initialize the LZO library if we use it
    */
    if (lzo_init() != LZO_E_OK)
    {
        printf("lzo_init() failed !!!\n");
        return 3;
    }

    // Process the command line arguments in pairs as "-f filename"
    if (argc > 1)
    {
        for (i = 1; i < argc; i++)
        {
            if (*(argv[ i ]) == '-')
            {
                switch ((argv[ i ])[ 1 ])
                {
                    case CLI_ARG_DEBUG :
                        // debug on to dump header contents ...
                        debug_on = true;

                        break;

                    case CLI_ARG_FILENAME :
                    {
                        char * filevar = infilename1;
                        if (argv[i][2] == '2')
                        {
                            filevar = infilename2;
                            dual_file = true;
                        }
                        if (++i < argc)
                        {
                            strncpy( filevar, argv[i], MAX_FILENAME_LEN );
                            filevar[MAX_FILENAME_LEN] = '\0';
                        }
                        else
                        {
                            fail( "Missing filename for -f option, cannot continue!!\n\r" );
                        }
                        break;
                    }

                    case CLI_ARG_OUTPUT_FILENAME :
                        if (++i < argc)
                        {
                            strncpy( outfilename, argv[i], MAX_FILENAME_LEN );
                            outfilename[MAX_FILENAME_LEN] = '\0';
                        }
                        else
                        {
                            fail( "Missing filename for -o option\n\r" );
                        }

                        break;

                    case CLI_ARG_VERSION :
                        if (++i < argc)
                        {
                            if ((argv[i][3] == '.') || (argv[i][4] == '.'))
                            {
                                sscanf( argv[i], "%4hx.%4hx", &(proghead.usMajorRevision), &(proghead.usMinorRevision) );
                            }
                            else
                            {
                                fail( "Version must be xxxx.xxxx.\n\r" );
                            }
                        }
                        else
                        {
                            fail( "Missing version for -v option.\n\r" );
                        }

                        break;

                    case CLI_ARG_COMPRESSION :

                        // Turn on compression

                        // Check to make sure that there is another arguement before doing check
                        if ((++i) < argc)
                        {
                            if (argv[i][0] == '0')
                            {
                                printf("Image will not be compressed\n");
                                proghead.usControl = NoCompression;
                            }
                            else if (argv[i][0] == '1')
                            {
                                printf("Using standard RLE/LZ Compression\n");
                                proghead.usControl = UseLZCompression;
                            }
                            else if (argv[i][0] == '2')
                            {
                                printf("Using miniLZ Compression\n");
                                proghead.usControl = UseminiLZOCompression;
                            }
                            else if (argv[i][0] == '3')
                            {
                                printf("Using NRV2D99 Compression.\n");
                                if ((argv[i][1] >= '1') && (argv[i][1] <= '9'))
                                {
                                    nrv2d99_compression_level = argv[i][1] - '0';
                                }
                                else
                                {
                                    nrv2d99_compression_level = 10;
                                    printf("Please be patient, this compression can take up to 2 minutes!\n");
                                }
                                proghead.usControl = UseNRV2D99Compression;
                            }
                            else if (argv[i][0] == '4')
                            {
                                printf("Using LZMA Compression.\n");
                                // Undocumented feature: Allow the user to specify
                                // full LZMA options with -c4.x.y.z, where
                                //   x = algorithm (0/1/2)
                                //   y = dictionary size (usually 256K to 16M)
                                //   z = fastbytes (3-255, where 3 is fastest, 255 is slowest)
                                if ( argv[i][1] == '.' )
                                {
                                    sscanf( &argv[i][2], "%d.%d.%d", &lzma_algo_g, &lzma_dictsize_g, &lzma_fastbytes_g );
                                }
                                else if ((argv[i][1] >= '0') && (argv[i][1] <= '9'))
                                {
                                    lzma_compression_level = argv[i][1] - '0';
                                }
                                proghead.usControl = UseLZMACompression;
                            }
                            // This is only here for temporary compatibility.  Delete any time.
                            else if (argv[i][0] == '6')
                            {
                                printf("Using NRV2D99 Compression.\n");
                                proghead.usControl = UseNRV2D99Compression;
                                nrv2d99_compression_level = 6;
                            }
                            else
                            {
                                fail("Invalid compression type!!!!\n");
                            }
                        }
                        else
                        {
                            fail("Missing compression type!!!!\n");
                        }

                        opMode = 1;

                        break;

                    case CLI_ARG_DECOMPRESSION :
                        // Turn on decompression
                        opMode = 0;

                        break;

                    case CLI_ARG_SIGNATURE :
                        if (++i < argc)
                        {
                            sscanf( argv[i], "%hx", &(proghead.usSignature) );
                        }
                        else
                        {
                            fail( "Missing signature for the -s option.\n\r" );
                        }
                        break;

                    case CLI_ARG_TIMESTAMP :
                        if (++i < argc)
                        {
                            // Read in the time as an ulong representing time since epoch
                            //   as from `date +%s`
                            //   e.g. 1406750554  // represents Wed Jul 30 20:02:34 UTC 2014
                            sscanf( argv[i], "%lu", &(proghead.ulcalendarTime) );
                        }
                        else
                        {
                            fail ( "Missing timestamp for the -t option.\n\r" );
                        }
                        break;

                    case CLI_ARG_LOAD_ADDRESS :
                        if (++i < argc)
                        {
                            sscanf( argv[i], "%lx", &(proghead.ulProgramLoadAddress) );
                        }
                        else
                        {
                            fail( "Missing load address for the -a option.\n\r" );
                        }
                        break;

                    case CLI_ARG_REVISION :
                        printf( "Revision %s\n\r", ProgramstoreRevision );
                        break;

                    case CLI_ARG_PAD :
                        if (++i < argc)
                        {
                            if ( sscanf( argv[i], "%i", &padParameter ) == 0 )
                            {
                                fail( "Invalid pad parameter for -p option.\n\r" );
                            }
                        }
                        else
                        {
                            fail( "Missing pad parameter for -p option.\n\r" );
                        }
                        break;

                    default :
                        printf( "Unknown command line option: %s\n\r", argv[i] );
                        exit(1);
                        break;
                };
            }
            else
            {
                printf( "Unknown command line option(ignored): %s\n\r", argv[i] );
                exit(1);
            }
        }
    }
    else
    {
        print_help(argv[0]);
        exit(1);
    }

    // Compression mode
    if (opMode == 1)
    {
        // Did we get the required filename argument?  One name is required for
        // "normal" mode and two for dual mode.
        if ((infilename1[0] == '\0') ||
            ((dual_file == true) && (infilename2[0] == '\0' )))
        {
            print_help(argv[0]);
            return(1);
        }
        // Open input file 1.
        if (( infile1 = fopen( infilename1, "rb" ) ) == NULL)
        {
            printf( "Failed to open input file %s\n\r", infilename1 );
            return(1);
        }
        // If needed, open input file 2.
        if (dual_file)
        {
            if (( infile2 = fopen( infilename2, "rb" ) ) == NULL)
            {
                printf( "Failed to open input file %s\n\r", infilename2 );
                fclose( infile1 );
                return(1);
            }
        }
        // Make sure that we got an output filename
        if (outfilename[0] == 0)
        {
            // No outfile name generated, so create one.
            for (i = 0; i < MAX_FILENAME_LEN-4; i++)
            {
                outfilename[i] = infilename1[i];

                if ((outfilename[i] == '.') || (outfilename[i] == 0))
                {
                    strcpy( &(outfilename[i]), ".out" );

                    break;
                }
            }
            printf( "No output file name specified.  Using %s.\n", outfilename );
        }

        // Open the output file.
        if (( outfile = fopen( outfilename, "wb" ) ) == NULL)
        {
            printf( "Failed to open output file %s\n\r", outfilename );
            fclose( infile1 );
            fclose( infile2 );
            return(1);
        }

        // Determine the length of input file 1.
        fseek( infile1, 0, SEEK_END );
        filelength1 = ftell( infile1 );
        rewind( infile1 );

        filelength2 = 0;
        if (dual_file)
        {
            // Determine the length of input file 2.
            fseek( infile2, 0, SEEK_END );
            filelength2 = ftell( infile2 );
            rewind( infile2 );
        }

        // Get a buffer to suck the entire input file into.
        // Get more than necessary just so we know we have enough
        indata = (char *)calloc( (filelength1 + filelength2 + 1024), 1 );
        if (indata == NULL)
        {
            fclose( infile1 );
            if (dual_file)
            {
                fclose( infile2 );
            }
            fclose( outfile );
            fail( "Failed to Calloc a buffer large enough to process file.\n\r" );
        }

        // Allocate a buffer large enough to hold the uncompressed files, in case
        // the "compression" function expands the data.
        compdatasize = filelength1 + filelength2 + 32*1024;
        compdata = (char *)calloc( compdatasize, 1 );
        if (compdata == NULL)
        {
            fail( "Error allocating compression buffer\n\r" );
        }

        complength1 = CompressFile( infile1, indata, filelength1, compdata, compdatasize, proghead.usControl );

        padLength = 0;
        if (padParameter != 0)
        {
            padLength = (padParameter - ((sizeof(BcmProgramHeader) + complength1) % padParameter)) % padParameter;
        }

        complength2 = 0;
        compdatasize -= (complength1 + padLength);
        if (dual_file)
        {
            complength2 = CompressFile( infile2, indata, filelength2, compdata + complength1 + padLength, compdatasize, proghead.usControl );
        }

        totalLength                      = complength1 + padLength + complength2;
        proghead.ulTotalCompressedLength = totalLength;

        // If the dual-file option isn't being used, we can use a longer file name.
        strncpy( proghead.cFilename, outfilename, 48 );
        proghead.cFilename[47] = '\0';

        if (dual_file)
        {
            proghead.usControl |= UseDualFiles;
            proghead.ulCompressedLength1 = complength1;
            proghead.ulCompressedLength2 = complength2;
            byte_swap( (unsigned char *)&(proghead.ulCompressedLength1), 4 );
            byte_swap( (unsigned char *)&(proghead.ulCompressedLength2), 4 );
            proghead.cFilename[47] = '\0';
        }

        // Swap the byte ordering in the header for proper CRC calculation and
        //   in preparation for writing into output file.
        byte_swap( (unsigned char *)&(proghead.usSignature), 2 );
        byte_swap( (unsigned char *)&(proghead.usControl), 2 );
        byte_swap( (unsigned char *)&(proghead.usMajorRevision), 2 );
        byte_swap( (unsigned char *)&(proghead.usMinorRevision), 2 );
        byte_swap( (unsigned char *)&(proghead.ulcalendarTime), 4 );
        byte_swap( (unsigned char *)&(proghead.ulTotalCompressedLength), 4 );
        byte_swap( (unsigned char *)&(proghead.ulProgramLoadAddress), 4 );
        // Compute the CRC for the header
        compute_crc( &proghead, CRC_CCITT, sizeof(BcmProgramHeader) - 8, &crc_word, 0 );
        proghead.usHcs = (unsigned short)( crc_word & 0xffff );
        byte_swap( (unsigned char *)&(proghead.usHcs), 2 );

        // Compute the CRC for the payload part of the file.
        compute_crc( compdata, CRC_32, totalLength, &crc_word, 0 );
        proghead.ulcrc = crc_word;
        byte_swap( (unsigned char *)&(proghead.ulcrc), 4 );

        if (debug_on)
        {
            // Dump the data back to the screen for feedback to user.
            printf( "\n\rHeader info\n\r" );
            printf(     "===========\n\r" );
            printf( "Signature:     0x%x\n\r",   swap2bytes(proghead.usSignature) );
            printf( "Control:       0x%x\n\r",   swap2bytes(proghead.usControl) );
            printf( "MajorRevision: 0x%02x\n\r", swap2bytes(proghead.usMajorRevision) );
            printf( "MinorRevision: 0x%02x\n\r", swap2bytes(proghead.usMinorRevision) );
            printf( "CalendarTime:  %ld\n\r",    swap4bytes(proghead.ulcalendarTime) );
            printf( "Filelength:    %ld\n\r",    swap4bytes(proghead.ulTotalCompressedLength) );
            printf( "LoadAddress:   0x%lx\n\r",  swap4bytes(proghead.ulProgramLoadAddress) );
            printf( "Filename:      %s\n\r",     proghead.cFilename );
            printf( "Hcs:           0x%x\n\r",   swap2bytes(proghead.usHcs) );
            printf( "reserved:      0x%x\n\r",   swap2bytes(proghead.reserved) );
            printf( "crc:           0x%lx\n\r\n\r",  swap4bytes(proghead.ulcrc) );

            printf( "infilename1:   %s\n\r",     infilename1 );
            if (dual_file)
            {
                printf( "infilename2:   %s\n\r",     infilename2 );
            }
        }

        // Write the header into the output file.
        fwrite( &proghead, sizeof(BcmProgramHeader), 1, outfile );

        // Write the payload into the output file.
        fwrite( compdata, 1, totalLength, outfile );


        // Success!!!


        // Free the data compression buffer.
        free( compdata );

        // Free the data buffer.
        free( indata );

        // Close the output files.
        fclose( outfile );

        // Close the input files.
        fclose( infile1 );

        if (dual_file)
        {
            fclose( infile2 );
        }
    }  // End of compression

    // Decompression
    else if (opMode == 0)
    {
        // Did we get the required filename argument?  One name is required for
        // "normal" mode and two for dual mode.
        if (infilename1[0] == '\0')
        {
            print_help(argv[0]);
            return(1);
        }
        // Open input file 1.
        if (( infile1 = fopen( infilename1, "rb" ) ) == NULL)
        {
            printf( "Failed to open input file %s\n\r", infilename1 );
            return(1);
        }
        // Make sure that we got an output filename
        if (outfilename[0] == 0)
        {
            // No outfile name generated, so create one.
            for (i = 0; i < MAX_FILENAME_LEN-4; i++)
            {
                outfilename[i] = infilename1[i];

                if ((outfilename[i] == '.') || (outfilename[i] == 0))
                {
                    strcpy( &(outfilename[i]), ".out" );

                    break;
                }
            }
            printf( "No output file name specified.  Using %s.\n", outfilename );
        }

        // Open the output file.
        if (( outfile = fopen( outfilename, "wb" ) ) == NULL)
        {
            printf( "Failed to open output file %s\n\r", outfilename );
            fclose( infile1 );
            return(1);
        }

        // Determine the length of input file 1.
        fseek( infile1, 0, SEEK_END );
        filelength1 = ftell( infile1 );
        rewind( infile1 );

        // Get a buffer to suck the entire input file into.
        // Get more than necessary just so we know we have enough
        indata = (char *) calloc( (filelength1 + 1024), 1 );
        if (indata == NULL)
        {
            fclose( infile1 );
            fclose( outfile );
            fail( "Failed to Calloc a buffer large enough to process file.\n\r" );
        }

        // Allocate a buffer large enough to hold the uncompressed file.  Assume
        // we won't have more than 6x compression.
        // the "compression" function expands the data.
        compdata = (char *) calloc( (filelength1 * 6), 1 );
        if (compdata == NULL)
        {
            fail( "Error allocating decompression buffer\n\r" );
        }

        int numRead;
        // Read the file into the buffer.
        numRead = fread( indata, 1, filelength1, infile1 );
        if (numRead == 0)
        {
            fail( "Fread failed to get all the data from the file.\n\r" );
        }

        // The first part of the file should be a programstore header.  The
        // modem is big-endian and the PC is little-endian, so the data needs
        // to be byte-swapped for correct interpretation.
        BcmProgramHeader *ph = (BcmProgramHeader *) indata;

        unsigned int ulCrc;
        // Compute a checksum on the header and see if it matches.
        compute_crc( ph, CRC_CCITT, sizeof(BcmProgramHeader)-8, &ulCrc, false );
        ulCrc &= 0x0000ffff;

//        printf("Computed HCS %4x\n\n", ulCrc );

        if ( ulCrc != swap2bytes( ph->usHcs ) )
        {
            fail( "Header checksum failed.\n\r" );
        }

        byte_swap( (unsigned char *)&(ph->usSignature), 2 );
        byte_swap( (unsigned char *)&(ph->usControl), 2 );
        byte_swap( (unsigned char *)&(ph->usMajorRevision), 2 );
        byte_swap( (unsigned char *)&(ph->usMinorRevision), 2 );
        byte_swap( (unsigned char *)&(ph->ulcalendarTime), 4 );
        byte_swap( (unsigned char *)&(ph->ulTotalCompressedLength), 4 );
        byte_swap( (unsigned char *)&(ph->ulCompressedLength1), 4 );
        byte_swap( (unsigned char *)&(ph->ulCompressedLength2), 4 );
        byte_swap( (unsigned char *)&(ph->ulProgramLoadAddress), 4 );
        byte_swap( (unsigned char *)&(ph->usHcs), 2 );
        byte_swap( (unsigned char *)&(ph->ulcrc), 4 );

        int outfilelength = filelength1 * 6;
        DecompressFile( (BcmProgramHeader *) indata, (void *) compdata, &outfilelength );

        // Write the payload into the output file.
        fwrite( compdata, 1, outfilelength, outfile );

    }

    return 0;
}


static unsigned long CompressFile( FILE *infile,
                                   char *indata,
                                   unsigned long infilelength,
                                   char *compdata,
                                   unsigned long compdatasize,
                                   int   compressionType )
{
    unsigned long complength;
    int           numRead;
    int           result;

    /* The compressdata size for the in/out argument */
    complength = compdatasize;
    // Read the file into the buffer.
    numRead = fread( indata, 1, infilelength, infile );
    if (numRead == 0)
    {
        fail( "Fread failed to get all the data from the file.\n\r" );
    }
    // Compress data if buffer space allows.
    if (compressionType == UseLZCompression)
    {
        complength = Compression( (unsigned char *)indata,
                                  (unsigned char *)compdata,
                                                   infilelength );
    }
    else if (compressionType == UseminiLZOCompression)
    {
        result = lzo1x_1_compress((unsigned char*)indata,
                                             infilelength,
                             (unsigned char*)compdata,
                             (unsigned int *)&complength,
                                             wrkmem);
        if (result != LZO_E_OK)
        {
            /* this should NEVER happen */
            printf("internal error - compression failed: %d\n", result);
            exit(2);
        }
    }
    else if (compressionType == UseNRV2D99Compression)
    {
        result = ucl_nrv2d_99_compress((unsigned char *)indata,
                                  infilelength,
                                  (unsigned char*)compdata,
                                  (unsigned int *)&complength,
                                  0,
                                  nrv2d99_compression_level,
                                  NULL,
                                  NULL);
        if (result != UCL_E_OK)
        {
            /* this should NEVER happen */
            printf("internal error - compression failed: %d\n", result);
            exit(2);
        }
    }
    else if (compressionType == UseLZMACompression)
    {
        unsigned lzma_algo;
        unsigned lzma_dictsize;
        unsigned lzma_fastbytes;
        bool     bresult;

        if ( lzma_fastbytes_g != 0 )
        {
            lzma_algo      = lzma_algo_g;
            lzma_dictsize  = lzma_dictsize_g;
            lzma_fastbytes = lzma_fastbytes_g;
        }
        else
        // The parsing code accepts values from 0-9.  1 is the default.
        switch (lzma_compression_level)
        {
            case 1 :
                lzma_algo = 1;
                lzma_dictsize = 1 << 20;
                lzma_fastbytes = 64;
                break;
            case 2 :
                lzma_algo = 1;
                lzma_dictsize = 16 * 1024;
                lzma_fastbytes = 64;
                break;
            case 3 :
                lzma_algo = 1;
                lzma_dictsize = 32 * 1024;
                lzma_fastbytes = 64;
                break;
            case 4 :
                lzma_algo = 2;
                lzma_dictsize = 1 << 22;
                lzma_fastbytes = 128;
                break;
            case 5 :
                lzma_algo = 2;
                lzma_dictsize = 1 << 24;
                lzma_fastbytes = 255;
                break;
            default :
                fail( "Invalid LZMA compression level." );
        }

        bresult = compress_lzma_7z((const unsigned char*) indata,
                                   (unsigned)             infilelength,
                                   (unsigned char*)       compdata,
                                   (unsigned &)           complength,
                                   lzma_algo,
                                   lzma_dictsize,
                                   lzma_fastbytes);
        if (bresult != true)
        {
            /* this should NEVER happen */
            fail("Internal error - LZMA compression failed.");
        }
    }
    // no Compression
    else
    {
        printf("Adding Program Header to start of the original image.\n");
        complength = infilelength;
        memcpy( compdata, (unsigned char *)indata, complength );
    }

    if (compressionType != NoCompression)
    {
        /* check for an incompressible block */
        if (complength >= infilelength)
        {
            printf("WARNING:This block contains incompressible data.\n");
        }
        printf( "before compression: %ld  after compression: %ld\n\r",
                infilelength, complength );
        printf("Percent Compression = %.2f\n\r",(float)((float)(infilelength - complength)/(float)infilelength)*(float)100);
    }
    return complength;
}


static void fail( const char * ErrorString )
{
    printf( ErrorString );
    exit(1);
}


static void print_help(char * source){
    printf( "usage: %s -f infile1 [-f2 infile2] [-o outfilename] [-v xxx.xxx] [-c x] [-x] [-s sig] [-t time] [-a addr]\n\r", source );
    printf( "       -f    -- specify the input filename    (required)\n\r" );
    printf( "       -f2   -- specify second input filename (for dual image)\n\r" );
    printf( "       -o    -- specify the output filename   (default input filename changed to .out)\n\r" );
    printf( "       -v    -- specify the version           (default 000.000)\n\r" );
    printf( "       -c 1  -- use old compression           (default)\n\r" );
    printf( "       -c 2  -- use miniLZO compression\n\r" );
    printf( "       -c 3  -- use NRV2D99 compression       (slowest and best)\n\r" );
    printf( "       -c 3x -- (x=1-9) NRV2D99 compression   (faster, but less compression)\n\r" );
    printf( "       -c 4  -- use LZMA compression\n\r" );
    printf( "       -s    -- specify the signature         (default 0x3350)\n\r" );
    printf( "       -t    -- specify the build time as integer time since epoch (default is current time)\n\r");
    printf( "       -a    -- specify the Load Address      (default 0x00000000)\n\r" );
    printf( "       -r    -- print revision information\n\r" );
    printf( "       -p    -- pad image 1 to n bytes to align image 2 (-f2)\n\r" );
    printf( "       -x    -- decompress the input file\n\r" );
}

// CRC computation routine
int compute_crc( void         *in2_str,
                 PolyType      polynomial_name,
                 int           Length,
                 unsigned int *crc_word,
                 int           reverse )
{
    int hex_index = 0;
    int old_hex_index = 0;
    int stop = 0;
    unsigned long value = 0;
    int digits = 0;
    int shift_count = 0;
    int max_shift_count = 0;
    unsigned long rvalue = 0;
    unsigned long data_word = 0;
    unsigned long polynomial_value = 0x00000000;
    unsigned long xor_mask = 0x00000000;
    unsigned char *DataPtr;
    int Counter;
    int i;

    DataPtr = (unsigned char *)in2_str;
    *crc_word = 0xffffffff;

    if (polynomial_name == CRC_CCITT)    // Header checksum
    {
        digits = 4;
        polynomial_value = 0x00001021;
    }
    else if (polynomial_name == CRC_32)  // Ethernet CRC
    {
        digits = 8;
        polynomial_value = 0x04c11db7;
    }
    else
    {
        return( -1 );
    }

    max_shift_count = digits * 4;

    Counter = 0;

    // Read in hex digits :
    while ((Counter < Length) || !stop)
    {
        if (Counter < Length)
            value = *DataPtr;
        else
        {
            stop = 1;
            value = 0;
            data_word = data_word << ( (digits - (hex_index%digits)) * 4 - 8 );
            max_shift_count = (hex_index % digits) * 4;
            old_hex_index = hex_index;
            hex_index = digits - 2;
        }

        if (reverse)
        {
            rvalue = 0;
            for (i = 0; i < 8; i++)
            {
                if (value & 0x01)
                    rvalue = rvalue | 0x01;
                value = value >> 1;
                rvalue = rvalue << 1;
            }
            rvalue = rvalue >> 1;
            value = rvalue;
        }

        data_word = data_word << 8;
        data_word += value;

        if (hex_index % digits == ( digits - 2 ))
        {
            for (shift_count = 0; shift_count < max_shift_count; shift_count++)
            {
                xor_mask = ( (data_word & (1L << (digits*4-1))) ^
                             (*crc_word & (1L << (digits*4-1))) ) ? polynomial_value : 0L;

                *crc_word = *crc_word << 1;
                *crc_word = *crc_word ^ xor_mask;

                data_word = data_word << 1;
            }
        }

        if (!stop)
        {
            hex_index += 2;
            DataPtr++;
            Counter++;
        }
        else
        {
            hex_index = old_hex_index;
        }

    }

    *crc_word = ~(*crc_word);
    value = *crc_word;

    if (reverse)
    {
        rvalue = 0;
        for (i = 0; i < 32; i++)
        {
            if (value & 0x01)
                rvalue = rvalue | 0x01;
            value = value >> 1;
            if (i != 31)
                rvalue = rvalue << 1;
        }
        *crc_word = (unsigned long) ((rvalue & 0xFF000000)) >> 24 |
                    (unsigned long) ((rvalue & 0x00FF0000) >> 8) |
                    (unsigned long) ((rvalue & 0x0000FF00) << 8) |
                    (unsigned long) ((rvalue & 0x000000FF) << 24);
    }

    return( 0 );

}


//		compute_dec_checksum
//
//    Computes a simple decimal checksum over the input
//		string - checksum character is return in ch
//
void compute_dec_checksum( char string[], int n, char * ch )
{
    char c;
    int j,k=0,m=0;
    static int ip[10][8] = { 0,1,5,8,9,4,2,7,1,5, 8,9,4,2,7,0,2,7,0,1,
        5,8,9,4,3,6,3,6,3,6, 3,6,4,2,7,0,1,5,8,9, 5,8,9,4,2,7,0,1,6,3,
        6,3,6,3,6,3,7,0,1,5, 8,9,4,2,8,9,4,2,7,0, 1,5,9,4,2,7,0,1,5,8};
    static int ij[10][10] = { 0,1,2,3,4,5,6,7,8,9, 1,2,3,4,0,6,7,8,9,5,
        2,3,4,0,1,7,8,9,5,6, 3,4,0,1,2,8,9,5,6,7, 4,0,1,2,3,9,5,6,7,8,
        5,9,8,7,6,0,4,3,2,1, 6,5,9,8,7,1,0,4,3,2, 7,6,5,9,8,2,1,0,4,3,
        8,7,6,5,9,3,2,1,0,4, 9,8,7,6,5,4,3,2,1,0};
    // Group multiplication and permutation tables.

    for (j = 0; j < n; j++)
    {                 // look at successive characters
        c = string[j];
        if (c >= 48 && c <= 57)         // ignore everything except digits
            k = ij[k][ip[(c+2) % 10][7 & m++]];
    }

    for (j = 0; j <= 9; j++)   // find which appended digit will check properly
        if (!ij[k][ip[j][m & 7]])
            break;
    *ch = j + 48;   // convert to ascii
}


// Routine to swap 2 or 4 bytes from intel ordering to "inter-machine(motorola)" ordering.
void byte_swap( unsigned char *inbytes, int numbytes)
{
    unsigned char temp_char;

    switch (numbytes)
    {
        case 2 :
            temp_char = inbytes[1];
            inbytes[1] = inbytes[0];
            inbytes[0] = temp_char;
            break;
        case 4 :
            temp_char = inbytes[3];
            inbytes[3] = inbytes[0];
            inbytes[0] = temp_char;
            temp_char = inbytes[1];
            inbytes[1] = inbytes[2];
            inbytes[2] = temp_char;
            break;
    };
}

static unsigned int swap2bytes( unsigned int inValue )
{
    return ((inValue & 0xff) << 8) | ((inValue & 0xff00) >> 8);
}

static unsigned long int swap4bytes( unsigned long int inValue )
{
    return (((inValue)&0xff)<<24) + (((inValue)&0xff00)<<8) +
           (((inValue)&0xff0000)>>8) + (((inValue)&0xff000000)>>24);
}

//**************************************************************************
//
//    Copyright 2000 Broadcom Corporation
//    All Rights Reserved
//    No portions of this material may be reproduced in any form without the
//    written permission of:
//             Broadcom Corporation
//             16251 Laguna Canyon Road
//             Irvine, California  92618
//    All information contained in this document is Broadcom Corporation
//    company private, proprietary, and trade secret.
//
//
//
//**************************************************************************
//    Filename: lzh.c
//    Author:   converted from Pascal to C by Kevin Peterson
//    Creation Date: April 21, 2000
//
//**************************************************************************
//    Description:
//
//		LZH compression routines, using LZRW1/KH compression algoritm.
//
//		Original Pascal comment header follows ...
//
//**************************************************************************
//    Revision History:
//
//**************************************************************************
//{    ###################################################################   }
//{    ##                                                               ##   }
//{    ##      ##    ##### #####  ##   ##  ##      ## ##  ## ##  ##     ##   }
//{    ##      ##      ### ##  ## ## # ## ###     ##  ## ##  ##  ##     ##   }
//{    ##      ##     ###  #####  #######  ##    ##   ####   ######     ##   }
//{    ##      ##    ###   ##  ## ### ###  ##   ##    ## ##  ##  ##     ##   }
//{    ##      ##### ##### ##  ## ##   ## #### ##     ##  ## ##  ##     ##   }
//{    ##                                                               ##   }
//{    ##   EXTREMELY FAST AND EASY TO UNDERSTAND COMPRESSION ALGORITM  ##   }
//{    ##                                                               ##   }
//{    ###################################################################   }
//{    ##                                                               ##   }
//{    ##   This unit implements the updated LZRW1/KH algoritm which    ##   }
//{    ##   also implements  some RLE coding  which is usefull  when    ##   }
//{    ##   compress files  containing  a lot  of consecutive  bytes    ##   }
//{    ##   having the same value.   The algoritm is not as good  as    ##   }
//{    ##   LZH, but can compete with Lempel-Ziff.   It's the fasted    ##   }
//{    ##   one I've encountered upto now.                              ##   }
//{    ##                                                               ##   }
//{    ##                                                               ##   }
//{    ##                                                               ##   }
//{    ##                                                Kurt HAENEN    ##   }
//{    ##                                                               ##   }
//{    ###################################################################   }


#include <memory.h>

//{$R-} { NO range checking !! }


//UNIT LZRW1KH;

//INTERFACE

//uses SysUtils;

//{$IFDEF WIN32}
//type Int16 = SmallInt;
//{$ELSE}
//type Int16 = Integer;
//{$ENDIF}

//CONST
//    BufferMaxSize  = 32768;
//    BufferMax      = BufferMaxSize-1;
//    FLAG_Copied    = $80;
//    FLAG_Compress  = $40;
//#define BufferMaxSize	32768
//#define BufferMax		BufferMaxSize-1
#define FLAG_Copied		0x80
#define FLAG_Compress	0x40

//TYPE
//    BufferIndex    = 0..BufferMax + 15;
//    BufferSize     = 0..BufferMaxSize;
//       { extra bytes needed here if compression fails      *dh *}
//    BufferArray    = ARRAY [BufferIndex] OF BYTE;
//    BufferPtr      = ^BufferArray;


//    ELzrw1KHCompressor = Class(Exception);


//IMPLEMENTATION

//type
//  HashTable      = ARRAY [0..4095] OF Int16;
//  HashTabPtr     = ^Hashtable;

//VAR
//  Hash                     : HashTabPtr;
#define HashTableSize	4096

long Hash[ HashTableSize ];

//                             { check if this string has already been seen }
//                             { in the current 4 KB window }
//FUNCTION  GetMatch       (    Source         : BufferPtr;
//                              X              : BufferIndex;
//                              SourceSize     : BufferSize;
//                              Hash           : HashTabPtr;
//                          VAR Size           : WORD;
//                          VAR Pos            : BufferIndex  )    : BOOLEAN;
bool GetMatch( unsigned char *Source, long X, long SourceSize, long *Hash, short *Size, long *Pos )
{
//VAR
//  HashValue      : WORD;
//  TmpHash        : Int16;
    short HashValue;
    long TmpHash;

//BEGIN
//  HashValue := (40543*((((Source^[X] SHL 4) XOR Source^[X+1]) SHL 4) XOR
//                                     Source^[X+2]) SHR 4) AND $0FFF;
    HashValue = ( 40543 * ((((Source[X]<<4) ^ Source[X+1]) << 4) ^ Source[X+2]) >> 4 ) & 0x0fff;
//  Result := FALSE;
//  TmpHash := Hash^[HashValue];
    TmpHash = Hash[HashValue];

//  IF (TmpHash <> -1) and (X - TmpHash < 4096) THEN BEGIN
    if ((TmpHash != -1) && ((X - TmpHash) < 4096))
    {
//    Pos := TmpHash;
//    Size := 0;
        *Pos = TmpHash;
        *Size = 0;

//    WHILE ((Size < 18) AND (Source^[X+Size] = Source^[Pos+Size])
//                       AND (X+Size < SourceSize)) DO begin
        while ((*Size < 18) && (Source[X+*Size] == Source[*Pos+*Size]) && (X+*Size < SourceSize))
        {
//      INC(Size);
            (*Size)++;
//    end;
        }

//    Result := (Size >= 3)
        return( *Size >= 3 );
//  END;
    }

//  Hash^[HashValue] := X
    Hash[HashValue] = X;

//END;
    return( false );
}
//                                    { compress a buffer of max. 32 KB }
//FUNCTION  Compression(Source, Dest : BufferPtr;
//                      SourceSize   : BufferSize) :BufferSize;
long Compression( unsigned char *Source, unsigned char *Dest, long SourceSize )
{
//VAR
//  Bit,Command,Size         : WORD;
//  Key                      : Word;
//  X,Y,Z,Pos                : BufferIndex;
    short Bit, Command, Size;
    short Key;
    long X, Y, Z, Pos;

//BEGIN
//  FillChar(Hash^,SizeOf(Hashtable), $FF);
//  Dest^[0] := FLAG_Compress;
//  X := 0;
//  Y := 3;
//  Z := 1;
//  Bit := 0;
//  Command := 0;
    memset( &Hash, 0xff, sizeof(Hash) );

    Dest[0] = FLAG_Compress;
    X = 0;
    Y = 3;
    Z = 1;
    Bit = 0;
    Command = 0;

//  WHILE (X < SourceSize) AND (Y <= SourceSize) DO BEGIN
    while ((X < SourceSize) && (Y <= SourceSize))
    {
//    IF (Bit > 15) THEN BEGIN
        if (Bit > 15)
        {
//      Dest^[Z] := HI(Command);
//      Dest^[Z+1] := LO(Command);
//      Z := Y;
//      Bit := 0;
//      INC(Y,2)
            Dest[Z] = (unsigned char)(Command >> 8);
            Dest[Z+1] = (unsigned char)(Command & 0xff);
            Z = Y;
            Bit = 0;
            Y += 2;
//    END;
        }

////    Size := 1;
        Size = 1;
//    WHILE ((Source^[X] = Source^[X+Size]) AND (Size < $FFF)
//                         AND (X+Size < SourceSize)) DO begin
//              INC(Size);
//    end;

        while ((Source[X] == Source[X+Size]) && (Size < 0x0fff) && (X+Size < SourceSize))
        {
            Size++;
        }

//    IF (Size >= 16) THEN BEGIN
        if (Size >= 16)
        {
//      Dest^[Y] := 0;
//      Dest^[Y+1] := HI(Size-16);
//      Dest^[Y+2] := LO(Size-16);
//      Dest^[Y+3] := Source^[X];
            Dest[Y] = 0;
            Dest[Y+1] = (unsigned char)( (Size-16) >> 8 );
            Dest[Y+2] = (unsigned char)( (Size-16) & 0xff);
            Dest[Y+3] = Source[X];
//      INC(Y,4);
//      INC(X,Size);
//      Command := (Command SHL 1) + 1;
            Y += 4;
            X += Size;
            Command = (Command << 1) + 1;
//    END
        }
//    ELSE begin { not size >= 16 }
        else
        {
//      IF (GetMatch(Source,X,SourceSize,Hash,Size,Pos)) THEN BEGIN
            if (GetMatch( Source, X, SourceSize, Hash, &Size, &Pos ))
            {
//        Key := ((X-Pos) SHL 4) + (Size-3);
//        Dest^[Y] := HI(Key);
//        Dest^[Y+1] := LO(Key);
                Key = ((X-Pos) << 4) + (Size-3);
                Dest[Y] = (unsigned char)(Key >> 8);
                Dest[Y+1] = (unsigned char)(Key & 0xff);
//        INC(Y,2);
//        INC(X,Size);
//        Command := (Command SHL 1) + 1
                Y += 2;
                X += Size;
                Command = (Command << 1) + 1;
//      END
            }
//      ELSE BEGIN
            else
            {
//        Dest^[Y] := Source^[X];
                Dest[Y] = Source[X];
//        INC(Y);
//        INC(X);
//        Command := Command SHL 1
                Y++;
                X++;
                Command = Command << 1;
//      END;
            }
//    end; { size <= 16 }
        }   //  size <= 16

//    INC(Bit);
        Bit++;
//  END; { while x < sourcesize ... }
    }   // while( x < SourceSize ... )

//  Command := Command SHL (16-Bit);
//  Dest^[Z] := HI(Command);
//  Dest^[Z+1] := LO(Command);
    Command = Command << (16-Bit);
    Dest[Z] = (unsigned char)(Command >> 8);
    Dest[Z+1] = (unsigned char)(Command & 0xff);

//  IF (Y > SourceSize) THEN BEGIN
    if (Y > SourceSize)
    {
//    MOVE(Source^[0],Dest^[1],SourceSize);
//    Dest^[0] := FLAG_Copied;
//    Y := SUCC(SourceSize)
        memcpy( &(Dest[1]), &(Source[0]), SourceSize );
        Dest[0] = FLAG_Copied;
        Y = SourceSize +1;
//  END;
    }

//  Result := Y
    return( Y );
//END;
}

//                                    { decompress a buffer of max 32 KB }
//FUNCTION  Decompression(Source,Dest    : BufferPtr;
//                        SourceSize     : BufferSize) : BufferSize;
long Decompression( unsigned char *Source, unsigned char * Dest, long SourceSize )
{
//VAR
//  X,Y,Pos                  : BufferIndex;
//  Command,Size,K           : WORD;
//  Bit                      : BYTE;
//  SaveY                    : BufferIndex; { * dh * unsafe for-loop variable Y }
    long X, Y, Pos;
    short Command, Size, K;
    unsigned char Bit;
    long SaveY;

//BEGIN
//  IF (Source^[0] = FLAG_Copied) THEN  begin
    if (Source[0] == FLAG_Copied)
    {
//    FOR Y := 1 TO PRED(SourceSize) DO begin
        for (Y = 1; Y <= (SourceSize - 1); Y++)
        {
//      Dest^[PRED(Y)] := Source^[Y];
//      SaveY := Y;
            Dest[Y-1] = Source[Y];
            SaveY = Y;
//    end;
        }

//    Y := SaveY;
        Y = SaveY;
//  end
    }
//  ELSE BEGIN
    else
    {
//    Y := 0;
//    X := 3;
//    Command := (Source^[1] SHL 8) + Source^[2];
//    Bit := 16;
        Y = 0;
        X = 3;
        Command = (Source[1] << 8) + Source[2];
        Bit = 16;

//    WHILE (X < SourceSize) DO BEGIN
        while (X < SourceSize)
        {
//      IF (Bit = 0) THEN BEGIN
            if (Bit == 0)
            {
//        Command := (Source^[X] SHL 8) + Source^[X+1];
//        Bit := 16;
//        INC(X,2)
                Command = (Source[X] << 8) + Source[X+1];
                Bit = 16;
                X += 2;
//      END;
            }

//      IF ((Command AND $8000) = 0) THEN BEGIN
            if ((Command & 0x8000) == 0)
            {
//           Dest^[Y] := Source^[X];
//           INC(X);
//           INC(Y)
                Dest[Y] = Source[X];
                X++;
                Y++;
//      END
            }
//      ELSE BEGIN  { command and $8000 }
            else    // command & 0x8000
            {
//        Pos := ((Source^[X] SHL 4)
//               +(Source^[X+1] SHR 4));
                Pos = ( (Source[X] << 4) + (Source[X+1] >> 4) );
//        IF (Pos = 0) THEN BEGIN
                if (Pos == 0)
                {
//          Size := (Source^[X+1] SHL 8) + Source^[X+2] + 15;
                    Size = (Source[X+1] << 8) + Source[X+2] + 15;

//          FOR K := 0 TO Size DO begin
                    for (K = 0; K <= Size; K++)
                    {
//               Dest^[Y+K] := Source^[X+3];
                        Dest[Y+K] = Source[X+3];
//          end;
                    }

//          INC(X,4);
//          INC(Y,Size+1)
                    X += 4;
                    Y += Size + 1;
//        END
                }
//        ELSE BEGIN  { pos = 0 }
                else    // pos == 0
                {
//          Size := (Source^[X+1] AND $0F)+2;
                    Size = (Source[X+1] & 0x0f) + 2;

//          FOR K := 0 TO Size DO
                    for (K = 0; K <= Size; K++)
//               Dest^[Y+K] := Dest^[Y-Pos+K];
                        Dest[Y+K] = Dest[Y-Pos+K];

//          INC(X,2);
//          INC(Y,Size+1)
                    X += 2;
                    Y += Size+1;
//        END; { pos = 0 }
                }   //  pos == 0
//      END;  { command and $8000 }
            }   //  command & 0x8000

//      Command := Command SHL 1;
            Command = Command << 1;
////      DEC(Bit)
            Bit--;
//    END { while x < sourcesize }
        }   // while x < sourcesize
//  END;
    }

//  Result := Y
    return( Y );
//END;  { decompression }
}   //  decompression

