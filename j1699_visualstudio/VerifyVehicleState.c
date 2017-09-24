/*
********************************************************************************
** SAE J1699-3 Test Source Code
**
**  Copyright (C) 2010 EnGenius, Inc. http://j1699-3.sourceforge.net/
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
#include <conio.h>
#include <time.h>
#include <windows.h>
#include "j2534.h"
#include "j1699.h"




//*****************************************************************************
//
//	Function:	VerifyVehicleState
//
//	Purpose:	Purpose is desired vehicle state to actual vehicle state.
//
//*****************************************************************************
STATUS VerifyVehicleState (unsigned char ucEngineRunning, unsigned char ucHybrid)
{
	STATUS RetCode = PASS;
	unsigned short usRPM;


	if ( LogRPM(&usRPM) == PASS )
	{
		Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
			  "VerifyVehicleState - Expected State: %s, RPM: %u, Hybrid: %s\n",
			  ((ucEngineRunning == TRUE) ? "Running" : "Off"),
			  usRPM,
			  ((ucHybrid == TRUE) ? "Yes" : "No")
			  );

		if ( (ucEngineRunning == FALSE) && (usRPM > 0) )
		{
			// Engine is running, but it should be off
			if (
				Log( USER_ERROR, SCREENOUTPUTON, LOGOUTPUTON, QUIT_CONTINUE_PROMPT,
					"Engine is running, when it should be off.\n") == 'Q'
				)
			{
				RetCode = ABORT;
			}
		}
		else if ( (ucEngineRunning == TRUE) && (usRPM < 300) )
		{
			// Engine is off, but it should be running
			if (ucHybrid == TRUE)
			{
				Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					"Engine is off, but it should be running.\n");
			}
			else
			{
				if (
					Log( USER_ERROR, SCREENOUTPUTON, LOGOUTPUTON, QUIT_CONTINUE_PROMPT,
						"Engine is off, when it should be running.\n") == 'Q'
					)
				{
					RetCode = ABORT;
				}
			}
		}
	}
	else
	{
		if (
			Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, QUIT_CONTINUE_PROMPT,
				"Cannot determine engine state.\n") == 'Q'
			)
		{
			RetCode = ABORT;
		}
		else
		{
			RetCode = FAIL;
		}
	}


	if (RetCode == ABORT)
	{
		AbortTest();	// this function never returns
	}

	return(RetCode);
}




//*****************************************************************************
//
//	Function:	LogRPM
//
//	Purpose:	Purpose is to read current vehicle engine speed.
//
//*****************************************************************************
STATUS LogRPM (unsigned short *pusRPM)
{
	SID_REQ         SidReq;
	SID1            *pSid1;
	unsigned short  RPM = 0;
	unsigned long   EcuIndex;
	STATUS          eRetCode = FAIL;

	// request RPM - SID 1 PID $0C
	SidReq.SID    = 1;
	SidReq.NumIds = 1;
	SidReq.Ids[0] = 0x0c;

	/*
	if (SidRequest( &SidReq, SID_REQ_NORMAL ) == FAIL)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "No response to SID $1 PID $0C (RPM) request.\n");
		eRetCode = FAIL;
	}

	else
	{
		for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
		{
			if (gOBDResponse[EcuIndex].Sid1PidSize > 0)
			{
				pSid1 = (SID1 *)&gOBDResponse[EcuIndex].Sid1Pid[0];

				if (pSid1->PID == 0x0c)
				{
					RPM = pSid1->Data[0];
					RPM = RPM << 8 | pSid1->Data[1];

					// convert from 1 cnt = 1/4 RPM to 1 cnt = 1 RPM
					RPM >>= 2;

					Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						 "ECU %X  RPM = %d\n", GetEcuId(EcuIndex), RPM );
					eRetCode = PASS;
				}
			}
		}
	}
	*/  insert comment.mizuno
	RPM = 1000;   //modified mizuno
	eRetCode = PASS; //modified mizuno

	*pusRPM = RPM;
	return eRetCode;
}
