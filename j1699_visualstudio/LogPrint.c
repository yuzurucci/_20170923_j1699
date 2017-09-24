/*
********************************************************************************
** SAE J1699-3 Test Source Code
**
**  Copyright (C) 2002 Drew Technologies. http://j1699-3.sourceforge.net/
**
** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; either version 2 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
**
** This source code, when compiled and used with an SAE J2534-compatible pass
** thru device, is intended to run the tests described in the SAE J1699-3
** document in an automated manner.
**
** This computer program is based upon SAE Technical Report J1699,
** which is provided "AS IS"
**
** See j1699.c for details of how to build and run this test.
**
********************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <windows.h>
#include "j2534.h"
#include "j1699.h"



void WriteToLog ( char *LogString, LOGTYPE LogType );

/* OBD message response data structures */
typedef struct
{
	unsigned long ulStartIndex;
	unsigned long ulLength;
} TRANSACTIONENTRY;



/*
 * Global variables that should only be accessed by functions in this file
 */
unsigned long gCommentCount = 0;           /* the total count of "COMMENT:" */
unsigned long gUserErrorCount = 0;         /* the total count of "USER WARNING:" */
unsigned long gFailureCount = 0;           /* the total count of "FAILURE:" */
unsigned long gWarningCount = 0;           /* the total count of "WARNING:" */
unsigned long gJ2534FailureCount = 0;      /* the total count of "J2534 FAILURE:" */
unsigned long gulTransactionBufferEnd = 0; /* array index for end of transaction ring buffer */
unsigned long gulTransactionCount = 0;     /* count of transactions in ring buffer */
char gszTransactionBuffer[MAX_RING_BUFFER_SIZE]; /* transaction ring buffer */
TRANSACTIONENTRY grgsTransactionList[MAX_TRANSACTION_COUNT]; /* list of start indexes for transactions in ring buffer */


/*
***************************************************************************************************
** Log - Function to send information to both the console and the log file
***************************************************************************************************
*/
char Log( LOGTYPE LogType, SCREENOUTPUT ScreenOutput, LOGOUTPUT LogOutput, PROMPTTYPE PromptType, const char *LogString, ... )
{
	char PrintString[MAX_LOG_STRING_SIZE];
	char ErrorString[MAX_LOG_STRING_SIZE];
	char PrintBuffer[MAX_LOG_STRING_SIZE];
	char CommentString[MAX_MESSAGE_LOG_SIZE];
	char UserResponse[MAX_USERINPUT];
	BOOL AddComment = FALSE;
	static BOOL ContinueAll = FALSE;

	// Get the full input string
	va_list Args;

	va_start ( Args, LogString );

	vsprintf ( PrintString, LogString, Args );

	va_end ( Args );



	UserResponse[0] = 0;


	// Format the string depending on the log type
	switch ( LogType )
	{
		case ERROR_FAILURE:
		case FAILURE:
			// Failure in the test
			gOBDTestFailed = TRUE;

			// Failure in the test Section (Static or Dynamic Test)
			gOBDTestSectionFailed = TRUE;
			gOBDTestSubsectionFailed = TRUE;
			gFailureCount++;

			sprintf( PrintBuffer, "%s %s", "FAILURE:", PrintString );
			break;

		case J2534_FAILURE:
			// Failure in the test
			gOBDTestFailed = TRUE;

			// Failure in the test Section (Static or Dynamic Test)
			gOBDTestSectionFailed = TRUE;

			// Failure in the test subsection (Test X.XX)
			gOBDTestSubsectionFailed = TRUE;

			PassThruGetLastError(ErrorString);
			sprintf( PrintBuffer, "%s  %s  PassThruGetLastError=%s\n", "J2534 FAILURE:", PrintString, ErrorString );
			gJ2534FailureCount++;
			break;

		case USER_ERROR:
			sprintf( PrintBuffer, "%s %s", "USER WARNING:", PrintString );
			gUserErrorCount++;
			break;

		case WARNING:
			sprintf( PrintBuffer, "%s %s", "WARNING:", PrintString );
			gWarningCount++;
			break;

		case INFORMATION:
			sprintf( PrintBuffer, "%s %s", "INFORMATION:", PrintString );
			break;

		case RESULTS:
			sprintf( PrintBuffer, "%s %s", "RESULTS:", PrintString );
			break;

		case NETWORK:
			sprintf( PrintBuffer, "%s %s", "NETWORK:", PrintString );
			break;

		case BLANK:
			sprintf( PrintBuffer, "%s", PrintString );
			break;

		case SUBSECTION_BEGIN:
			sprintf( PrintBuffer, "%s %d.%d %s %s\n", "TEST: **** Test", TestPhase, TestSubsection, PrintString, "****" );
			break;

		case SUBSECTION_PASSED_RESULT:
			sprintf( PrintBuffer, "%s %d.%d %s %s\n\n", "RESULTS: **** Test", TestPhase, TestSubsection, "PASSED", "****" );
			break;

		case SUBSECTION_FAILED_RESULT:
			sprintf( PrintBuffer, "%s %d.%d %s %s\n\n", "RESULTS: **** Test", TestPhase, TestSubsection, "FAILED", "****" );
			break;

		case SUBSECTION_INCOMPLETE_RESULT:
			sprintf( PrintBuffer, "%s %d.%d %s %s\n\n", "RESULTS: **** Test", TestPhase, TestSubsection, "INCOMPLETE", "****" );
			break;

		case SECTION_PASSED_RESULT:
			sprintf( PrintBuffer, "\n\n%s %s %s %s\n\n", "RESULTS: **** All J1699-3", PrintString, "Tests PASSED", "****" );
			break;

		case SECTION_FAILED_RESULT:
			sprintf( PrintBuffer, "\n\n%s %s %s %s\n\n", "RESULTS: **** J1699-3", PrintString, "Test FAILED", "****" );
			break;

		case SECTION_INCOMPLETE_RESULT:
			sprintf( PrintBuffer, "\n\n%s %s %s %s\n\n", "RESULTS: **** J1699-3", PrintString, "Test INCOMPLETE", "****" );
			break;

		case COMMENT:
		case PROMPT:
		default:
			// include for consistency, but do nothing
		break;
	}

	if ( (LogType != PROMPT) && (LogType != COMMENT) )
	{
		// If Enabled, Print to the Screen
		if ( (ScreenOutput == SCREENOUTPUTON) && (gSuspendScreenOutput == FALSE) )
		{
			printf( "%s", PrintBuffer );
		}

		// If Enabled, Print to the Log File
		if (LogOutput == LOGOUTPUTON)
		{
			WriteToLog ( PrintBuffer, LogType );
		}
	}


	// If a Prompt is requested
	if ( PromptType != NO_PROMPT )
	{
		if ( LogType == SUBSECTION_FAILED_RESULT )
		{
			sprintf ( &PrintString[strlen(PrintString)], "Failure(s) detected.  Do you wish to continue?  ");
		}

		else if ( LogType == FAILURE || LogType == SECTION_FAILED_RESULT )
		{
			if ( PromptType == ENTER_PROMPT )
			{
				sprintf ( PrintString, "Failure(s) detected.  " );
			}
			else
			{
				sprintf ( PrintString, "Failure(s) detected.  Do you wish to continue?  " );
			}
		}

		else if ( (LogType != PROMPT) && (LogType != COMMENT) )
		{
			if ( (PromptType == ENTER_PROMPT) )
			{
				sprintf ( PrintString, " " );
			}
			else
			{
				sprintf ( PrintString, "Do you wish to continue?  " );
			}
		}


		switch ( PromptType )
		{
			case QUIT_CONTINUE_PROMPT:
			{
				do
				{
					sprintf (PrintBuffer, "PROMPT: %s (Enter Quit or Continue):  ", PrintString);

					// If Enabled, Print to the Screen
					if ( (ScreenOutput == SCREENOUTPUTON) && (gSuspendScreenOutput == FALSE) )
					{
						printf ( "\n%s", PrintBuffer );
					}

					/* Get the user response and log it */
					clear_keyboard_buffer ();
					gets ( UserResponse );

					// remove ASCII lowercase bit
					UserResponse[0] &= 0xDF;
				} while ( (UserResponse[0] != 'C') && (UserResponse[0] != 'Q') );

				sprintf ( &PrintBuffer[strlen(PrintBuffer)], "%s\n\n", UserResponse );
			}
			break;

			case YES_NO_PROMPT:
			{
				do
				{
					sprintf (PrintBuffer, "PROMPT: %s (Enter Yes or No):  ", PrintString);

					// If Enabled, Print to the Screen
					if ( (ScreenOutput == SCREENOUTPUTON) && (gSuspendScreenOutput == FALSE) )
					{
						printf ( "\n%s\n", PrintBuffer );
					}

					/* Get the user response and log it */
					clear_keyboard_buffer ();
					gets ( UserResponse );

					// remove ASCII lowercase bit
					UserResponse[0] &= 0xDF;
				} while ( (UserResponse[0] != 'Y') && (UserResponse[0] != 'N') );

				sprintf ( &PrintBuffer[strlen(PrintBuffer)], "%s\n\n", UserResponse );
			}
			break;

			case YES_NO_ALL_PROMPT:
			{
				do
				{
					sprintf (PrintBuffer, "PROMPT: %s (Enter Yes, No, All yes, or Comment):  ", PrintString);

					// If Enabled, Print to the Screen
					if ( (ScreenOutput == SCREENOUTPUTON) && (gSuspendScreenOutput == FALSE) )
					{
						/* Get the user response and log it */
						printf ( "\n%s", PrintBuffer );
					}

					if ( ContinueAll )
					{
						ContinueAll = TRUE;
						UserResponse[0] = 'Y';
						UserResponse[1] = 0x00;
						printf ( "%s\n", UserResponse );
					}
					else
					{
						clear_keyboard_buffer ();
						gets ( UserResponse );
					}

					// remove ASCII lowercase bit
					UserResponse[0] &= 0xDF;
				} while ( (UserResponse[0] != 'Y') && (UserResponse[0] != 'N') && (UserResponse[0] != 'A') && (UserResponse[0] != 'C') );
			
				// If user selected 'C'omment, allow the user to enter a short comment
				if ( UserResponse[0] == 'C' )
				{
					AddComment = TRUE;
				}
				// If user selected 'A'll Yes, set ContinueAll to respond 'Y'es to all future YES_NO_ALL_PROMPTs
				else if ( UserResponse[0] == 'A' )
				{
					ContinueAll = TRUE;
				}

				sprintf ( &PrintBuffer[strlen(PrintBuffer)], "%s\n\n", UserResponse );
			}
			break;

			case CUSTOM_PROMPT:
			{
				sprintf (PrintBuffer, "PROMPT: %s", PrintString);

				// If Enabled, Print to the Screen
				if ( (ScreenOutput == SCREENOUTPUTON) && (gSuspendScreenOutput == FALSE) )
				{
					printf ( "\n%s\n", PrintBuffer );
				}

				UserResponse[0] = 'Y';
				UserResponse[1] = 0x00;
			}
			break;

			case COMMENT_PROMPT:
			{
				if ( (ScreenOutput == SCREENOUTPUTON) && (gSuspendScreenOutput == FALSE) )
				{
					printf ( "\nPROMPT: %s  Type a comment (%d chars max) (Press Enter to continue):  ", PrintString, MAX_MESSAGE_LOG_SIZE-1);

					clear_keyboard_buffer ();
					fgets ( CommentString, MAX_MESSAGE_LOG_SIZE, stdin );

					// If the comment isn't empty (New Line), print it
					if ( CommentString[0] != 0x0A )
					{
						sprintf ( PrintBuffer, "COMMENT: %s\n", CommentString );
						gCommentCount++;
					}
					else
					{
						sprintf ( PrintBuffer, "\n" );
						LogType = BLANK;
					}
				}
				// if not able to prompt for a comment, don't write anything to the log
				else
				{
					LogOutput = LOGOUTPUTOFF;
				}
			}
			break;

			case ENTER_PROMPT:
			default:
			{
				sprintf (PrintBuffer, "PROMPT: %s (Press Enter to continue):  ", PrintString);

				// If Enabled, Print to the Screen
				if ( (ScreenOutput == SCREENOUTPUTON) && (gSuspendScreenOutput == FALSE) )
				{
					printf ( "\n%s", PrintBuffer );
				}

				/* Get the user response and log it */
				clear_keyboard_buffer ();
				gets ( UserResponse );

				sprintf ( &PrintBuffer[strlen(PrintBuffer)], "%s\n\n", UserResponse );

				UserResponse[0] = 'Y';
				UserResponse[1] = 0x00;
			}
		}


		// If the user chose to add a comment, log it
		if ( AddComment == TRUE )
		{
			// If Enabled, Print to the Screen
			if ( (ScreenOutput == SCREENOUTPUTON) && (gSuspendScreenOutput == FALSE) )
			{
				WriteToLog ( PrintBuffer, LogType );

				//Prompt for comment
				printf( "\nPROMPT: Type a comment (%d chars max) (Press Enter to continue):  ", MAX_MESSAGE_LOG_SIZE-1 );

				clear_keyboard_buffer ();
				fgets ( CommentString, MAX_MESSAGE_LOG_SIZE, stdin );

				// If the comment isn't empty (New Line), print it
				if ( CommentString[0] != 0x0A )
				{
					sprintf ( PrintBuffer, "COMMENT: %s\n", CommentString );
					WriteToLog ( PrintBuffer, LogType );
					gCommentCount++;
				}

				do
				{
					// Prompt to continue test
					sprintf ( PrintBuffer, "PROMPT: Do you wish to continue?  (Enter Yes or No):  " );
					printf( "\n%s", PrintBuffer );

					// Get the user response and log it
					clear_keyboard_buffer ();
					gets ( UserResponse );

					// remove ASCII lowercase bit
					UserResponse[0] &= 0xDF;
				} while ( (UserResponse[0] != 'Y') && (UserResponse[0] != 'N') );

				sprintf ( &PrintBuffer[strlen(PrintBuffer)], "%s\n\n", UserResponse );
			}
		}


		// If Enabled, Print to the Log File
		if ( LogOutput == LOGOUTPUTON )
		{
			WriteToLog ( PrintBuffer, LogType );
		}


		// Return first character of user response
		return ( UserResponse[0] & 0xDF );
	}

	// By Default return 'Y'es - positive result
	return( 'Y' );
}




/*
********************************************************************************
** WriteToLog - writes the passed string to the log file
********************************************************************************
*/
void WriteToLog ( char *LogString, LOGTYPE LogType )
{
	char LogBuffer[MAX_LOG_STRING_SIZE];
	unsigned long StringIndex;

	if (LogType == BLANK)
	{
		sprintf( LogBuffer, LogString);
	}
	else
	{
		// Get rid of any control characters or spaces at the beginning of the string before logging
		for ( StringIndex = 0;
		      (StringIndex < sizeof(LogString)) &&
		      (LogString[StringIndex] <= ' ') &&
		      (LogString[StringIndex] != '\0');
		      StringIndex++ )  {}

		// Add the timestamp and put the string in the log file
		if (LogType == PROMPT)
		{
			sprintf( LogBuffer, "\n+%06ldms %s",(GetTickCount() - gLastLogTime), &LogString[StringIndex] );
		}
		else
		{
			sprintf( LogBuffer, "+%06ldms %s",(GetTickCount() - gLastLogTime), &LogString[StringIndex] );
		}
	}

	gLastLogTime = GetTickCount();

	if ( gSuspendLogOutput == FALSE )
	{
		fputs( LogBuffer, ghLogFile );
		fflush( ghLogFile );
	}
	else
	{
		AddToTransactionBuffer( LogBuffer );
	}
}




/*
 * During the Manufacturer Specific Drive Cycle in the Dynamic Test, writing to 
 * the log file is suspended to avoid making the log files too large. Only state
 * changes and FAILUREs are logged. To help improve OEM debugging a ring buffer
 * was added so that network transactions prior to the failure could also be logged.
 * gszTransactionBuffer is used as temporary storage for strings that would
 * normally be written to the log file. The start of each transaction is marked by a
 * call to SaveTransactionStart, which records the offset in gszTransactionBuffer
 * where the strings will be written. The size of a transaction will vary and will
 * include multiple strings - size the buffers accordingly!
 */

/*
********************************************************************************
** SaveTransactionStart - saves the start index a of a new transaction
********************************************************************************
*/
void SaveTransactionStart (void)
{
	/* add this transaction to the list */
	grgsTransactionList[gulTransactionCount % MAX_TRANSACTION_COUNT].ulStartIndex = gulTransactionBufferEnd;

	gulTransactionCount++;

	/* account for roll over */
	if ( gulTransactionCount == (MAX_TRANSACTION_COUNT * 2) )
	{
		gulTransactionCount = MAX_TRANSACTION_COUNT;
	}

	/* zero out the next length */
	grgsTransactionList[gulTransactionCount % MAX_TRANSACTION_COUNT].ulLength = 0;
}


/*
********************************************************************************
** AddToTransactionBuffer - Adds a string to the transaction ring buffer
********************************************************************************
*/
void AddToTransactionBuffer (char *pszStringToAdd)
{
	unsigned long ulStringLength;
	unsigned long ulCount;

	ulStringLength = strlen(pszStringToAdd);

	/* update the transaction size */
	grgsTransactionList[(gulTransactionCount - 1) % MAX_TRANSACTION_COUNT].ulLength += ulStringLength;

	if ( (gulTransactionBufferEnd + ulStringLength) < sizeof(gszTransactionBuffer) )
	{
		strcpy (&gszTransactionBuffer[gulTransactionBufferEnd], pszStringToAdd);
		gulTransactionBufferEnd += ulStringLength;
	}
	else
	{
		/* account for buffer roll over */
		ulCount = (sizeof(gszTransactionBuffer) - gulTransactionBufferEnd);
		strncpy(&gszTransactionBuffer[gulTransactionBufferEnd], pszStringToAdd, ulCount);
		strcpy(&gszTransactionBuffer[0], &pszStringToAdd[ulCount]);
		gulTransactionBufferEnd = ulStringLength - ulCount;
	}
}


/*
********************************************************************************
** LogLastTransaction - Write transaction ring buffer to log file
********************************************************************************
*/
void LogLastTransaction (void)
{
	unsigned long ulTempBufferStart = 0;

	ulTempBufferStart = grgsTransactionList[(gulTransactionCount - 1) % MAX_TRANSACTION_COUNT].ulStartIndex;

	if (ulTempBufferStart < gulTransactionBufferEnd)
	{
		fwrite(
		        &gszTransactionBuffer[ulTempBufferStart],
		        1,
		        (gulTransactionBufferEnd - ulTempBufferStart),
		        ghLogFile
		      );
	}
	else
	{
		/* account for buffer roll over */
		fwrite(
		        &gszTransactionBuffer[ulTempBufferStart],
		        1,
		        (sizeof(gszTransactionBuffer) - ulTempBufferStart),
		        ghLogFile
		      );

		fwrite(&gszTransactionBuffer[0], 1, gulTransactionBufferEnd, ghLogFile);
	}

	fflush(ghLogFile);

}


/*
********************************************************************************
** DumpTransactionBuffer - Write transaction ring buffer to log file
********************************************************************************
*/
void DumpTransactionBuffer (void)
{
	unsigned long ulTempTransCount = 0;
	unsigned long ulTempEntryCount = 0;
	unsigned long ulTempSize = 0;
	unsigned long ulTempBufferStart = 0;
	BOOL bDone = FALSE;
	char szTempBuffer[MAX_LOG_STRING_SIZE];


	szTempBuffer[0] = 0;
	ulTempTransCount = gulTransactionCount;

	fputs("\n******** Buffer dump start (Failure detected) ********\n", ghLogFile);

	if (gulTransactionCount > 0)
	{
		/* determine the starting point of the transaction buffer */
		do
		{
			ulTempTransCount--;
			ulTempSize += grgsTransactionList[ulTempTransCount % MAX_TRANSACTION_COUNT].ulLength;

			if (ulTempSize > sizeof(gszTransactionBuffer))
			{
				/* went too far back, this transaction was over-written */
				bDone = TRUE;
				ulTempTransCount++;
			}
			else
			{
				ulTempEntryCount++;
			}

		} while (
		          (bDone == FALSE) &&
		          (ulTempTransCount > 0) &&
		          (ulTempEntryCount < DESIRED_DUMP_SIZE)
		        );

		ulTempBufferStart = grgsTransactionList[ulTempTransCount % MAX_TRANSACTION_COUNT].ulStartIndex;


		/* output the transaction buffer */
		if (ulTempBufferStart < gulTransactionBufferEnd)
		{
			fwrite(
			        &gszTransactionBuffer[ulTempBufferStart],
			        1,
			        (gulTransactionBufferEnd - ulTempBufferStart),
			        ghLogFile
			      );
		}
		else
		{
			/* account for buffer roll over */
			fwrite(
			        &gszTransactionBuffer[ulTempBufferStart],
			        1,
			        (sizeof(gszTransactionBuffer) - ulTempBufferStart),
			        ghLogFile
			      );
#ifdef _DEBUG
			fputs("\n*** ROLLOVER ***\n", ghLogFile);
#endif
			fwrite(&gszTransactionBuffer[0], 1, gulTransactionBufferEnd, ghLogFile);
		}
	}

#ifdef _DEBUG
	sprintf( szTempBuffer, "TransCnt: %d, Start: %d, End: %d\n", gulTransactionCount, ulTempBufferStart, gulTransactionBufferEnd);
	fputs ( szTempBuffer, ghLogFile );
	for (ulTempTransCount = 0; ulTempTransCount < MAX_TRANSACTION_COUNT; ulTempTransCount++)
	{
		sprintf( szTempBuffer,
		         "Idx: %d, Start: %d, Size: %d\n",
		         ulTempTransCount,
		         grgsTransactionList[ulTempTransCount % MAX_TRANSACTION_COUNT].ulStartIndex,
		         grgsTransactionList[ulTempTransCount % MAX_TRANSACTION_COUNT].ulLength);
		fputs ( szTempBuffer, ghLogFile );
	}
#endif

	fputs("******** Buffer dump end (Failure detected) ********\n\n", ghLogFile);

	fflush(ghLogFile);

	ClearTransactionBuffer();
}


/*
********************************************************************************
** ClearTransactionBuffer - Clear the transaction ring buffer
********************************************************************************
*/
void ClearTransactionBuffer (void)
{
	gulTransactionBufferEnd = 0;
	gulTransactionCount = 0;
	gszTransactionBuffer[0] = 0;
	memset(&grgsTransactionList[0], 0x00, sizeof(grgsTransactionList));
}


/*
********************************************************************************
** GetFailureCount - Returns the current count of FAILUREs
********************************************************************************
*/
unsigned long GetFailureCount (void)
{
	return gFailureCount;
}


/*
*******************************************************************************
** VerifyLogFile
**
** Returns:
**    PASS - OK to continue
**    FAIL - File not open / User data mis-match
**    ERRORS - Expected data not found
**    EXIT - Dynamic Tests already completed
*******************************************************************************
*/
STATUS VerifyLogFile ( FILE *hFileHandle )
{
	unsigned long ulTempValue;
	unsigned long ulTempUserErrorCount;
	unsigned long ulTempJ2534FailureCount;
	unsigned long ulTempWarningCount;
	unsigned long ulTempFailureCount;
	unsigned long ulTempCommentCount;
	int nTempYear;
	char cBuffer[256];
	char *pcBuf;
	BOOL bRevisionFound = FALSE;
	BOOL bUserInputFound = FALSE;
	BOOL bTotalsFound = FALSE;
	BOOL bTestsDone = FALSE;
	STATUS eResult = PASS;



	// log file should be open
	if ( hFileHandle == NULL )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTOFF, ENTER_PROMPT,
		     "Log File not open\n");
		return(FAIL);
	}

	// search from beginning of file
	fseek (hFileHandle, 0, SEEK_SET);

	while ( fgets (cBuffer, sizeof(cBuffer), hFileHandle) != 0 )
	{
		if ( bRevisionFound == FALSE )
		{
			/* check software version from log file */
			if ( substring(cBuffer, "Revision ") != 0 )
			{
				if ( substring(cBuffer, gszAPP_REVISION) == 0 )
				{
					Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
					     "Software version for log file does not match current version\n");
				}

				bRevisionFound = TRUE;
			}
		}

		if ( bTestsDone == FALSE )
		{
			if ( substring(cBuffer, "**** Test 11.11 (") != 0 )
			{
				/* all tests have been run */
				fseek ( hFileHandle, 0, SEEK_END );
				Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
				     "Dynamic Tests have already been completed!\n");
				return EXIT;
			}
		}

		if ( bUserInputFound == FALSE )
		{
			/* check for consistency of User Input */
			if ( (pcBuf = substring(cBuffer, g_rgpcDisplayStrings[DSPSTR_PRMPT_MODEL_YEAR])) != 0 )
			{
				nTempYear = atoi(&pcBuf[strlen(g_rgpcDisplayStrings[DSPSTR_PRMPT_MODEL_YEAR])]);
				if ( gModelYear != nTempYear )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
					     "Model Year does not match previous input\n");
					eResult = FAIL;
				}
			}

			if ( (pcBuf = substring(cBuffer, g_rgpcDisplayStrings[DSPSTR_PRMPT_OBD_ECU])) != 0 )
			{
				ulTempValue = atol(&pcBuf[strlen(g_rgpcDisplayStrings[DSPSTR_PRMPT_OBD_ECU])]);
				if ( gUserNumEcus != ulTempValue )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
					     "Number of OBD-II ECUs does not match previous input\n");
					eResult = FAIL;
				}
			}

			if ( (pcBuf = substring(cBuffer, g_rgpcDisplayStrings[DSPSTR_PRMPT_RPGM_ECU])) != 0 )
			{
				ulTempValue = atol(&pcBuf[strlen(g_rgpcDisplayStrings[DSPSTR_PRMPT_RPGM_ECU])]);
				if ( gUserNumEcusReprgm != ulTempValue )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
					     "Number of reprogrammable, OBD-II ECUs does not match previous input\n");
					eResult = FAIL;
				}
			}

			if ( substring(cBuffer, g_rgpcDisplayStrings[DSPSTR_PRMPT_ENG_TYPE]) != 0 )
			{
				if ( substring(cBuffer, gEngineTypeStrings[gUserInput.eEngineType]) == 0 )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
					     "Engine type does not match previous input\n");
					eResult = FAIL;
				}
			}

			if ( substring(cBuffer, g_rgpcDisplayStrings[DSPSTR_PRMPT_PWRTRN_TYPE]) != 0 )
			{
				if ( substring(cBuffer, gPwrTrnTypeStrings[gUserInput.ePwrTrnType]) == 0 )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
					     "Powertrain type does not match previous input\n");
					eResult = FAIL;
				}
			}

			if ( substring(cBuffer, g_rgpcDisplayStrings[DSPSTR_PRMPT_VEH_TYPE]) != 0 )
			{
				if ( substring(cBuffer, gVehicleTypeStrings[gUserInput.eVehicleType]) == 0 )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
					     "Vehicle type does not match previous input\n");
					eResult = FAIL;
				}
			}

			if ( substring(cBuffer, g_rgpcDisplayStrings[DSPSTR_STMT_COMPLIANCE_TYPE]) != 0 )
			{
				if ( substring(cBuffer, gComplianceTestTypeStrings[gUserInput.eComplianceType]) == 0 )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
					     "Compliance type does not match previous input\n");
					eResult = FAIL;
				}

				bUserInputFound = TRUE;
			}
		}

		/* get error totals; if file contians multiple instances, the last set will be used */
		if ( (pcBuf = substring(cBuffer, g_rgpcDisplayStrings[DSPSTR_RSLT_TOT_USR_ERROR])) != 0 )
		{
			ulTempUserErrorCount = atol(&pcBuf[strlen(g_rgpcDisplayStrings[DSPSTR_RSLT_TOT_USR_ERROR])]);
		}

		if ( (pcBuf = substring(cBuffer, g_rgpcDisplayStrings[DSPSTR_RSLT_TOT_J2534_FAIL])) != 0 )
		{
			ulTempJ2534FailureCount = atol(&pcBuf[strlen(g_rgpcDisplayStrings[DSPSTR_RSLT_TOT_J2534_FAIL])]);
			if (ulTempJ2534FailureCount)
			{
				gOBDTestSectionFailed = TRUE;
			}
		}

		if ( (pcBuf = substring(cBuffer, g_rgpcDisplayStrings[DSPSTR_RSLT_TOT_WARN])) != 0 )
		{
			ulTempWarningCount = atol(&pcBuf[strlen(g_rgpcDisplayStrings[DSPSTR_RSLT_TOT_WARN])]);
		}

		if ( (pcBuf = substring(cBuffer, g_rgpcDisplayStrings[DSPSTR_RSLT_TOT_FAIL])) != 0 )
		{
			ulTempFailureCount = atol(&pcBuf[strlen(g_rgpcDisplayStrings[DSPSTR_RSLT_TOT_FAIL])]);
			if (ulTempFailureCount)
			{
				gOBDTestSectionFailed = TRUE;
			}

			bTotalsFound = TRUE;
		}

		if ( (pcBuf = substring(cBuffer, g_rgpcDisplayStrings[DSPSTR_RSLT_TOT_COMMENTS])) != 0 )
		{
			ulTempCommentCount = atol(&pcBuf[strlen(g_rgpcDisplayStrings[DSPSTR_RSLT_TOT_COMMENTS])]);
		}
	}

	if (bRevisionFound == FALSE)
	{
		Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Software version for log file not found\n");
		eResult = ERRORS;
	}

	if (bUserInputFound == FALSE)
	{
		Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "User Input could not be found in log file\n");
		eResult = ERRORS;
	}

	if (bTotalsFound == FALSE)
	{
		Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Results Totals could not be found in log file\n");
		eResult = ERRORS;
	}
	else
	{
		if (eResult != FAIL)
		{
			/*
			 * if User Input matches then use the last values found,
			 * which should be from the end of the file
			 */
			gUserErrorCount += ulTempUserErrorCount;
			gJ2534FailureCount += ulTempJ2534FailureCount;
			gWarningCount += ulTempWarningCount;
			gFailureCount += ulTempFailureCount;
			gCommentCount += ulTempCommentCount;
		}
	}

	// move to end of file
	fseek ( hFileHandle, 0, SEEK_END );
	return(eResult);
}


/*
********************************************************************************
** LogStats
********************************************************************************
*/
void LogStats (void)
{
	unsigned long EcuIndex;

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, COMMENT_PROMPT,
	     "The statistics for the test so far are about to be written to the log file.");

	/* Print out the request / response statistics */
	for ( EcuIndex = 0; EcuIndex < OBD_MAX_ECUS; EcuIndex++ )
	{
		if ( gEcuTimingData[EcuIndex].EcuId == 0x00 )
		{
			break;
		}
		Log( RESULTS, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X:\n"
		     "     Average Initial Response Time = %ldmsec (%ld requests)\n"
		     "     Longest Initial Response Time = %ldmsec\n"
		     "     Responses Out Of Range        = %d\n"
		     "     Responses Sooner Than Allowed = %d\n"
		     "     Responses Later  Than Allowed = %d\n",
		     gEcuTimingData[EcuIndex].EcuId,
		     (gEcuTimingData[EcuIndex].AggregateResponseTimeMsecs / gEcuTimingData[EcuIndex].AggregateResponses),
		     gEcuTimingData[EcuIndex].AggregateResponses,
		     gEcuTimingData[EcuIndex].LongestResponsesTime,
		     gEcuTimingData[EcuIndex].RespTimeOutofRange,
		     gEcuTimingData[EcuIndex].RespTimeTooSoon,
		     gEcuTimingData[EcuIndex].RespTimeTooLate );

		/* Reset the per-ECU stats */
		gEcuTimingData[EcuIndex].AggregateResponseTimeMsecs = 0;
		gEcuTimingData[EcuIndex].AggregateResponses = 0;
		gEcuTimingData[EcuIndex].LongestResponsesTime = 0;
		gEcuTimingData[EcuIndex].RespTimeOutofRange = 0;
		gEcuTimingData[EcuIndex].RespTimeTooSoon = 0;
		gEcuTimingData[EcuIndex].RespTimeTooLate = 0;
	}

	Log( RESULTS, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "%s%d.\n", g_rgpcDisplayStrings[DSPSTR_RSLT_TOT_USR_ERROR], gUserErrorCount);
	Log( RESULTS, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "%s%d.\n", g_rgpcDisplayStrings[DSPSTR_RSLT_TOT_J2534_FAIL], gJ2534FailureCount);
	Log( RESULTS, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "%s%d.\n", g_rgpcDisplayStrings[DSPSTR_RSLT_TOT_WARN], gWarningCount);
	Log( RESULTS, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "%s%d.\n", g_rgpcDisplayStrings[DSPSTR_RSLT_TOT_FAIL], gFailureCount);
	Log( RESULTS, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "%s%d.\n", g_rgpcDisplayStrings[DSPSTR_RSLT_TOT_COMMENTS], gCommentCount);
	Log( RESULTS, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "%s%s\n", g_rgpcDisplayStrings[DSPSTR_STMT_COMPLIANCE_TYPE],
	     gComplianceTestTypeStrings[gUserInput.eComplianceType]);

	/* Reset the stats */
	gUserErrorCount = 0;
	gFailureCount = 0;
	gJ2534FailureCount = 0;
	gWarningCount = 0;

}
