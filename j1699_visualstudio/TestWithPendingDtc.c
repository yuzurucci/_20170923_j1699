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
* DATE          MODIFICATION
* 05/01/04      Renumber all test cases to reflect specification.  This section
*               has been indicated as section 6 in Draft 15.4.
********************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <conio.h>
#include "j2534.h"
#include "j1699.h"
/*
*******************************************************************************
** TestWithPendingDtc - Tests 6.x
** Function to run test with a pending code induced by a fault
*******************************************************************************
*/
STATUS TestWithPendingDtc(void)
{
	BOOL          bSubTestFailed = FALSE;
	BOOL          bLoopDone = FALSE;

//*******************************************************************************
	TestSubsection = 1;
	gOBDTestSubsectionFailed = FALSE;

	/* Prompt user to induce fault and start engine */
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Induce Fault)");

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Turn key off and disconnect a sensor (e.g. ECT, TP, IAT, MAF, etc.)\n"
	     "that will generate a MIL light with the engine idling for one ECU.\n"
	     "The selected fault must illuminate the MIL using %s driving cycles\n"
	     "(not one like GM Type A) to allow for proper testing of Mode 7 and\n"
	     "freeze frame", gUserInput.szNumDCToSetMIL );

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Start engine and let idle.\n\n"
	     "NOTE: Some powertrain control systems have engine controls that can start and\n"
	     "stop the engine without regard to ignition position.\n" );

	gOBDEngineRunning = TRUE;

	Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");


//*******************************************************************************
	/* Determine the OBD protocol to use */
	TestSubsection = 2;
	gOBDTestSubsectionFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Determine Protocol, Engine Running)");
	if ( DetermineProtocol() != PASS )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Protocol determination unsuccessful.\n" );
		Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
		return(FAIL);
	}
	else if (
	          (VerifyVehicleState(gOBDEngineRunning, gOBDHybridFlag) != PASS) ||
	          (gOBDTestSubsectionFailed == TRUE)
	        )
	{
		if ( Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT, "") == 'N' )
		{
			return(FAIL);
		}
	}
	else
	{
		Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
	}


//*******************************************************************************
	TestSubsection = 3;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Pending DTCs (Service $07), Engine Running)");

	/* flush the STDIN stream of any user input before loop */
	clear_keyboard_buffer ();

	/* Wait for pending DTC */
	Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTOFF, NO_PROMPT,
	     "Waiting for pending DTC...(press any key to stop test)\n");
	while (!kbhit())
	{
		if (IsDTCPending((SID_REQ_NORMAL|SID_REQ_ALLOW_NO_RESPONSE)) == PASS)
		{
			bLoopDone = TRUE;
			break;
		}
		Sleep(500);
	}

	if (bLoopDone == FALSE)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "User abort\n");
	}

	/* Beep */
	printf("\007\n");

	/* Flush the STDIN stream of any user input above */
	clear_keyboard_buffer ();

	/* Set flag to indicate a pending DTC should be present */
	gOBDDTCPending = TRUE;

	/* Verify there is a pending DTC */
	if ( IsDTCPending((SID_REQ_NORMAL|SID_REQ_ALLOW_NO_RESPONSE)) != FAIL )
	{
		/* Get DTC list from SID $07 */
		SaveDTCList (7);

		/* Check if a DTC is stored as well (i.e. GM Type A) */
		if (IsDTCStored((SID_REQ_NORMAL|SID_REQ_ALLOW_NO_RESPONSE)) == PASS)
		{
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Type A DTC detected, pending without a stored code functionality not tested\n");
			gOBDDTCStored = TRUE;
		}
	}
	else
	{
		bSubTestFailed = TRUE;
	}

	/* Verify pending DTC data */
	if ( VerifyDTCPendingData() != PASS )
	{
		bSubTestFailed = TRUE;
	}

	if ( VerifyVehicleState(gOBDEngineRunning, gOBDHybridFlag) != PASS )
	{
		bSubTestFailed = TRUE;
	}

	if ( (gOBDTestSubsectionFailed == TRUE) || (bSubTestFailed == TRUE) )
	{
		if ( Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT, "") == 'N' )
		{
			return(FAIL);
		}
	}
	else
	{
		Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
	}


//*******************************************************************************
	/* Verify stored DTC data */
	TestSubsection = 4;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Stored DTCs (Service $03), Engine Running)");
	if ( VerifyDTCStoredData() != PASS )
	{
		bSubTestFailed = TRUE;
	}

	/* Verify MIL and DTC status is cleared */
	if ( VerifyMILData() != PASS )
	{
		bSubTestFailed = TRUE;
	}

	if ( VerifyVehicleState(gOBDEngineRunning, gOBDHybridFlag) != PASS )
	{
		bSubTestFailed = TRUE;
	}

	if ( (gOBDTestSubsectionFailed == TRUE) || (bSubTestFailed == TRUE) )
	{
		if ( Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT, "") == 'N' )
		{
			return(FAIL);
		}
	}
	else
	{
		Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
	}


//*******************************************************************************
	/* Verify freeze frame support and data */
	TestSubsection = 5;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Freeze Frame Data (Service $02), Engine Running)");
	if ( VerifyFreezeFrameSupportAndData() != PASS )
	{
		bSubTestFailed = TRUE;
	}

	/* Link active test to verify communication remained active for ALL protocols */
	if ( VerifyLinkActive() != PASS )
	{
		bSubTestFailed = TRUE;
	}

	if ( VerifyVehicleState(gOBDEngineRunning, gOBDHybridFlag) != PASS )
	{
		bSubTestFailed = TRUE;
	}

	if ( (gOBDTestSubsectionFailed == TRUE) || (bSubTestFailed == TRUE) )
	{
		if ( Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT, "") == 'N' )
		{
			return(FAIL);
		}
	}
	else
	{
		Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
	}


//*******************************************************************************
	if ( gUserInput.eComplianceType != US_OBDII &&
	     gUserInput.eComplianceType != HD_OBD )
	{
		/* Continue to Induce Circuit Fault (EOBD/IOBD/OBDBr only) */
		TestSubsection = 6;
		gOBDTestSubsectionFailed = FALSE;
		bSubTestFailed = FALSE;
		Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "(Continue to induce fault - EOBD/IOBD/OBDBr only)");

		Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
		     "Turn key OFF for 30 seconds or longer, as appropriate for the ECU,\n"
		     "and keep sensor disconnected.\n" );

		Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
		     "Start engine and let idle for 1 minute or whatever time it takes to set\n"
		     "a pending DTC.\n\n"
		     "NOTE: Some powertrain control systems have engine controls that can start and\n"
		     "stop the engine without regard to ignition position.\n" );

		if ( (gOBDTestSubsectionFailed == TRUE) || (bSubTestFailed == TRUE) )
		{
			if ( Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT, "") == 'N' )
			{
				return(FAIL);
			}
		}
		else
		{
			Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
		}
	}

	return(PASS);
}


//*****************************************************************************
//
//	Function:	SaveDTCList
//
//	Purpose:	Save the DTC list from SID $03 or $07
//
//*****************************************************************************
void SaveDTCList (int nSID)
{
	unsigned long  EcuIndex;
	unsigned long  index;
	unsigned long  DTCIndex;
	unsigned short size;

	if (nSID != 3 && nSID != 7)
		return;

	for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
	{
		size = gDTCList[EcuIndex].Size = (nSID == 3) ? gOBDResponse[EcuIndex].Sid3Size
		                                             : gOBDResponse[EcuIndex].Sid7Size;
		for (index = 0; index < size; index++)
			gDTCList[EcuIndex].DTC[index] = (nSID == 3) ? gOBDResponse[EcuIndex].Sid3[index]
			                                            : gOBDResponse[EcuIndex].Sid7[index];
	}

	/* If this is Test 7.4 and there is a pending DTC, append to DTC list */
	if ( nSID == 3 &&
	     TestPhase == eTestConfirmedDTC &&
	     IsDTCPending((SID_REQ_NORMAL|SID_REQ_ALLOW_NO_RESPONSE)) != FAIL )
	{
		/* Get DTC list from SID $07 */
		for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
		{
			size = gOBDResponse[EcuIndex].Sid7Size;
			DTCIndex = gDTCList[EcuIndex].Size;
			gDTCList[EcuIndex].Size += gOBDResponse[EcuIndex].Sid7Size;
			for ( index = 0; index < size; index++, DTCIndex++ )
				gDTCList[EcuIndex].DTC[DTCIndex] = gOBDResponse[EcuIndex].Sid7[index];
		}
	}
}
