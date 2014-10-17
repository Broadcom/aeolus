/****************************************************************************
*  
*  Copyright (c) 2007 Broadcom Corporation
*
*  This program is the proprietary software of Broadcom Corporation and/or
*  its licensors, and may only be used, duplicated, modified or distributed
*  pursuant to the terms and conditions of a separate, written license
*  agreement executed between you and Broadcom (an "Authorized License").
*  Except as set forth in an Authorized License, Broadcom grants no license
*  (express or implied), right to use, or waiver of any kind with respect to
*  the Software, and Broadcom expressly reserves all rights in and to the
*  Software and all intellectual property rights therein.  IF YOU HAVE NO
*  AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY,
*  AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE
*  SOFTWARE.  
*
*  Except as expressly set forth in the Authorized License,
*
*  1.     This program, including its structure, sequence and organization,
*  constitutes the valuable trade secrets of Broadcom, and you shall use all
*  reasonable efforts to protect the confidentiality thereof, and to use this
*  information only in connection with your use of Broadcom integrated circuit
*  products.
*
*  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
*  "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS
*  OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*  RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
*  IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR
*  A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
*  ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME
*  THE ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
*
*  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM
*  OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*  INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
*  RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
*  HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN
*  EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1,
*  WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY
*  FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
*
****************************************************************************
*  $Id: MessageLogZapper.cpp 1.7 2007/11/08 15:43:34 enderby Exp $
*
*  Filename:       MessageLogZapper.cpp
*  Author:         John McQueen & Chris Zacker
*  Creation Date:  Sept 12, 2001
*
****************************************************************************
*  Description:  This application compensates for the GNU compiler's inability
*  to not compile out string text from the MessageLogging macros that are compiled
*  out from the V2 application by using the gNullMessageLog.//      
*
*****************************************************************************/

// Include files
#ifndef __GNUC__
//    #include <stdafx.h>
#endif
#include <string.h>
#include <stdio.h>

// Defines
#define MLZ_MAJOR_VER   1
#define MLZ_MINOR_VER   1
#define MLZ_RELEASE_VER 2

#define MAX_LINE_SIZE   250


// These are the message log string we search for...
char *pMessageLogs[] = 
{


  "gFatalErrorMsg",
  "gFatalErrorMsgNoFields",
  "gErrorMsg",
  "gErrorMsgNoFields",
  "gWarningMsg",
  "gWarningMsgNoFields",
  "gInitMsg",
  "gInitMsgNoFields",
  "gFuncEntryExitMsg",
  "gFuncEntryExitMsgNoFields",
  "gInfoMsg",
  "gInfoMsgNoFields",
  "gAlwaysMsg",
  "gAlwaysMsgNoFields",
  "gAppMsg",
  "gAppMsgNoFields",
  "SnmpLogRaw",
  "SnmpLogWarn",
  "SnmpLogErr",
  "SnmpLogStart",
  "SnmpLogInit",
  "SnmpLogSearch",
  "SnmpLogMem",
  "SnmpLogCfg",
  "SnmpLogReq",
  "SnmpLogNm",
  "SnmpLogFilt",
  "SnmpLogEv",

#if 0
  "CallTrace(",
  "gLogMessage",
  "gLogMessageNoFields",
  "gLogMessageRaw",
#endif
  NULL
};




int FindMessageLogString( char *pLine )
{
    int i=0;

    if ( pLine == NULL ) 
        return -1;

    while ( pMessageLogs[i] )
    {
        if ( strstr(pLine, pMessageLogs[i] ) != NULL )
            return 0; 
        i++;       
    }

    return -1;
}



int main(int argc, char* argv[])
{
   
   FILE *outputFile;
   FILE *inputFile;
   FILE *logFile;

   char filename[100];
   char line[MAX_LINE_SIZE];
   char *pDest = NULL;
   int  lineNumber=0;
   int  OnlyWhiteSpaceRemaining=0;
   int  FoundEmbeddedSemicolon=0;
   int  LookingForTerminatingSemicolon=0;
   int  InsideAComment=0;
   int  AbortZapping=0;
  
   if ( argc < 2 )
   {
      printf("Broadcom MessageLogZapper version %d.%d.%d\n", MLZ_MAJOR_VER, MLZ_MINOR_VER, MLZ_RELEASE_VER);
      return -1;
   }

   // open the messagelogzapper.log file for appending
   if( (logFile = fopen( "messagelogzapper.log" , "a+" )) == NULL )
   {
       printf("MessageLogZapper error: unable to open log file\n");
       return -1;
   }

   // open the inputted file for reading
   if( (inputFile = fopen( argv[1], "r" )) == NULL )
   {
       printf("MessageLogZapper error: unable to open input file <%s>\n",argv[1]);
       return -1;
   }

   if (argc > 2)
   {
       // open the output file to write, name specified on the command line
       sprintf(filename,"%s", argv[2]);
   }
   else
   {
       // open the output file to write, we add the suffix .bak.cpp to the extension 
       sprintf(filename,"%s%s", argv[1], ".bak.cpp");
   }

   if( (outputFile = fopen( filename, "w+" )) == NULL )
   {
       printf("MessageLogZapper error: unable to open output file <%s>\n",filename);
       return -1;
   }

   // Store version in log file
   fprintf(logFile,"Broadcom MessageLogZapper version %d.%d.%d, Zapping file: %s\n", MLZ_MAJOR_VER, MLZ_MINOR_VER, MLZ_RELEASE_VER, argv[1]);

   // clear out the line buffer
   memset(&line,0x00,sizeof(line));    
  
    // get the next line
   while ( fgets( line, MAX_LINE_SIZE, inputFile ) != NULL)
   {
     lineNumber++;

     // Lets check if we are to be disabled
     if ( strstr(line, "NEINZAPPER" ) != NULL )
     {
         fprintf(logFile,"File requested no zapping..\n");
         AbortZapping = 1;
     }

	 // True if we found one of our log strings in this line and we have not aborted zapping
     if ( (FindMessageLogString(&line[0]) == 0 || LookingForTerminatingSemicolon) && (AbortZapping == 0) )
     {

        // Look for a terminating semicolon
        pDest = NULL;
        pDest = strstr(line,";");

        if ( pDest == NULL )
        {
            // Output this line to line 
            fprintf(outputFile,"%c%c%c%s",';','/','/',line);
            fprintf(logFile,"FILE:%s LINE:%d %s",filename,lineNumber,line);
            
            memset(&line,0x00,sizeof(line));
            LookingForTerminatingSemicolon = 1;
            continue;
        }

        // Found a semicolon!!!
        // Check to see if it is a semicolon embedded in the text...
        // Check to see if it is a semicolon on the same line as the log message
        // Check to see if it is our desired terminating semicolon...
        else
        {
            OnlyWhiteSpaceRemaining= 1;
            pDest++;
            
            // Iterate through the remainder of the line...
            while ( pDest < line + strlen(line) )
            {
                    // check if it points to a semicolon; if so,
                    // step over it
                    if ( *pDest == 0x3b ) /* ';' => 0x3b */
                    {
                        OnlyWhiteSpaceRemaining = 1;
                        pDest++;
                        continue;
                    }

                    // look for white space Also check for Horiz TAB, vert Tab and FormFeed, Space, New Line, Carriage return
                    if ( *pDest != 0x09 && *pDest != 0x0b && *pDest != 0x0c && *pDest != 0x0a && *pDest != 0x0d && *pDest != 0x20)
                    {
						if ( (*pDest == '/')  && (*(pDest+1) == '/') )
						{
							// found C++ commenting-style - the remainder of the line
						    // will be commented out.. so break out of line loop
							printf("MessageLogZapper: Warning found C++ comment file %s line %d\n",
								filename,lineNumber );
							break;
						}

						if ( (*pDest == '/')  && (*(pDest+1)) == '*')
						{
							// inside a C-sytle comment
							printf("MessageLogZapper: Warning found start of C comment file %s line %d\n",
								filename,lineNumber );
							InsideAComment = 1;
							pDest++;
						}

						if ( (*pDest == '*')  && (*(pDest+1)) == '/') 
						{
							// found possible commented out log message
							printf("MessageLogZapper: Warning found end of C comment file %s line %d\n",
								filename,lineNumber );
							InsideAComment = 0;
							pDest++; // skip over terminating '/' of comment
							
						}
						else
						{
							// If we are not inside a comment, mark this as not white space.
							if ( ! InsideAComment )
								OnlyWhiteSpaceRemaining = 0;
						}
                    }

                    pDest++;     
                                                                  
            }  // end of while ()


            if ( OnlyWhiteSpaceRemaining )
            {
                LookingForTerminatingSemicolon =0;
            }
                
            fprintf(outputFile,"%c%c%c%s",';','/','/',line);
            fprintf(logFile,"FILE:%s LINE:%d %s\n",filename,lineNumber,line);
            memset(&line,0x00,sizeof(line));

                       
        } // end of else
      
     }  // end of if ( FindMessageLogString ) 
     

     // Not one of our MessageLog strings...
     else
     {
         fprintf(outputFile,"%s",line);
         memset(&line,0x00,sizeof(line));
     }

   
  } // end of while ( fgets( line, MAX_LINE_SIZE, inputFile ) != NULL)
 

  fprintf(logFile,"\n");

  fclose( outputFile );
  fclose( inputFile );
  fclose( logFile);
  
  return 0;


}






