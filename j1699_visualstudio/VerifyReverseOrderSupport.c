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
**	DATE		MODIFICATION
**	05/10/04	Modified to request ALL PIDs starting from $FF, not $E0 as
**				originally coded.
********************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <time.h>
#include <windows.h>
#include "j2534.h"
#include "j1699.h"

unsigned char gReverseOrderState[OBD_MAX_ECUS];

/*
*******************************************************************************
** VerifyReverseOrderSupport - Function to verify SID1 reverse order support
*******************************************************************************
*/
STATUS VerifyReverseOrderSupport(void)
{
	unsigned long EcuIndex;
	unsigned int  IdIndex;
	unsigned long IdBitIndex;
	unsigned long ulInit_FailureCount = 0;
	SID_REQ SidReq;

	BOOL bTestFailed = FALSE;

	/* Save the original response data for comparison */
	memcpy(&gOBDCompareResponse[0], &gOBDResponse[0], sizeof(gOBDResponse));

	ulInit_FailureCount = GetFailureCount();

	/* Ignore unsupported responses */
	/* Support info is cleared on receive of first response to reverse order request */
	gIgnoreUnsupported = TRUE;

	/* reverse order request started */
	for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
	{
		gReverseOrderState[EcuIndex] = REVERSE_ORDER_REQUESTED;
	}

	/* Request SID 1 support data in reverse order */
	for (IdIndex = 0xE0; IdIndex < 0x100; IdIndex -= 0x20)
	{
		SidReq.SID = 1;
		SidReq.NumIds = 1;
		SidReq.Ids[0] = (unsigned char)IdIndex;
		if (SidRequest(&SidReq, SID_REQ_NORMAL|SID_REQ_IGNORE_NO_RESPONSE) == FAIL)
		{
			/* There must be a response to PID 0x00 */
			if (IdIndex == 0x00)
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "No response to SID $1 PID $%02X request\n", IdIndex );
				bTestFailed = TRUE;
			}
			else
			{
				for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
				{
					/* Check if PID is supported */
					if ( (gOBDResponse[EcuIndex].Sid1PidSupportSize != 0) &&
					     ((gOBDCompareResponse[EcuIndex].Sid1PidSupport[(IdIndex - 1) >> 5].IDBits[
					     ((IdIndex - 1) >> 3) & 0x03] & (0x80 >> ((IdIndex - 1) & 0x07))) != 0x00))
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  No response to SID $1 PID $%02X request\n",
						     GetEcuId(EcuIndex),
						     IdIndex );
						bTestFailed = TRUE;
					}
				}
			}
		}
		else if (IdIndex != 0x00)
		{
			for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
			{
				/* Check if data received and PID is not supported */
				if ( (gOBDResponse[EcuIndex].Sid1PidSupportSize != 0) &&
				     ((gOBDCompareResponse[EcuIndex].Sid1PidSupport[(IdIndex - 1) >> 5].IDBits[
				     ((IdIndex - 1) >> 3) & 0x03] & (0x80 >> ((IdIndex - 1) & 0x07))) == 0))
				{
					/* If ISO15765 protocol, this is an error */
					if (gOBDList[gOBDListIndex].Protocol == ISO15765)
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  Unsupported SID $1 PID $%02X response\n",
						     GetEcuId(EcuIndex),
						     IdIndex );
						bTestFailed = TRUE;
					}
				}
			}
		}
	}


	if ( VerifySid1PidSupportData() == FAIL )
	{
		bTestFailed = TRUE;
	}


	/* No longer ignore unsupported response */
	gIgnoreUnsupported = FALSE;

	/* reverse order request finished */
	for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
	{
		gReverseOrderState[EcuIndex] = NOT_REVERSE_ORDER;
	}

	/* Verify that all SID 1 PID support data compares */
	for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
	{
		for (IdIndex = 0; IdIndex < 8; IdIndex++)
		{
			for (IdBitIndex = 0; IdBitIndex < 4; IdBitIndex++)
			{
				if ( gOBDResponse[EcuIndex].Sid1PidSupport[IdIndex].IDBits[IdBitIndex] !=
				     gOBDCompareResponse[EcuIndex].Sid1PidSupport[IdIndex].IDBits[IdBitIndex])
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  SID $1 PID $%02X Byte %d reverse/normal support response mismatch\n",
					     GetEcuId(EcuIndex),
						 gOBDCompareResponse[EcuIndex].Sid1PidSupport[IdIndex].FirstID,
					     IdBitIndex );
					bTestFailed = TRUE;
				}
			}
		}
	}


	/* Determine size of PIDs $06, $07, $08, $09 */
	if (DetermineVariablePidSize () != PASS)
	{
		bTestFailed = TRUE;
	}

	/* Warn the user that this could take a while */
	Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTOFF, NO_PROMPT,
		 "Please wait (press any key to abort)...\n");
	clear_keyboard_buffer ();

	/* Request SID 1 PID data in reverse */
	for (IdIndex = 0xFF; IdIndex < 0x100; IdIndex -= 0x01)
	{
		/* Bypass PID request if it is a support PID (already done above) */
		if ((IdIndex & 0x1F) != 0)
		{
			printf( "INFORMATION: Checking PID $%02X\r", IdIndex );
			SidReq.SID = 1;
			SidReq.NumIds = 1;
			SidReq.Ids[0] = (unsigned char)IdIndex;
			SidRequest(&SidReq, SID_REQ_NORMAL|SID_REQ_IGNORE_NO_RESPONSE);
			for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
			{
				/* Check if data received and PID is not supported */
				if ( (gOBDResponse[EcuIndex].Sid1PidSize != 0) &&
					 (IsSid1PidSupported (EcuIndex, IdIndex) == FALSE) )
				{
					/* If ISO15765 protocol, this is an error */
					if (gOBDList[gOBDListIndex].Protocol == ISO15765)
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								"ECU %X  Unsupported SID $1 PID $%02X response\n",
								GetEcuId(EcuIndex),
								IdIndex );
					}
				}

				/* Check if no data received and PID is supported */
				if ( (gOBDResponse[EcuIndex].Sid1PidSize == 0) &&
					 (IsSid1PidSupported (EcuIndex, IdIndex) == TRUE) )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							"ECU %X  SID $1 PID $%02X in reverse order failed\n",
							GetEcuId(EcuIndex),
							IdIndex );
				}
			}
		}

		/* Abort test if user presses a key */
		if (kbhit())
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "User abort\n");
			clear_keyboard_buffer ();
			return(FAIL);
		}
	}

	printf("\n");

	/*
	** If ISO15765 protocol, make sure the required OBDMID/SDTID values are supported
	** and try group support.  Verify group test only for ISO15765 only!
	*/
	if (gOBDList[gOBDListIndex].Protocol == ISO15765)
	{
		if(VerifyReverseGroupDiagnosticSupport() == FAIL)
		{
			bTestFailed = TRUE;
		}
	}

	/* Link active test to verify communication remained active for ALL protocols */
	if (VerifyLinkActive() != PASS)
	{
		bTestFailed = TRUE;
	}

	if ( (bTestFailed == TRUE) || (ulInit_FailureCount != GetFailureCount()) )
	{
		/* There could have been early/late responses that weren't treated as FAIL
		 * or the test failed earlier
		 */
		return(FAIL);
	}
	else
	{
		return(PASS);
	}

}

