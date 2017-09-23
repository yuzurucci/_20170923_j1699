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
#include <time.h>
#include <windows.h>
#include "j2534.h"
#include "j1699.h"

/*
***************************************************************************************************
** ClearCodes - Function to clear OBD diagnostic information
***************************************************************************************************
*/
STATUS ClearCodes(void)
{
	SID_REQ       SidReq;
	STATUS eReturnCode = PASS;  // saves the return code from function calls
	unsigned long EcuIndex;
	unsigned short usPositiveAckCount = 0;
	unsigned short usRetryCount = 0;

	/* Reset the global DTC data */
	gOBDDTCPending = FALSE;
	gOBDDTCStored  = FALSE;

	do
	{
		/* Send SID 4 request */
		SidReq.SID = 4;
		SidReq.NumIds = 0;
		eReturnCode = SidRequest(&SidReq, SID_REQ_ALLOW_NO_RESPONSE);

		/* Delay after request to allow time for codes to clear */
		Sleep (CLEAR_CODES_DELAY_MSEC);

		if (eReturnCode != FAIL)
		{
			eReturnCode = PASS;  // mark it as PASS, since ERROR can get here too

			if (gOBDNumEcusResp == 0)
			{
				if  (gOBDEngineRunning == FALSE)
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "No response to SID $4 request\n");
					eReturnCode = FAIL;
				}
				else
				{
					Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "No response to SID $4 request\n");
				}
			}
			else
			{
				/* all ECUs must respond with the same ackowledgement (positive or negative) with engine running */
				for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
				{
					if (gOBDResponse[EcuIndex].Sid4Size > 0)
					{
						if (gOBDResponse[EcuIndex].Sid4[0] == 0x44)
						{
							usPositiveAckCount++;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Positive response to SID $4 request\n",
							     GetEcuId(EcuIndex) );
						}
						else
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Negative response to SID $4 request\n",
							     GetEcuId(EcuIndex) );
						}
					}
				}

				/* Check that all ECUs response the same (all positive or all negative) with engine running */
				if ( (usPositiveAckCount > 0) && (usPositiveAckCount < gOBDNumEcusResp) )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						 "Received both positive and negative responses\n" );
					eReturnCode = FAIL;
				}
			}
		}
	}
	while ( ++usRetryCount < 2 && usPositiveAckCount == 0 );

	return(eReturnCode);
}
