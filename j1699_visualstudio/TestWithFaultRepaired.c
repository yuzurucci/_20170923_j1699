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
*               has been indicated as section 8 in Draft 15.8.
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
** TestWithFaultRepaired - Tests 8.x
** Function to run test with fault repaired
*******************************************************************************
*/
STATUS TestWithFaultRepaired(void)
{
	BOOL          bSubTestFailed = FALSE;
	BOOL          bLoopDone = FALSE;

//*******************************************************************************
	TestSubsection = 1;
	gOBDTestSubsectionFailed = FALSE;

	/* Prompt user to fix fault and perform first two drive cycles */
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
         "(Repair Fault and Complete One Drive Cycle, MIL Illuminated)");

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	               "Turn key OFF for 30 seconds or longer, as appropriate for the ECU" );

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	               "Reconnect sensor" );

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	               "Start engine and let idle for whatever time it takes to run the monitor\n"
	               "and detect that there is no malfunction.\n\n"
	               "Note: Some powertrain control systems have engine controls that can start and\n"
	               "stop the engine without regard to ignition position.\n" );

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	               "Turn key OFF for 30 seconds or longer, as appropriate for the ECU\n"
	               "(this completes one driving cycle)." );

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	               "Start engine and let idle for whatever time it takes to run the monitor\n"
	               "and detect that there is no malfunction.\n\n"
	               "Note: Some powertrain control systems have engine controls that can start and\n"
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
	/* Check for a pending DTC */
	TestSubsection = 3;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
         "(Verify Pending DTCs (Service $07), Engine Running)");

	/* flush the STDIN stream of any user input before loop */
	clear_keyboard_buffer ();

	Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTOFF, NO_PROMPT,
		 "Waiting for pending DTC to clear...(press any key to stop test)\n");
	while (!kbhit())
	{
		if (IsDTCPending((SID_REQ_NORMAL|SID_REQ_ALLOW_NO_RESPONSE)) == FAIL)
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

	/* Set flag to indicate a pending DTC should NOT be present */
	gOBDDTCPending = FALSE;

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
	if ( VerifyDTCStoredData() != PASS || gOBDTestSubsectionFailed == TRUE )
	{
		if ( Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT,
             "Verify DTC stored data unsuccessful.\n" ) == 'N' )
		{
    	    Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
			return(FAIL);
		}
		bSubTestFailed = TRUE;
		gOBDTestSubsectionFailed = FALSE;
	}

	/* Verify MIL and DTC status is cleared */
	if (Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_PROMPT, "Is MIL light ON?") != 'Y')
	{
		bSubTestFailed = TRUE;
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
             "MIL light check.\n" );
	}

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

	gOBDDTCPermanent = TRUE;

	/* Verify permanent codes */
	TestSubsection = 6;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
         "(Verify Permanent DTCs (Service $0A), Engine Running)" );
	if ( VerifyPermanentCodeSupport() != PASS )
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

	return(PASS);
}
