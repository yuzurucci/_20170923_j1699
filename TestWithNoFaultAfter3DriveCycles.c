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
*               has been indicated as section 9 in Draft 19.4.
********************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include "j2534.h"
#include "j1699.h"

/*
*******************************************************************************
** TestWithNoFaultsAfter3DriveCycles - Tests 9.x
** Function to run test with no faults after 3 drive cycles
*******************************************************************************
*/
STATUS TestWithNoFaultsAfter3DriveCycles(void)
{
	BOOL    bSubTestFailed = FALSE;
	STATUS  RetCode;
	unsigned char i;
	char chUserResponse;


//*******************************************************************************
	TestSubsection = 1;
	gOBDTestSubsectionFailed = FALSE;

	/* Prompt user to complete the second and perform the third drive cycle */
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Complete Two or More Additional Drive Cycles)");

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Turn key OFF for 30 seconds or longer, as appropriate for the ECU\n"
	     "(This completes two driving cycles)." );

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Start engine and let idle for 1 minute or whatever time it takes\n"
	     "to run the monitor and detect that there is no malfunction.\n"
	     "(This starts third driving cycle; however, third driving cycle will not be\n"
	     "completed until key is turned off).\n\n"
	     "Note: Some powertrain control systems have engine controls that can start and\n"
	     "stop the engine without regard to ignition position.\n" );

	do
	{
		Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
		     "Turn key OFF for 30 seconds or longer, as appropriate for the ECU\n"
		     "(This completes the current driving cycles with no fault)." );

		Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
		     "Start engine and let idle for 1 minute or whatever time it takes\n"
		     "to run the monitor and detect that there is no malfunction.\n"
		     "(This starts another driving cycle; however, the driving cycle will not be\n"
		     "completed until key is turned off).\n\n"
		     "Note: Some powertrain control systems have engine controls that can start and\n"
		     "stop the engine without regard to ignition position.\n" );
	}
	while (
	        Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_PROMPT,
	             "The MIL should now be OFF.  If it is, the test will continue to\n"
	             "Test 9.2.\n"
	             "If the MIL is still ON, additional driving cycles may be performed\n"
	             "until MIL is off.\n\n"
	             "Is MIL OFF?") != 'Y'
	      );

	gOBDEngineRunning = TRUE;

	Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");


//*******************************************************************************
	/* Set flag to indicate DTC should be historical */
	gOBDDTCHistorical = TRUE;

	/* Set flag to indicate no permanent DTC */
	gOBDDTCPermanent = FALSE;

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
	/* Verify pending DTC data */
	TestSubsection = 3;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Pending DTCs (Service $07), Engine Running)");
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


//*******************************************************************************
	/* Verify Permanent Codes Drive Cycle */
	TestSubsection = 7;
	gOBDTestSubsectionFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Complete Static Test or Continue to Permanent Code Drive Cycle)" );
	if ( gService0ASupported != 0 )
	{
		for (i = 0; i < gService0ASupported; i++)
		{
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Service $0A is supported by:  ECU %02X\n", gSID0ASupECU[i]  );
		}

		if (
		     Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_PROMPT,
		          "Do you wish to run the CARB drive cycle portion for Permanent DTCs?") == 'Y'
		   )
		{
			Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");

			/* Verify permanent codes */
			if ( (RetCode = TestToVerifyPermanentCodes()) != PASS )
			{
				return(RetCode);
			}

			/* Verify permanent code support */
			TestSubsection = 22;
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
		}

		else
		{
			Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Service $0A was supported but the operator chose not to run\n"
			     "the Permanent Code Drive Cycle test.\n\n" );

			Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
		}
	}
	/* Service $0A not supported */
	/* IF US, warn if SID $A not supported */
	else if ( gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD )
	{
		Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Service $0A was not supported by any ECU, therefore\n"
		     "the Permanent Code Drive Cycle was not tested.\n\n" );

		Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
	}
	/* IF not US, just note that SID $A not supported and Permanent Code Drive Cycle not tested */
	else
	{
		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Service $0A was not supported by any ECU, therefore\n"
		     "the Permanent Code Drive Cycle was not tested.\n\n" );

		Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
	}


//*******************************************************************************
	/* Clear codes */
	if (
	     (chUserResponse = Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_PROMPT,
	                            "You may now clear codes or exit Test 9.\n"
	                            "If you choose to clear all diagnostic information,\n"
	                            "you will then have the option of clearing codes with engine running.\n\n"
	                            "Clear all diagnostic information?") ) == 'Y'
	   )
	{
		TestSubsection = 23;
		gOBDTestSubsectionFailed = FALSE;
		bSubTestFailed = FALSE;
		Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "(Clear DTCs (Service $04))" );

		if (
		     Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_PROMPT,
		          "Clear codes with engine running?") != 'Y'
		   )
		{
			/* Clear Codes Engine OFF */
			Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
			     "Turn key OFF for 30 seconds or longer, as appropriate for the ECU.\n" );
			Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
			     "Turn key ON with engine OFF. Do not crank engine.\n" );

			gOBDEngineRunning = FALSE;

			if ( DetermineProtocol() != PASS )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "Protocol determination unsuccessful.\n" );
				Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
				Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,"Turn key OFF" );
				return(FAIL);
			}
			else
			{
				Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "Protocol determination passed.\n" );
			}
		}
	}

	if (chUserResponse == 'Y')
	{
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
			if ( Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT, "") == 'N' )
			{
				Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT, "Turn key OFF" );
				return(FAIL);
			}
		}
		else
		{
			Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
		}
	}
	else
	{
		/* Don't clear Codes, EXIT Test 9 */
		Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT, "Turn key OFF" );

		Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
	}


	return(PASS);
}
