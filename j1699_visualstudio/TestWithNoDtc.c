
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
** DATE          MODIFICATION
** 05/01/04      Renumber all test cases to reflect specification.  This section
**               has been indicated as section 5 in Draft 15.4.
** 05/10/04      Test case 5.19, replaced existing link active logic with
**               routine call to 'VerifyLinkActive'.  Function will verity
**               response from total number of controllers.
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
** TestWithNoDtc -
** Function to run test with no malfunction, no DTC set
*******************************************************************************
*/
STATUS TestWithNoDtc(void)
{
	BOOL          bSubTestFailed = FALSE;


//*******************************************************************************
	/* Do MIL light check */
	TestSubsection = 1;
	gOBDTestSubsectionFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(MIL bulb check, Engine Off, No DTC set)");
	if ( CheckMILLight() != PASS || gOBDTestSubsectionFailed == TRUE )
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
	/* Determine the OBD protocol to use */
	TestSubsection = 2;
	gOBDTestSubsectionFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	      "(Determine Protocol, Ignition On, Engine Off)");
	
	/*
	if ( DetermineProtocol() != PASS )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Protocol determination unsuccessful.\n" );
		Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
		return(FAIL);
	}
	else if (
		modified (VerifyVehicleState(gOBDEngineRunning, gOBDHybridFlag) != PASS) ||
		modified (gOBDTestSubsectionFailed == TRUE)
		False)
	{
		if (
		     modified (Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT, "")) == 'N'
		   False)
		{
			return(FAIL);
		}
	}
	else
	{
		Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
	}
	*/
	//modified. put just below
	Log(SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
//*******************************************************************************
	/* Clear codes */
	TestSubsection = 3;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Clear DTCs (Service $04), Engine Off)");
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
			return(FAIL);
		}
	}
	else
	{
		Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
	}


//*******************************************************************************
	/* Verify MIL and DTC status is cleared */
	TestSubsection = 4;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify MIL status bit, Verify no Permanent DTCs, Engine Off)");
	if ( VerifyMILData() != PASS )
	{
		bSubTestFailed = TRUE;
	}

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
	/* Verify monitor test support and results */
	TestSubsection = 5;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Monitor Test Results (Service $06), Engine Off)");
	if ( VerifyMonitorTestSupportAndResults() != PASS )
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
	/* Verify diagnostic support and data */
	TestSubsection = 6;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Diagnostic Data (Service $01), Engine Off)");
	if ( VerifyDiagnosticSupportAndData( FALSE ) != PASS )
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
	/* Verify control support and data */
	TestSubsection = 7;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Control of System, Test or Component (Service $08), Engine Off)");
	if ( VerifyControlSupportAndData() != PASS )
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
	/* Tell user to start engine */
	TestSubsection = 8;
	gOBDTestSubsectionFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Determine Protocol, Engine Running)");

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Start engine and let idle for one (1) minute.\n\n"
	     "Note: Some powertrain control systems have engine controls that can start and\n"
	     "stop the engine without regard to ignition position.\n" );
	gOBDEngineRunning = TRUE;

	/* Determine the OBD protocol to use */
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
	/* Clear codes */
	TestSubsection = 9;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Clear DTCs (Service $04), Engine Running)");
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
			return(FAIL);
		}
	}
	else
	{
		Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
	}


//*******************************************************************************
	/* Verify diagnostic support and data */
	TestSubsection = 10;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Diagnostic Data (Service $01), Engine Running)");
	if ( VerifyDiagnosticSupportAndData( FALSE ) != PASS )
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
	TestSubsection = 11;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Freeze Frame Data (Service $02), Engine Running)");
	if ( VerifyFreezeFrameSupportAndData() != PASS )
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
	TestSubsection = 12;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Stored DTCs (Service $03), Engine Running)");
	if ( VerifyDTCStoredData() != PASS )
	{
		bSubTestFailed = TRUE;
	}

	if ( VerifyMILData() != PASS )
	{
		bSubTestFailed = TRUE;
	}

	if ( VerifyVehicleState(gOBDEngineRunning, gOBDHybridFlag) != PASS )
	{
		bSubTestFailed = TRUE;
	}

	if ( gOBDTestSubsectionFailed == TRUE || bSubTestFailed == TRUE)
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
	/* Verify O2 monitor test results */
	TestSubsection = 13;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Oxygen Sensor Monitoring Test Results (Service $05), Engine Running)");
	if ( VerifyO2TestResults() != PASS )
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
	/* Verify monitor test support and results */
	TestSubsection = 14;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Monitor Test Results (Service $06), Engine Running)");
	if ( VerifyMonitorTestSupportAndResults() != PASS )
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
	/* Verify pending DTC data */
	TestSubsection = 15;
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
	/* Verify control support and data */
	TestSubsection = 16;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Control of System, Test or Component (Service $08), Engine Running)");
	if ( VerifyControlSupportAndData() != PASS )
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
	/* Verify vehicle information support and data */
	TestSubsection = 17;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Vehicle Information (Service $09), Engine Running)");
	if ( VerifyVehicleInformationSupportAndData() != PASS )
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
	/* Verify reverse order support */
	TestSubsection = 18;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Diagnostic Data (Service $01) in reverse order)");
	if ( VerifyReverseOrderSupport() != PASS )
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
	/* Verify tester present support */
	TestSubsection = 19;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Diagnostic Data (Service $01) idle message timing)");

	/* Sleep for 15 seconds, then make sure the tester present messages kept the link alive */
	Sleep(15000);

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
	/* Verify diagnostic burst support */
	TestSubsection = 20;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Diagnostic Data (Service $01) burst message timing)" );
	if ( VerifyDiagnosticBurstSupport() != PASS )
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
	/* Verify Reserved Services */
	TestSubsection = 21;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Reserved/Unused Services, Engine Running)" );
	if ( VerifyReservedServices() != PASS )
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
