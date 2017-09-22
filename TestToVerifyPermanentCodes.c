/*
********************************************************************************
** SAE J1699-3 Test Source Code
**
**  Copyright (C) 2007. http://j1699-3.sourceforge.net/
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
#include <string.h>
#include <time.h>
#include <windows.h>
#include "j2534.h"
#include "j1699.h"

/* These Test 10.10 functions and variable are reused
   here in Test 9.19 */
extern SID9IPT Test10_10_Sid9Ipt[OBD_MAX_ECUS];
extern STATUS VerifyIPTData (BOOL *pbIPTSupported);
extern STATUS RunDynamicTest10 (unsigned long tEngineStartTimeStamp);

/*
*******************************************************************************
** TestToVerifyPermanentCodes - Tests 9.x
** Function to run test to read permanent codes
*******************************************************************************
*/
STATUS TestToVerifyPermanentCodes(void)
{
	unsigned long tEngineStartTimeStamp;
	STATUS        ret_code;
	BOOL          bIPTSupported = FALSE;

	BOOL          bSubTestFailed = FALSE;

//*******************************************************************************
	TestSubsection = 8;
	gOBDTestSubsectionFailed = FALSE;

	/* Prompt user to induce fault and set pending DTC */
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Induce Circuit Fault to Set Pending DTC)" );

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "With engine and ignition off, disconnect a sensor that is tested\n"
	     "continuously (e.g., ECT, TP, IAT, MAF, etc.) to set a fault\n"
	     "that will generate a MIL light and a single DTC on only one ECU\n"
	     "with the engine idling in a short period of time (i.e. < 10 seconds).\n\n"
	     "The selected fault must illuminate the MIL using %s driving cycles,\n"
	     "not one driving cycle (like GM \"Type A\" DTC) to allow proper testing\n"
	     "of Service 07 and freeze frame.\n\n"
	     "NOTE: It is acceptable to use a fault that sets in one driving cycle.\n"
	     "If this is the case, a pending DTC, a confirmed DTC, and MIL\n"
	     "will be set on the first driving cycle.\n", gUserInput.szNumDCToSetMIL );

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Start engine, let idle for one minute or whatever time it takes to set\n"
	     "a pending DTC.\n\n"
	     "NOTE: Some powertrain control systems have engine controls that can start and\n"
	     "stop the engine without regard to ignition position.\n" );

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Turn ignition OFF (engine OFF) for 30 seconds or longer, as appropriate\n"
	     "for the ECU, and keep sensor disconnected.\n" );

	if ( gUserInput.eComplianceType != US_OBDII &&
	     gUserInput.eComplianceType != HD_OBD )
	{
		Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
		     "Start engine, let idle for one minute or whatever time it takes to set\n"
		     "a confirmed DTC. The MIL may or may not illuminate on this driving cycle.\n\n"
		     "NOTE: Some powertrain control systems have engine controls that can start and\n"
		     "stop the engine without regard to ignition position.\n" );

		Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
		     "Turn ignition OFF (engine OFF) for 30 seconds or longer, as appropriate\n"
		     "for the ECU, and keep sensor disconnected.\n" );
	}

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Start engine, let idle for one minute or whatever time it takes to set\n"
	     "a confirmed DTC and illuminate the MIL.\n\n"
	     "NOTE: Some powertrain control systems have engine controls that can start and\n"
	     "stop the engine without regard to ignition position.\n" );

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Turn ignition OFF (engine OFF) for 30 seconds or longer, as appropriate\n"
	     "for the ECU, and keep sensor disconnected.\n"
	     "(This completes the driving cycle and allows permanent code to be set).\n" );

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Turn ignition on. DO NOT crank engine.\n");

	/* Engine should now be not running */
	gOBDEngineRunning = FALSE;

	Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");


//*******************************************************************************
	/* Establish communication, engine off */
	TestSubsection = 9;
	gOBDTestSubsectionFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Determine Protocol, Engine Off)" );
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
		if ( (Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT, "")) == 'N' )
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

	/* Request DTC, engine off */
	TestSubsection = 10;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Stored DTCs (Service $03), Engine Off)" );
	if ( VerifyDTCStoredData() != PASS )
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
	/* Verify stored DTC data, engine off */
	TestSubsection = 11;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Permanent DTCs (Service $0A), Engine Off)" );
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


//*******************************************************************************
	TestSubsection = 12;
	gOBDTestSubsectionFailed = FALSE;

	/* Prompt user to induce fault to be able to retrieve permanent DTC */
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Repair Circuit Fault)" );

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Turn ignition OFF (engine OFF) for 30 seconds or longer, as appropriate\n"
	     "for the ECU." );

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT, "Connect sensor.");

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Turn ignition on.  DO NOT crank engine.\n");

	Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");


//*******************************************************************************
	/* Establish communication, engine off */
	TestSubsection = 13;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Determine Protocol and Clear DTCs (Service $04), Engine Off)" );
	if ( DetermineProtocol() != PASS )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Protocol determination unsuccessful.\n" );
		Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
		return(FAIL);
	}

	if ( ClearCodes() != PASS )
	{
		bSubTestFailed = TRUE;
	}

	if ( VerifyVehicleState(gOBDEngineRunning, gOBDHybridFlag) != PASS )
	{
		bSubTestFailed = TRUE;
	}

	if ( (gOBDTestSubsectionFailed == TRUE) || (bSubTestFailed == TRUE) )
	{
		if ( (Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT, "")) == 'N' )
		{
			return(FAIL);
		}
	}
	else
	{
		Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
	}


//*******************************************************************************
	/* Verify MIL status, engine off */
	TestSubsection = 14;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify MIL Status, Engine Off)" );
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
	/* Verify stored DTC data, engine off */
	TestSubsection = 15;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Permanent DTCs (Service $0A), Engine Off)" );
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


//*******************************************************************************
	TestSubsection = 16;
	gOBDTestSubsectionFailed = FALSE;

	/* Prompt user to repair circuit fault and complete one drive cycle */
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Complete One Driving Cycle With Fault Repaired)" );

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Start engine, let idle for one minute or whatever time it takes\n"
	     "to run monitor and detect that there is no malfunction\n"
	     "(Monitor may have already run with engine off).\n\n"
	     "NOTE: Some powertrain control systems have engine controls that can start and\n"
	     "stop the engine without regard to ignition position.\n" );

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Turn ignition OFF (engine OFF) for 30 seconds or longer, as appropriate\n"
	     "for the ECU\n"
	     "(This completes one driving cycle with no fault).\n");

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Start engine and REMAIN AT IDLE until directed to start drive cycle\n" );

	tEngineStartTimeStamp = GetTickCount ();

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Continue to idle until the ECU detects that there is no malfunction.\n"
	     "(This starts the second driving cycle; however, second driving cycle\n"
	     "will not be complete until key is turned off.)\n\n"
	     "NOTE: Some powertrain control systems have engine controls that can start and\n"
	     "stop the engine without regard to ignition position.\n" );

	/* Engine should now be running */
	gOBDEngineRunning = TRUE;

	Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");


//*******************************************************************************
	/* Establish communication, engine running */
	TestSubsection = 17;
	gOBDTestSubsectionFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Determine Protocol, Engine Running)" );
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
		if ( (Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT, "")) == 'N' )
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

	/* Verify stored DTC data, engine off */
	TestSubsection = 18;
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


//*******************************************************************************
	/* User prompt, run test */
	TestSubsection = 19;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Complete Permanent Code Drive Cycle)");

	/* Initialize Array */
	memset (Test10_10_Sid9Ipt, 0x00, sizeof(Test10_10_Sid9Ipt));

	if ( VerifyIPTData(&bIPTSupported) != PASS || gOBDTestSubsectionFailed == TRUE )
	{
		if ( Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT,
	              "Unable to capture Initial OBDCOND and IGNCNT.\n") == 'N' )
		{
			Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
			return(FAIL);
		}

		bSubTestFailed = TRUE;
		gOBDTestSubsectionFailed = FALSE;
	}

	// stop tester-present message
	StopPeriodicMsg (TRUE);
	Sleep (gOBDRequestDelay);

	gSuspendScreenOutput = TRUE;
	ret_code = RunDynamicTest10 (tEngineStartTimeStamp);
	gSuspendScreenOutput = FALSE;

	// re-start tester-present message
	StartPeriodicMsg ();

	if ( VerifyVehicleState(gOBDEngineRunning, gOBDHybridFlag) != PASS )
	{
		bSubTestFailed = TRUE;
	}

	if ( ret_code == FAIL || gOBDTestSubsectionFailed == TRUE || bSubTestFailed == TRUE )
	{
		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Errors detected during dynamic test.  View Logfile for more details.\n");

		if ( Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT, "") == 'N' )
		{
			return FAIL;
		}
	}
	else
	{
		Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
	}

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Stop the vehicle in a safe location and turn the ignition off.\n");

	/* Engine should now be not running */
	gOBDEngineRunning = FALSE;


//*******************************************************************************
	TestSubsection = 20;
	gOBDTestSubsectionFailed = FALSE;

	/* Prompt user to induce fault to be able to retrieve permanent DTC */
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Key On)" );

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Turn ignition on and start engine.\n");

	/* Engine should now be  running */
	gOBDEngineRunning = TRUE;

	Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");


//*******************************************************************************

	gOBDDTCPermanent = FALSE;

	/* Establish communication, engine off */
	TestSubsection = 21;
	gOBDTestSubsectionFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Determine Protocol, Engine Running)" );
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
		if ( (Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT, "")) == 'N' )
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
