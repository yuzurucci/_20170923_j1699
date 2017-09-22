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
*               has been indicated as section 10 in Draft 15.4.
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
#include "ScreenOutput.h"

STATUS VerifyIPTData (BOOL *pbIPTSupported);
STATUS LogSid9Inf12 ( SID9IPT pSid9Ipt[] );
STATUS VerifySid9Inf12 ( SID9IPT *pSid9Ipt );
STATUS ReadVIN (void);
STATUS PrintCALIDs (void);
STATUS PrintCVNs (void);
STATUS StartLogFile (BOOL *pbTestReEntered);
STATUS RunDynamicTest10 (unsigned long tEngineStartTimeStamp);
void DisplayDynamicTest10Results ( void );
extern STATUS VerifyInf12Format (unsigned long EcuIndex);

/* Initial data for Test 11.x. Declared in TestToVerifyPerformanceCounters.c */
extern SID9IPT Test10_10_Sid9Ipt[OBD_MAX_ECUS];

/*
*******************************************************************************
** Test 10.x dynamic screen mapping
*******************************************************************************
*/

#define SPACE             7
#define ECU_WIDTH         7
#define NUM_WIDTH         5
#define STRING_WIDTH      7

#define COL1              18

#define IDLE_TIME_ROW     15
#define SPEED_ROW         15
#define RUN_TIME_ROW      16
#define TOTAL_TIME_ROW    17
#define PHEV_TIME_ROW     18
#define TEST_STATUS_ROW   18
#define ECU_ID_ROW        20
#define INI_OBD_COND_ROW  22
#define CUR_OBD_COND_ROW  23
#define INI_IGN_CNT_ROW   25
#define CUR_IGN_CNT_ROW   26
#define INI_FEO_CNT_ROW   28
#define CUR_FEO_CNT_ROW   29
#define BOTTOM_ROW        31

StaticTextElement  _string_elements_1st_9[] =
{
	{"Drive the vehicle in the following manner, so that the OBD Condition Counter will increment:", 1, 0},
};

StaticTextElement  _string_elements_1st_10[] =
{
	{"Drive the vehicle in the following manner, at an altitude < 8000 ft", 1, 0},
	{"(BARO < 22 in Hg) and ambient temperature > or = 20 deg F,", 1, 1},
	{"so that the OBD Condition Counter will increment:", 1, 2},
};

StaticTextElement  _string_elements10[] =
{
	{"NOTE: Some powertrain control systems have engine controls that can start and", 1, 4},
	{"stop the engine without regard to ignition position.", 1, 5},

	{"- Continuous time > or = 30 seconds with vehicle speed < or = 1 MPH", 1, 7},
	{"  and accelerator pedal released.", 2, 8},
	{"- Cumulative time > or = 300 seconds with vehicle speed > 25MPH/40KPH.", 1, 9},
	{"- Cumulative time since engine start > or = 600 seconds.", 1, 10},
	{"- Cumulative fueled engine time since engine start > or = 10 seconds for PHEV.", 1, 11},

	{"OBD Drive Cycle Status", 1, 13},

	{" 30 Seconds Idle Timer:", 1, IDLE_TIME_ROW},
	{"300 Seconds at speeds > 25MPH/40KPH Timer:", 1, RUN_TIME_ROW},
	{"600 Seconds Total Drive Timer:", 1, TOTAL_TIME_ROW},
	{" 10 Seconds Fueled Engine Operation (PHEV only):", 1, PHEV_TIME_ROW},

	{"ECU ID:", 1, ECU_ID_ROW},

	{"Initial OBDCOND:", 1, INI_OBD_COND_ROW},
	{"Current OBDCOND:", 1, CUR_OBD_COND_ROW},

	{"Initial IGNCTR:", 1, INI_IGN_CNT_ROW},
	{"Current IGNCTR:", 1, CUR_IGN_CNT_ROW},

	{"Initial FEOCNTR:", 1, INI_FEO_CNT_ROW},
	{"Current FEOCNTR:", 1, CUR_FEO_CNT_ROW},

	{"Speed:", 60, SPEED_ROW},
	{"       MPH    KPH", 60, SPEED_ROW+1},

	{"Test Status:", 60, TEST_STATUS_ROW},

	{"Press ESC to abort the Drive Cycle and fail the test", 1, BOTTOM_ROW},
	{"", 0, BOTTOM_ROW+1},
};

StaticTextElement  _string_elements10RPM[] =
{
	{"NOTE: Some powertrain control systems have engine controls that can start and", 1, 4},
	{"stop the engine without regard to ignition position.", 1, 5},

	{"- Continuous time > or = 30 seconds with engine speed < 1150 RPM", 1, 7},
	{"  and accelerator pedal released.", 2, 8},
	{"- Cumulative time > or = 300 seconds with engine speed > or = 1150 RPM.", 1, 9},
	{"- Cumulative time since engine start > or = 600 seconds.", 1, 10},
	{"- Cumulative fueled engine time since engine start > or = 10 seconds for PHEV.", 1, 11},

	{"OBD Drive Cycle Status", 1, 13},

	{" 30 Seconds Idle Timer:", 1, IDLE_TIME_ROW},
	{"300 Seconds at speeds > 1150  RPM  Timer:", 1, RUN_TIME_ROW},
	{"600 Seconds Total Drive Timer:", 1, TOTAL_TIME_ROW},
	{" 10 Seconds Fueled Engine Operation (PHEV only):", 1, PHEV_TIME_ROW},

	{"ECU ID:", 1, ECU_ID_ROW},

	{"Initial OBDCOND:", 1, INI_OBD_COND_ROW},
	{"Current OBDCOND:", 1, CUR_OBD_COND_ROW},

	{"Initial IGNCTR:", 1, INI_IGN_CNT_ROW},
	{"Current IGNCTR:", 1, CUR_IGN_CNT_ROW},

	{"Initial FEOCNTR:", 1, INI_FEO_CNT_ROW},
	{"Current FEOCNTR:", 1, CUR_FEO_CNT_ROW},

	{"Speed:", 60, SPEED_ROW},
	{"       RPM", 60, SPEED_ROW+1},

	{"Test Status:", 60, TEST_STATUS_ROW},

	{"Press ESC to abort the Drive Cycle and fail the test", 1, BOTTOM_ROW},
	{"", 0, BOTTOM_ROW+1},
};

const int _num_string_elements_1st_9 = sizeof(_string_elements_1st_9)/sizeof(_string_elements_1st_9[0]);
const int _num_string_elements_1st_10 = sizeof(_string_elements_1st_10)/sizeof(_string_elements_1st_10[0]);
const int _num_string_elements10 = sizeof(_string_elements10)/sizeof(_string_elements10[0]);
const int _num_string_elements10RPM = sizeof(_string_elements10RPM)/sizeof(_string_elements10RPM[0]);

DynamicValueElement  _dynamic_elements10[] =
{
	{26, IDLE_TIME_ROW, NUM_WIDTH},                 // (Index 0) 30 seconds idle drive cycle timer
	{45, RUN_TIME_ROW, NUM_WIDTH},                  // (Index 1) 300 seconds at speeds greater then 25 MPH timer
	{33, TOTAL_TIME_ROW, NUM_WIDTH},                // (Index 2) 600 seconds total drive timer
	{51, PHEV_TIME_ROW, NUM_WIDTH},                 // (Index 3) 10 Seconds Fueled Engine Operation (PHEV only)

	{COL1 + 0*SPACE, ECU_ID_ROW, ECU_WIDTH},        // (Index 4) ECU ID 1
	{COL1 + 1*SPACE, ECU_ID_ROW, ECU_WIDTH},        // (Index 5) ECU ID 2
	{COL1 + 2*SPACE, ECU_ID_ROW, ECU_WIDTH},        // (Index 6) ECU ID 3
	{COL1 + 3*SPACE, ECU_ID_ROW, ECU_WIDTH},        // (Index 7) ECU ID 4
	{COL1 + 4*SPACE, ECU_ID_ROW, ECU_WIDTH},        // (Index 8) ECU ID 5
	{COL1 + 5*SPACE, ECU_ID_ROW, ECU_WIDTH},        // (Index 9) ECU ID 6
	{COL1 + 6*SPACE, ECU_ID_ROW, ECU_WIDTH},        // (Index 10) ECU ID 7
	{COL1 + 7*SPACE, ECU_ID_ROW, ECU_WIDTH},        // (Index 11) ECU ID 8

	{COL1 + 0*SPACE, INI_OBD_COND_ROW, NUM_WIDTH},  // (Index 12) initial obdcond, ECU 1
	{COL1 + 1*SPACE, INI_OBD_COND_ROW, NUM_WIDTH},  // (Index 13) initial obdcond, ECU 2
	{COL1 + 2*SPACE, INI_OBD_COND_ROW, NUM_WIDTH},  // (Index 14) initial obdcond, ECU 3
	{COL1 + 3*SPACE, INI_OBD_COND_ROW, NUM_WIDTH},  // (Index 15) initial obdcond, ECU 4
	{COL1 + 4*SPACE, INI_OBD_COND_ROW, NUM_WIDTH},  // (Index 16) initial obdcond, ECU 5
	{COL1 + 5*SPACE, INI_OBD_COND_ROW, NUM_WIDTH},  // (Index 17) initial obdcond, ECU 6
	{COL1 + 6*SPACE, INI_OBD_COND_ROW, NUM_WIDTH},  // (Index 18) initial obdcond, ECU 7
	{COL1 + 7*SPACE, INI_OBD_COND_ROW, NUM_WIDTH},  // (Index 19) initial obdcond, ECU 8

	{COL1 + 0*SPACE, CUR_OBD_COND_ROW, NUM_WIDTH},  // (Index 20) current obdcond, ECU 1
	{COL1 + 1*SPACE, CUR_OBD_COND_ROW, NUM_WIDTH},  // (Index 21) current obdcond, ECU 2
	{COL1 + 2*SPACE, CUR_OBD_COND_ROW, NUM_WIDTH},  // (Index 22) current obdcond, ECU 3
	{COL1 + 3*SPACE, CUR_OBD_COND_ROW, NUM_WIDTH},  // (Index 23) current obdcond, ECU 4
	{COL1 + 4*SPACE, CUR_OBD_COND_ROW, NUM_WIDTH},  // (Index 24) current obdcond, ECU 5
	{COL1 + 5*SPACE, CUR_OBD_COND_ROW, NUM_WIDTH},  // (Index 25) current obdcond, ECU 6
	{COL1 + 6*SPACE, CUR_OBD_COND_ROW, NUM_WIDTH},  // (Index 26) current obdcond, ECU 7
	{COL1 + 7*SPACE, CUR_OBD_COND_ROW, NUM_WIDTH},  // (Index 27) current obdcond, ECU 8

	{COL1 + 0*SPACE, INI_IGN_CNT_ROW, NUM_WIDTH},   // (Index 28) initial ignctr, ECU 1
	{COL1 + 1*SPACE, INI_IGN_CNT_ROW, NUM_WIDTH},   // (Index 29) initial ignctr, ECU 2
	{COL1 + 2*SPACE, INI_IGN_CNT_ROW, NUM_WIDTH},   // (Index 30) initial ignctr, ECU 3
	{COL1 + 3*SPACE, INI_IGN_CNT_ROW, NUM_WIDTH},   // (Index 31) initial ignctr, ECU 4
	{COL1 + 4*SPACE, INI_IGN_CNT_ROW, NUM_WIDTH},   // (Index 32) initial ignctr, ECU 5
	{COL1 + 5*SPACE, INI_IGN_CNT_ROW, NUM_WIDTH},   // (Index 33) initial ignctr, ECU 6
	{COL1 + 6*SPACE, INI_IGN_CNT_ROW, NUM_WIDTH},   // (Index 34) initial ignctr, ECU 7
	{COL1 + 7*SPACE, INI_IGN_CNT_ROW, NUM_WIDTH},   // (Index 35) initial ignctr, ECU 8

	{COL1 + 0*SPACE, CUR_IGN_CNT_ROW, NUM_WIDTH},   // (Index 36) current ignctr, ECU 1
	{COL1 + 1*SPACE, CUR_IGN_CNT_ROW, NUM_WIDTH},   // (Index 37) current ignctr, ECU 2
	{COL1 + 2*SPACE, CUR_IGN_CNT_ROW, NUM_WIDTH},   // (Index 38) current ignctr, ECU 3
	{COL1 + 3*SPACE, CUR_IGN_CNT_ROW, NUM_WIDTH},   // (Index 39) current ignctr, ECU 4
	{COL1 + 4*SPACE, CUR_IGN_CNT_ROW, NUM_WIDTH},   // (Index 40) current ignctr, ECU 5
	{COL1 + 5*SPACE, CUR_IGN_CNT_ROW, NUM_WIDTH},   // (Index 41) current ignctr, ECU 6
	{COL1 + 6*SPACE, CUR_IGN_CNT_ROW, NUM_WIDTH},   // (Index 42) current ignctr, ECU 7
	{COL1 + 7*SPACE, CUR_IGN_CNT_ROW, NUM_WIDTH},   // (Index 43) current ignctr, ECU 8

	{COL1 + 0*SPACE, INI_FEO_CNT_ROW, NUM_WIDTH},   // (Index 44) initial feocntr, ECU 1
	{COL1 + 1*SPACE, INI_FEO_CNT_ROW, NUM_WIDTH},   // (Index 45) initial feocntr, ECU 2
	{COL1 + 2*SPACE, INI_FEO_CNT_ROW, NUM_WIDTH},   // (Index 46) initial feocntr, ECU 3
	{COL1 + 3*SPACE, INI_FEO_CNT_ROW, NUM_WIDTH},   // (Index 47) initial feocntr, ECU 4
	{COL1 + 4*SPACE, INI_FEO_CNT_ROW, NUM_WIDTH},   // (Index 48) initial feocntr, ECU 5
	{COL1 + 5*SPACE, INI_FEO_CNT_ROW, NUM_WIDTH},   // (Index 49) initial feocntr, ECU 6
	{COL1 + 6*SPACE, INI_FEO_CNT_ROW, NUM_WIDTH},   // (Index 50) initial feocntr, ECU 7
	{COL1 + 7*SPACE, INI_FEO_CNT_ROW, NUM_WIDTH},   // (Index 51) initial feocntr, ECU 8

	{COL1 + 0*SPACE, CUR_FEO_CNT_ROW, NUM_WIDTH},   // (Index 52) current feocntr, ECU 1
	{COL1 + 1*SPACE, CUR_FEO_CNT_ROW, NUM_WIDTH},   // (Index 53) current feocntr, ECU 2
	{COL1 + 2*SPACE, CUR_FEO_CNT_ROW, NUM_WIDTH},   // (Index 54) current feocntr, ECU 3
	{COL1 + 3*SPACE, CUR_FEO_CNT_ROW, NUM_WIDTH},   // (Index 55) current feocntr, ECU 4
	{COL1 + 4*SPACE, CUR_FEO_CNT_ROW, NUM_WIDTH},   // (Index 56) current feocntr, ECU 5
	{COL1 + 5*SPACE, CUR_FEO_CNT_ROW, NUM_WIDTH},   // (Index 57) current feocntr, ECU 6
	{COL1 + 6*SPACE, CUR_FEO_CNT_ROW, NUM_WIDTH},   // (Index 58) current feocntr, ECU 7
	{COL1 + 7*SPACE, CUR_FEO_CNT_ROW, NUM_WIDTH},   // (Index 59) current feocntr, ECU 8

	{72, IDLE_TIME_ROW, 3},                         // (Index 60) RPM label
	{67, IDLE_TIME_ROW, NUM_WIDTH},                 // (Index 61) RPM value
	{67, SPEED_ROW, NUM_WIDTH},                     // (Index 62) Speed MPH
	{67 + SPACE, SPEED_ROW, NUM_WIDTH},             // (Index 63) Speed KPH

	{73, TEST_STATUS_ROW, STRING_WIDTH},            // (Index 64) Test Status
};

const int _num_dynamic_elements10 = sizeof(_dynamic_elements10)/sizeof(_dynamic_elements10[0]);

#define IDLE_TIMER          0
#define SPEED_25_MPH_TIMER  1
#define TOTAL_DRIVE_TIMER   2
#define PHEV_TIMER          3
#define ECU_ID              4

#define INITIAL_OBDCOND     12
#define CURRENT_OBDCOND     20
#define INITIAL_IGNCTR      28
#define CURRENT_IGNCTR      36
#define INITIAL_FEOCNTR     44
#define CURRENT_FEOCNTR     52

#define RPM_LABEL_INDEX     60
#define RPM_INDEX           61
#define SPEED_INDEX_MPH     62
#define SPEED_INDEX_KPH     63
#define TESTSTATUS_INDEX    64

// used with the on-screen test status
#define SUB_TEST_NORMAL     0x00
#define SUB_TEST_FAIL       0x01
#define SUB_TEST_DTC        0x02
#define SUB_TEST_INITIAL    0x80


#define NORMAL_TEXT         -1
#define HIGHLIGHTED_TEXT    8

#define SetFieldDec(index,val)  (update_screen_dec(_dynamic_elements10,_num_dynamic_elements10,index,val))
#define SetFieldHex(index,val)  (update_screen_hex(_dynamic_elements10,_num_dynamic_elements10,index,val))
#define SetFieldText(index,val) (update_screen_text(_dynamic_elements10,_num_dynamic_elements10,index,val))


SID9IPT Test10_10_Sid9FEOCNTR[OBD_MAX_ECUS];
SID9IPT Last_Sid9IPT[OBD_MAX_ECUS];




/*
*******************************************************************************
** TestToVerifyInUseCounters - Tests 10.x
** Function to run test to verify in-use counters with no faults
*******************************************************************************
*/
STATUS TestToVerifyInUseCounters (BOOL *pbTestReEntered)
{
	STATUS        eResults;
	unsigned long tEngineStartTimeStamp;
	BOOL          bIPTSupported = FALSE;

	BOOL          bSubTestFailed = FALSE;


	/* Initialize Array */
	memset (Test10_10_Sid9Ipt, 0x00, sizeof(Test10_10_Sid9Ipt));


//*******************************************************************************
	/* Prompt user to perform drive cycle to clear I/M readiness bits */
	TestSubsection = 1;
	gOBDTestSubsectionFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Establish Communication, Engine Off)");

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Turn key on without cranking or starting engine.\n");

	/* Engine should now be not running */
	gOBDEngineRunning = FALSE;

	/* Determine the OBD protocol to use */
	gOBDProtocolOrder = 0;

	if ( DetermineProtocol() != PASS )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Protocol determination unsuccessful.\n" );
		Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
		return(FAIL);
	}
	else if ( (VerifyVehicleState(gOBDEngineRunning, gOBDHybridFlag) != PASS) ||
	          (gOBDTestSubsectionFailed == TRUE) )
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
	/* Get SID $01 data */
	TestSubsection = 2;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Diagnostic Data (Service $01), Engine Off)");
	if ( VerifyDiagnosticSupportAndData( TRUE ) == FAIL )
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
	/* Get VIN and create (or append) log file */
	TestSubsection = 3;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Vehicle Information (Service $09), Engine Off)");
	eResults = StartLogFile(pbTestReEntered);
	if ( eResults == FAIL )
	{
		Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
		return(FAIL);
	}
	else if ( eResults == EXIT )
	{
		/* Dynamic Tests are all done, just exit */
		return(EXIT);
	}

	/* Add CALIDs to the log file */
	if (PrintCALIDs() != PASS)
	{
		bSubTestFailed = TRUE;
	}

	/* Add CVNs to the log file */
	if (PrintCVNs() != PASS)
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

	if (*pbTestReEntered == TRUE)
	{
		return(PASS);
	}


//*******************************************************************************
	/* Identify ECUs that support SID 1 PID 1 */
	TestSubsection = 4;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Request current powertrain diagnostic data (Service $01), Engine Off)");
	if ( RequestSID1SupportData() != PASS )
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


	if (*pbTestReEntered == TRUE)
	{
		return(PASS);
	}


//*******************************************************************************
	/* Clear DTCs (service 4) */
	TestSubsection = 5;
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

	gOBDDTCHistorical = FALSE;

	/* Verify I/M readiness is "not ready" (service 1) */
	TestSubsection = 6;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify IM Readiness (Service $01), Engine Off)");
	if ( VerifyIM_Ready() != PASS )
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
	/* Verify Monitor Resets (service 6) */
	TestSubsection = 7;
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
	/* Verify no pending DTCs (service 7) */
	TestSubsection = 8;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Pending DTCs (Service $07), Engine Off)");
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
	/* Verify no confirmed DTCs (service 3) */
	TestSubsection = 9;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify DTCs (Service $03), Engine Off)");
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
	/* Get in-use performance tracking (service 9 infotype 8 and B) */
	TestSubsection = 10;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Vehicle Information (Service $09), Engine Off)");

	if ( VerifyIPTData(&bIPTSupported) != PASS || gOBDTestSubsectionFailed == TRUE )
	{
		if ( Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT,
		          "Verify IPT Stored Data.\n" ) == 'N' )
		{
			Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
			return(FAIL);
		}
		bSubTestFailed = TRUE;
		gOBDTestSubsectionFailed = FALSE;
	}
	else if ( bIPTSupported == FALSE &&
	          ( ( (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) && gModelYear < 2005 ) ||
	            (gUserInput.eComplianceType == EOBD_NO_IUMPR) ||
	            (gUserInput.eComplianceType == IOBD_NO_IUMPR) ||
	            (gUserInput.eComplianceType == HD_IOBD_NO_IUMPR) ||
	            (gUserInput.eComplianceType == OBDBr_NO_IUMPR) ) )
	{
		/* allowed to skip the CARB Drive Cycle */
		Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
		return PASS;
	}

	if ( gOBDPlugInFlag )
	{
		if ( LogSid9Inf12( Test10_10_Sid9FEOCNTR ) != PASS )
		{
			bSubTestFailed = TRUE;
		}
	}

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Turn ignition off (engine off) for 60 seconds.\n");

	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Turn ignition to crank position and start engine.\n");

	tEngineStartTimeStamp = GetTickCount ();

	if ( (gOBDTestSubsectionFailed == TRUE) || (bSubTestFailed == TRUE) )
	{
		Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
	}
	else
	{
		Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
	}


//*******************************************************************************
	/* Engine should now be running */
	gOBDEngineRunning = TRUE;

	TestSubsection = 11;
	gOBDTestSubsectionFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Establish Communication, Engine Running)");

	if ( DetermineProtocol() != PASS )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Protocol determination unsuccessful.\n" );
		Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
		return(FAIL);
	}
	else if ( (VerifyVehicleState(gOBDEngineRunning, gOBDHybridFlag) != PASS) ||
	          (gOBDTestSubsectionFailed == TRUE) )
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
	/* User prompt, run test */
	TestSubsection = 12;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Complete CARB drive cycle)");


	StopPeriodicMsg (TRUE);
	Sleep (gOBDRequestDelay);

	gSuspendScreenOutput = TRUE;
	eResults = RunDynamicTest10 (tEngineStartTimeStamp);
	DisplayDynamicTest10Results();
	gSuspendScreenOutput = FALSE;

	// re-start tester-present message
	StartPeriodicMsg ();

	if ( VerifyVehicleState(gOBDEngineRunning, gOBDHybridFlag) != PASS )
	{
		bSubTestFailed = TRUE;
	}

	if ( (eResults == FAIL) || (gOBDTestSubsectionFailed == TRUE) || (bSubTestFailed == TRUE) )
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
	     "Stop the vehicle in a safe location without turning the ignition off.\n");


//*******************************************************************************

	gOBDEngineWarm = TRUE;

	/* Verify engine warm data (service 1) */
	TestSubsection = 13;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Diagnostic Data (Service $01), Engine Running)");
	if ( VerifyDiagnosticSupportAndData( FALSE ) == FAIL )
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
			return FAIL;
		}
	}
	else
	{
		Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
	}


//*******************************************************************************
	/* Get in-use performance tracking (service 9 infotype 8) */
	TestSubsection = 14;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Vehicle Information (Service $09), Engine Running)");
	if ( LogSid9Ipt() != PASS || gOBDTestSubsectionFailed == TRUE )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 IPT data error\n");
		bSubTestFailed = TRUE;
	}

	if ( gOBDPlugInFlag )
	{
		if ( VerifySid9Inf12( &Test10_10_Sid9FEOCNTR[0] ) != PASS )
		{
			bSubTestFailed = TRUE;
		}
	}

	if ( VerifyVehicleState(gOBDEngineRunning, gOBDHybridFlag) != PASS )
	{
		bSubTestFailed = TRUE;
	}

	if ( (gOBDTestSubsectionFailed == TRUE) || (bSubTestFailed == TRUE) )
	{
		if ( Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT, "") == 'N' )
		{
			return FAIL;
		}
	}
	else
	{
		Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
	}

	return ( PASS );
}


/*
*******************************************************************************
** StartLogFile - create new log file or open and append to existing one.
**                use VIN as log filename.
*******************************************************************************
*/
STATUS StartLogFile (BOOL *pbTestReEntered)
{
	char buf[256];
	char characterset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890 -_.";
	STATUS eResult = PASS;
	FILE *hTempFileHandle; /* temporary file handle */

	/* Get VIN */
	if (ReadVIN () == PASS)
	{
		/* use VIN as log filename */
		strcpy (gLogFileName, gVIN);
	}
	else
	{
		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Unable to obtain VIN\n" );
		do
		{
			Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, CUSTOM_PROMPT,
			     "Enter a valid filename (<%d characters) for the log file: ", MAX_LOGFILENAME );
			gets ( buf );
			Log( BLANK, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "\n");
		} while ( strlen (buf) == 0 ||                           // empty (NULL) string
		          strspn ( buf, characterset ) < strlen (buf) || // contains nonalphanumeric characters
		          (strlen (buf) + 1) > MAX_LOGFILENAME );        // longer than max filename

		strcpy (gLogFileName, buf);
	}

	/* Check for log file from Tests 5.xx - 9.xx  */
	strcat (gLogFileName, ".log");
	if ( (ghTempLogFile != ghLogFile) && (ghLogFile != NULL) )
	{
		fclose (ghLogFile);
	}

	/* Test if log file already exists */
	hTempFileHandle = fopen (gLogFileName, "r+");
	if (hTempFileHandle == NULL)
	{
		/* file does not exist - create file and start with test 10 */
		ghLogFile = fopen (gLogFileName, "w+");
		if (ghLogFile == NULL)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTOFF, NO_PROMPT,
			     "Cannot open log file %s\n", gLogFileName);
			return(FAIL);
		}

		/* log file doesn't exist. continue with Test 10.x */
		*pbTestReEntered = FALSE;
	}
	else
	{
		/* check software version, user input, test already complete, and get results totals in log file */
		eResult = VerifyLogFile(hTempFileHandle);
		if ( (eResult == FAIL) || (eResult == EXIT) )
		{
			return(eResult);
		}

		ghLogFile = hTempFileHandle;

		/* log file already exists, go to Test 11.x */
		fputs ("\n****************************************************\n", ghLogFile);
		*pbTestReEntered = TRUE;
	}

	/* Application version, build date, OS, etc */
	LogVersionInformation (SCREENOUTPUTOFF, LOGOUTPUTON);

	/* Copy temp log file to actual log file */
	if (AppendLogFile() != PASS)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTOFF, NO_PROMPT,
		     "Error copying temp log file to %s\n", gLogFileName);
		return(FAIL);
	}

	/* Echo user responses to the log file */
	Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
	     "%s%d\n", g_rgpcDisplayStrings[DSPSTR_PRMPT_MODEL_YEAR], gModelYear);
	Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
	     "%s%d\n", g_rgpcDisplayStrings[DSPSTR_PRMPT_OBD_ECU], gUserNumEcus);
	Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
	     "%s%d\n", g_rgpcDisplayStrings[DSPSTR_PRMPT_RPGM_ECU], gUserNumEcusReprgm);
	Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
	     "%s%s\n", g_rgpcDisplayStrings[DSPSTR_PRMPT_ENG_TYPE], gEngineTypeStrings[gUserInput.eEngineType] );
	Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
	     "%s%s\n", g_rgpcDisplayStrings[DSPSTR_PRMPT_PWRTRN_TYPE], gPwrTrnTypeStrings[gUserInput.ePwrTrnType] );
	Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
	     "%s%s\n", g_rgpcDisplayStrings[DSPSTR_PRMPT_VEH_TYPE], gVehicleTypeStrings[gUserInput.eVehicleType] );
	Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
	     "%s%s\n", g_rgpcDisplayStrings[DSPSTR_STMT_COMPLIANCE_TYPE],
	     gComplianceTestTypeStrings[gUserInput.eComplianceType]);

	/* done */
	return(PASS);
}


/******************************************************************************
// Function:   ReadVIN
//
// Purpose: Purpose of this function is to read the VIN from the ECUs.
//          If an ECU returns a correctly formatted VIN, return TRUE,
//          otherwise return FAIL.
******************************************************************************/
STATUS ReadVIN (void)
{
	unsigned long  EcuIndex;
	unsigned long  NumResponses;
	SID_REQ        SidReq;
	SID9          *pSid9;
	unsigned long  SidIndex;

	STATUS         RetCode = PASS;

	/* Request SID 9 support data */
	if (RequestSID9SupportData() != PASS)
	{
		return FAIL;
	}

	/* INF Type Vin Count for non-ISO15765 only */
	if (gOBDList[gOBDListIndex].Protocol != ISO15765)
	{
		/* If INF is supported by any ECU, request it */
		if (IsSid9InfSupported (-1, INF_TYPE_VIN_COUNT) == TRUE)
		{
			SidReq.SID    = 9;
			SidReq.NumIds = 1;
			SidReq.Ids[0] = INF_TYPE_VIN_COUNT;

			if (SidRequest (&SidReq, SID_REQ_NORMAL) == FAIL)
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "SID $9 INF $1 (VIN COUNT) request\n");
				RetCode = FAIL;
			}
			else
			{
				NumResponses = 0;

				/* check responses */
				for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
				{
					/* Check the data to see if it is valid */
					pSid9 = (SID9 *)&gOBDResponse[EcuIndex].Sid9Inf[0];

					if (gOBDResponse[EcuIndex].Sid9InfSize != 0)
					{
						NumResponses++;
						Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  Responded to SID $9 INF $1 (VIN COUNT) request.\n", GetEcuId(EcuIndex) );

						if (pSid9->INF == INF_TYPE_VIN_COUNT)
						{
							if (pSid9->NumItems != 0x05)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  SID $9 INF $1 (VIN Count) NumItems = %d (should be 5)\n",
								     GetEcuId(EcuIndex),
								     pSid9->NumItems );
								RetCode = FAIL;
							}
						}

					}
				}

				if ( (NumResponses > 1) &&
				     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "%u ECUs responded to SID $9 INF $1 (VIN COUNT).  Only 1 is allowed.\n", NumResponses);
					RetCode = FAIL;
				}
			}
		}
	}

	/* If INF is supported by any ECU, request it */
	if (IsSid9InfSupported (-1, INF_TYPE_VIN) == TRUE)
	{
		SidReq.SID    = 9;
		SidReq.NumIds = 1;
		SidReq.Ids[0] = INF_TYPE_VIN;

		if (SidRequest (&SidReq, SID_REQ_NORMAL) == FAIL)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $9 INF $2 (VIN) request\n" );
			RetCode = FAIL;
		}
		else
		{
			NumResponses = 0;

			/* check responses */
			for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
			{
				/* Check the data to see if it is valid */
				pSid9 = (SID9 *)&gOBDResponse[EcuIndex].Sid9Inf[0];

				if (pSid9->INF == INF_TYPE_VIN)
				{
					/* Copy the VIN into the global array */
					if (gOBDList[gOBDListIndex].Protocol == ISO15765)
					{
						memcpy (gVIN, &pSid9[0].Data[0], 17);

						/* check that there are no pad bytes included in message. */
						if (pSid9[0].Data[20] != 0x00)
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $9 INF $2 (VIN) format error, must be 17 chars!\n", GetEcuId(EcuIndex) );
							RetCode = FAIL;
						}
					}
					else
					{
						/* INF $2 (VIN) should not be supported if INF $1 (VIN msg count) not supported for non-15765 protocols */
						if ( IsSid9InfSupported ( EcuIndex, INF_TYPE_VIN_COUNT ) == FALSE )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $9 INF $2 (VIN) is supported but not INF $1 (VIN msg count)\n",
							     GetEcuId(EcuIndex));
							RetCode = FAIL;
						}

						for (
						      SidIndex = 0;
						      SidIndex < (gOBDResponse[EcuIndex].Sid9InfSize / sizeof (SID9));
						      SidIndex++
						    )
						{
							if (SidIndex == 0)
							{
								gVIN[0] = pSid9[SidIndex].Data[3];
							}
							else if (SidIndex < 5)
							{
								memcpy (&gVIN[SidIndex*4 - 3], &pSid9[SidIndex].Data[0], 4);
							}
						}
					}

					NumResponses++;
					Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X Responded to SID $9 INF $2 (VIN) request.\n", GetEcuId(EcuIndex) );

					if (VerifyVINFormat (EcuIndex) != PASS)
					{
						RetCode = FAIL;
					}
				}
			}

			if ( (NumResponses != 1) && /* only 1 ECU allowed to support VIN */
			     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "%u ECUs responded to SID $9 INF $2 (VIN).  Only 1 is allowed.\n", NumResponses );
				RetCode = FAIL;
			}
		}
	}
	else
	{
		/* No VIN support */
		if ( gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $9 INF $2 must be reported by one ECU\n");
			RetCode = FAIL;
		}
		else
		{
			Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $9 INF $2 not reported by any ECUs\n");
		}
	 }

	return RetCode;
}


//*****************************************************************************
//
// Function:   PrintCALIDs
//
// Purpose: print CALIDs to the log file
//
//*****************************************************************************
STATUS PrintCALIDs (void)
{
	unsigned long  EcuIndex;
	SID_REQ        SidReq;
	SID9          *pSid9;

	unsigned long  Inf3NumItems[OBD_MAX_ECUS] = {0};


	if (gOBDList[gOBDListIndex].Protocol != ISO15765)
	{
		if (IsSid9InfSupported (-1, 3) == TRUE)
		{
			SidReq.SID    = 9;
			SidReq.NumIds = 1;
			SidReq.Ids[0] = 3;

			if (SidRequest(&SidReq, SID_REQ_NORMAL) == FAIL)
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "SID $9 INF $3 (CALID Count) request\n" );
				return FAIL;
			}

			for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
			{
				/* If INF is not supported, skip to next ECU */
				if (IsSid9InfSupported (EcuIndex, 3) == FALSE)
					continue;

				/* Check the data to see if it is valid */
				pSid9 = (SID9 *)&gOBDResponse[EcuIndex].Sid9Inf[0];

				if (gOBDResponse[EcuIndex].Sid9InfSize == 0)
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  No SID $9 INF $3 (CALID Count) data\n", GetEcuId(EcuIndex) );
					return (FAIL);
				}

				if (pSid9[0].NumItems & 0x03)
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  SID $9 INF $3 (CALID Count) NumItems = %d (should be a multiple of 4)\n",
					     GetEcuId(EcuIndex), pSid9[0].NumItems );
					return (FAIL);
				}

				Inf3NumItems[EcuIndex] = pSid9[0].NumItems;
			}
		}
	}

	SidReq.SID    = 9;
	SidReq.NumIds = 1;
	SidReq.Ids[0] = 4;

	if (SidRequest(&SidReq, SID_REQ_NORMAL) == FAIL)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 INF $4 (CALID) request\n");
		return FAIL;
	}

	for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
	{
		VerifyCALIDFormat ( EcuIndex, Inf3NumItems[EcuIndex] );
	}

	return PASS;
}


//*****************************************************************************
//
// Function:   PrintCVNs
//
// Purpose: print CVNs to the log file
//
//*****************************************************************************
STATUS PrintCVNs (void)
{
	unsigned long  EcuIndex;
	SID_REQ        SidReq;
	SID9          *pSid9;

	unsigned long  Inf5NumItems[OBD_MAX_ECUS] = {0};


	if ( gOBDList[gOBDListIndex].Protocol != ISO15765 )
	{
		if ( IsSid9InfSupported ( -1, 5 ) == TRUE )
		{
			SidReq.SID    = 9;
			SidReq.NumIds = 1;
			SidReq.Ids[0] = 5;

			if ( SidRequest( &SidReq, SID_REQ_NORMAL ) == FAIL )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "SID $9 INF $5 (CVN Count) request\n" );
				return FAIL;
			}

			for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
			{
				/* If INF is not supported, skip to next ECU */
				if ( IsSid9InfSupported ( EcuIndex, 5 ) == FALSE )
					continue;

				/* Save the data */
				pSid9 = (SID9 *)&gOBDResponse[EcuIndex].Sid9Inf[0];
				Inf5NumItems[EcuIndex] = pSid9[0].NumItems;
			}
		}
	}

	SidReq.SID    = 9;
	SidReq.NumIds = 1;
	SidReq.Ids[0] = 6;

	if ( SidRequest( &SidReq, SID_REQ_NORMAL ) == FAIL )
	{
		if ( gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $9 INF $6 (CVN) request\n" );
			return FAIL;
		}
		else
		{
			return PASS;
		}
	}

	for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
	{
		/* If INF is not supported, skip to next ECU */
		if ( IsSid9InfSupported ( EcuIndex, 6 ) == FALSE &&
		     (gUserInput.eComplianceType != US_OBDII && gUserInput.eComplianceType != HD_OBD) )
			continue;

		VerifyCVNFormat ( EcuIndex, Inf5NumItems[EcuIndex] );
	}

	return PASS;
}


/*
*******************************************************************************
** VerifyIPTData
*******************************************************************************
*/
STATUS VerifyIPTData (BOOL *pbIPTSupported)
{
	SID_REQ        SidReq;
	unsigned int   EcuIndex;
	unsigned char  fInf8Supported = FALSE;
	unsigned char  fInfBSupported = FALSE;


	SidReq.SID    = 9;
	SidReq.NumIds = 1;

	/* Sid 9 Inf 8 request*/
	if ( IsSid9InfSupported (-1, 0x08) == TRUE )
	{
		fInf8Supported = TRUE;
		*pbIPTSupported = TRUE;

		SidReq.Ids[0] = 8;
	}

	/* Sid 9 Inf B request*/
	if ( IsSid9InfSupported (-1, 0x0B) == TRUE )
	{
		fInfBSupported = TRUE;
		*pbIPTSupported = TRUE;

		SidReq.Ids[0] = 0x0B;
	}


	if ( fInf8Supported == FALSE && fInfBSupported == FALSE )
	{
		if ( ( (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) &&
		       gModelYear >= 2005 ) ||
		     (gUserInput.eComplianceType == EOBD_WITH_IUMPR ||
		      gUserInput.eComplianceType == HD_EOBD) )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $9 IPT (INF $8 or INF $B) not supported\n" );
			return FAIL;
		}
		else
		{
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $9 IPT (INF $8 or INF $B) not supported\n" );
			return PASS;
		}
	 }
	 else if ( fInf8Supported == TRUE && fInfBSupported == TRUE )
	{
		for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
		{
			if ( IsSid9InfSupported (EcuIndex, 0x08) == TRUE )
			{
				Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  SID $9 INF $8 supported\n", GetEcuId(EcuIndex) );
			}
			if ( IsSid9InfSupported (EcuIndex, 0x0B) == TRUE )
			{
				Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  SID $9 INF $B supported\n", GetEcuId(EcuIndex) );
			}
		}
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 IPT (INF $8 and INF $B) both supported\n" );
		return FAIL;
	}


	if ( SidRequest( &SidReq, SID_REQ_NORMAL ) == FAIL)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 INF $%X request\n", SidReq.Ids[0] );
		return (FAIL);
	}

	for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
	{
		if ( IsSid9InfSupported(EcuIndex, 0x08) == TRUE )
		{
			if ( VerifyINF8Data(EcuIndex) != FAIL )
			{
				if ( GetSid9IptData(EcuIndex, &Test10_10_Sid9Ipt[EcuIndex]) != PASS )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  Get SID $9 IPT Data\n", GetEcuId(EcuIndex) );
					return (FAIL);
				}
			}
			else
			{
				return (FAIL);
			}
		}
		else if ( IsSid9InfSupported(EcuIndex, 0x0B) == TRUE )
		{
			if ( VerifyINFBData(EcuIndex) == PASS )
			{
				if ( GetSid9IptData(EcuIndex, &Test10_10_Sid9Ipt[EcuIndex]) != PASS )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  Get SID $9 IPT Data\n", GetEcuId(EcuIndex) );
					return (FAIL);
				}
			}
			else
			{
				return (FAIL);
			}
		}
	}

	if ( LogSid9Ipt() != PASS )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Log SID $9 IPT Data\n" );
		return (FAIL);
	}

	return (PASS);
}


//*****************************************************************************
//
// Function:   GetSid9IptData
//
// Purpose: Copy from gOBDResponse[EcuIndex].Sid9Inf into common,
//          protocol-independent format.
//
//*****************************************************************************
STATUS GetSid9IptData ( unsigned int EcuIndex, SID9IPT *pSid9Ipt )
{
	unsigned int    SidIndex;
	unsigned int	IptIndex;
	unsigned short *pData;
	SID9           *pSid9;
	unsigned int    IPT_MAX;
	STATUS Result = FAIL;


	// clear IPT structure
	memset (pSid9Ipt, 0, sizeof (SID9IPT));

	// point to IPT structure in gOBDResponse
	pSid9 = (SID9 *)&gOBDResponse[EcuIndex].Sid9Inf[0];

	// check to ensure that gOBDResponse contains IPT data
	if ( gOBDResponse[EcuIndex].Sid9InfSize == 0 ||
	     ( pSid9->INF != 0x08 && pSid9->INF != 0x0B ) )
	{
		return FAIL;
	}

	// if ISO15765 protocol
	if ( gOBDList[gOBDListIndex].Protocol == ISO15765 )
	{
		memcpy (pSid9Ipt, &(gOBDResponse[EcuIndex].Sid9Inf[0]), sizeof (SID9IPT));
		memcpy (&Last_Sid9IPT[EcuIndex], &(gOBDResponse[EcuIndex].Sid9Inf[0]), sizeof (SID9IPT));

		// If NOT test 9.19, 10.12 or 11.2, log failures
		if ( ( TestPhase != eTestNoFault3DriveCycle	|| TestSubsection != 19 ) &&
		     ( TestPhase != eTestInUseCounters || TestSubsection != 12 ) &&
		     ( TestPhase != eTestPerformanceCounters || TestSubsection != 2 ) )
		{
			if ( pSid9->INF == 0x08 )
			{
				Result = VerifyINF8Data(EcuIndex);
			}
			else if ( pSid9->INF == 0x0B )
			{
				Result = VerifyINFBData(EcuIndex);
			}

			if (Result == FAIL)
			{
				return (Result);
			}
		} //end if !test 9.19 and !test 10.12 and !test11.2

		// point to start of IPT data in structure
		pData = &(pSid9Ipt->IPT[0]);

		// calculate number of IPT data bytes
		IPT_MAX = (pSid9Ipt->NODI <= INF_TYPE_IPT_NODI) ? (pSid9Ipt->NODI)*2 : INF_TYPE_IPT_NODI*2;

		// swap low and high bytes
		for ( SidIndex = 2, IptIndex = 0; SidIndex <= IPT_MAX; SidIndex+=2 )
		{
			*pData++ = gOBDResponse[EcuIndex].Sid9Inf[SidIndex] * 256
			         + gOBDResponse[EcuIndex].Sid9Inf[SidIndex+1];
			Last_Sid9IPT[EcuIndex].IPT[IptIndex++] = gOBDResponse[EcuIndex].Sid9Inf[SidIndex] * 256
			                                       + gOBDResponse[EcuIndex].Sid9Inf[SidIndex+1];
		}
	} // end if ( gOBDList[gOBDListIndex].Protocol == ISO15765 )

	// else Legacy protocols
	else
	{
		pSid9Ipt->INF  = gOBDResponse[EcuIndex].Sid9Inf[0];
		pSid9Ipt->NODI = (gOBDResponse[EcuIndex].Sid9InfSize / sizeof(SID9)) * 2; // MessageCount * 2

		Last_Sid9IPT[EcuIndex].INF  = gOBDResponse[EcuIndex].Sid9Inf[0];
		Last_Sid9IPT[EcuIndex].NODI = (gOBDResponse[EcuIndex].Sid9InfSize / sizeof(SID9)) * 2; // MessageCount * 2

		// If NOT test 9.19, 10.12 or 11.2, log failures
		if ( ( TestPhase != eTestNoFault3DriveCycle	|| TestSubsection != 19 ) &&
		     ( TestPhase != eTestInUseCounters || TestSubsection != 12 ) &&
		     ( TestPhase != eTestPerformanceCounters || TestSubsection != 2 ) )
		{
			if ( pSid9->INF == 0x08 )
			{
				Result = VerifyINF8Data(EcuIndex);
			}
			else if ( pSid9->INF == 0x0B )
			{
				Result = VerifyINFBData(EcuIndex);
			}

			if (Result != PASS)
			{
				return (Result);
			}
		}

		// point to start of IPT data
		pData = &(pSid9Ipt->IPT[0]);

		// save IPT data to structure
		for ( SidIndex = 0, IptIndex = 0; SidIndex < (gOBDResponse[EcuIndex].Sid9InfSize / sizeof(SID9)); SidIndex++ )
		{
			*pData++ = pSid9[SidIndex].Data[0] * 256 + pSid9[SidIndex].Data[1];
			*pData++ = pSid9[SidIndex].Data[2] * 256 + pSid9[SidIndex].Data[3];

			Last_Sid9IPT[EcuIndex].IPT[IptIndex++] = pSid9[SidIndex].Data[0] * 256 + pSid9[SidIndex].Data[1];
			Last_Sid9IPT[EcuIndex].IPT[IptIndex++] = pSid9[SidIndex].Data[2] * 256 + pSid9[SidIndex].Data[3];
		}
	}

	// data structure is valid
	pSid9Ipt->Flags = 1;

	return PASS;
}


//******************************************************************************
// LogSid9Ipt
//*****************************************************************************
const char szECUID[]     = "ECU ID:";
const char szINF_SIZE[]  = "INF SIZE:";

const char szINF8[][10]  = { "OBDCOND",
                             "IGNCNTR",
                             "CATCOMP1",
                             "CATCOND1",
                             "CATCOMP2",
                             "CATCOND2",
                             "O2SCOMP1",
                             "O2SCOND1",
                             "O2SCOMP2",
                             "O2SCOND2",
                             "EGRCOMP",
                             "EGRCOND",
                             "AIRCOMP",
                             "AIRCOND",
                             "EVAPCOMP",
                             "EVAPCOND",
                             "SO2SCOMP1",
                             "SO2SCOND1",
                             "SO2SCOMP2",
                             "SO2SCOND2",
                             "AFRICOMP1",
                             "AFRICOND1",
                             "AFRICOMP2",
                             "AFRICOND2",
                             "PFCOMP1",
                             "PFCOND1",
                             "PFCOMP2",
                             "PFCOND2" };

const char szINFB[][10]  = { "OBDCOND",
                             "IGNCNTR",
                             "HCCATCOMP",
                             "HCCATCOND",
                             "NCATCOMP",
                             "NCATCOND",
                             "NADSCOMP",
                             "NADSCOND",
                             "PMCOMP",
                             "PMCOND",
                             "EGSCOMP",
                             "EGSCOND",
                             "EGRCOMP",
                             "EGRCOND",
                             "BPCOMP",
                             "BPCOND",
                             "FUELCOMP",
                             "FUELCOND" };

STATUS LogSid9Ipt (void)
{
	SID_REQ       SidReq;
	unsigned int  EcuIndex;
	unsigned int  IptIndex;
	SID9IPT       Sid9Ipt;
	unsigned char fInf8Supported = FALSE;
	unsigned char fInfBSupported = FALSE;


	SidReq.SID    = 9;
	SidReq.NumIds = 1;

	/* Sid 9 Inf 8 request*/
	if ( IsSid9InfSupported (-1, 0x08) == TRUE )
	{
		fInf8Supported = TRUE;

		/* Sid 9 Inf 8 request*/
		SidReq.Ids[0] = 0x08;
	}

	/* Sid 9 Inf B request*/
	if ( IsSid9InfSupported (-1, 0x0B) == TRUE )
	{
		fInfBSupported = TRUE;

		/* Sid 9 Inf 8 request*/
		SidReq.Ids[0] = 0x0B;
	}

	if ( fInf8Supported == FALSE && fInfBSupported == FALSE )
	{
		if ( ( (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) &&
		       gModelYear >= 2005 ) ||
		     (gUserInput.eComplianceType == EOBD_WITH_IUMPR || gUserInput.eComplianceType == HD_EOBD) )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $9 IPT (INF $8 or INF $B) not supported\n" );
			return FAIL;
		}
		else
		{
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $9 IPT (INF $8 or INF $B) not supported\n" );
			return PASS;
		}
	}
	else if ( fInf8Supported == TRUE && fInfBSupported == TRUE )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 IPT (INF $8 and INF $B) both supported\n" );
		return FAIL;
	}

	if ( SidRequest( &SidReq, SID_REQ_NORMAL ) == FAIL)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 INF $%X request\n", SidReq.Ids[0]);
		return FAIL;
	}

	for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
	{
		if ( GetSid9IptData ( EcuIndex, &Sid9Ipt ) == PASS )
		{
			// log OBDCOND, IGNCTR, all OBD monitor and completion counters
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "%-9s 0x%X\n", szECUID, GetEcuId (EcuIndex) );
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "%-9s 0x%X %d\n", szINF_SIZE, Sid9Ipt.INF, Sid9Ipt.NODI );

			for ( IptIndex = 0; IptIndex < Sid9Ipt.NODI && IptIndex < INF_TYPE_IPT_NODI; IptIndex++ )
			{
				Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "%-9s = %u\n",
				     (Sid9Ipt.INF == 0x08) ? szINF8[IptIndex] : szINFB[IptIndex],
				     Sid9Ipt.IPT[IptIndex] );
			}
		}
	}

	return PASS;
}


//*****************************************************************************
// ReadSid9Ipt
//*****************************************************************************
BOOL ReadSid9Ipt (const char * szTestSectionEnd, SID9IPT Sid9Ipt[])
{
	char buf[256];
	char * p;
	int  count;
	unsigned int EcuIndex;
	unsigned int EcuId = 0;

	// search for ECU ID
	while (fgets (buf, sizeof(buf), ghLogFile) != 0)
	{
		if (substring(buf, szTestSectionEnd) != 0)     // end of Test XX section
			return FALSE;

		if ( (p = substring(buf, szECUID)) != 0)
		{
			p += sizeof (szECUID);
			EcuId = strtoul (p, NULL, 16);
			break;
		}
	}

	// find EcuIndex
	for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
	{
		if (EcuId == GetEcuId (EcuIndex))
		{
			break;
		}
	}

	if (EcuIndex >= gUserNumEcus)
	{
		return FALSE;
	}

	// search for IPT INFOTYPE and SIZE
	while (fgets (buf, sizeof(buf), ghLogFile) != 0)
	{
		if ( substring(buf, szTestSectionEnd) != 0 )     // end of Test XX section
			return FALSE;

		if ( (p = substring(buf, szINF_SIZE)) != 0)
		{
			p += sizeof (szINF_SIZE);
			Sid9Ipt[EcuIndex].INF = (unsigned char) strtoul (p, NULL, 16);
			p += 3;
			Sid9Ipt[EcuIndex].NODI = (unsigned char) strtod (p, NULL);
			break;
		}
	}

	// read counters
	for ( count = 0; count < Sid9Ipt[EcuIndex].NODI; count++ )
	{
		fgets (buf, sizeof(buf), ghLogFile);

		if ( ( Sid9Ipt[EcuIndex].INF == 0x08 && (p = substring(buf, szINF8[count])) != 0 ) ||
		     ( Sid9Ipt[EcuIndex].INF == 0x0B && (p = substring(buf, szINFB[count])) != 0 ) )
		{
			p = substring(p, "=") + 1;
			Sid9Ipt[EcuIndex].IPT[count] = (unsigned short)strtoul (p, NULL, 10);
			continue;
		}

		return FALSE;
	}

	// Sid 9 Inf 8 data valid
	Sid9Ipt[EcuIndex].Flags = 1;

	return TRUE;
}


/*
*******************************************************************************
** ReadSid9IptFromLogFile
*******************************************************************************
*/
BOOL ReadSid9IptFromLogFile ( const char * szTestSectionStart, const char * szTestSectionEnd, SID9IPT Sid9Ipt[] )
{
	char buf[256];
	unsigned int EcuIndex;

	// log file should be open
	if ( ghLogFile == NULL )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTOFF, NO_PROMPT,
		     "Log File not open\n");
		return FALSE;
	}

	// search from beginning of file
	fseek (ghLogFile, 0, SEEK_SET);

	while ( fgets (buf, sizeof(buf), ghLogFile) != 0 )
	{
		if ( substring(buf, szTestSectionStart) != 0 )
		{
			for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
			{
				if ( ReadSid9Ipt ( szTestSectionEnd, Sid9Ipt ) == FALSE )
				{
					break;
				}
			}
			fseek ( ghLogFile, 0, SEEK_END );
			return TRUE;
		}
	}

	// move to end of file
	fseek ( ghLogFile, 0, SEEK_END );
	return FALSE;
}


//******************************************************************************
// LogSid9Inf12
//*****************************************************************************
STATUS LogSid9Inf12 ( SID9IPT pSid9Ipt[] )
{
	unsigned int    EcuIndex;
	SID_REQ         SidReq;
	SID9           *pSid9;

	STATUS Result = PASS;
	unsigned char   fInf12Supported = FALSE;


	// clear INF structure
	memset (pSid9Ipt, 0, (sizeof (SID9IPT)) * OBD_MAX_ECUS);

	/* Sid 9 Inf 12 request*/
	if ( IsSid9InfSupported (-1, 0x12) == FALSE )
	{
		if ( gUserInput.eComplianceType == US_OBDII &&
		     gOBDPlugInFlag == TRUE &&
		     gModelYear >= 2014 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $9 INF $12 not supported (Required for Plug-In Hybrid vehicles MY 2014 and later).\n" );
			return FAIL;
		}
		else
		{
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $9 INF $12 not supported\n" );
			return PASS;
		}
	}

	SidReq.SID    = 9;
	SidReq.NumIds = 1;
	SidReq.Ids[0] = 0x12;

	if ( SidRequest( &SidReq, SID_REQ_NORMAL ) == FAIL)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 INF $%X request\n", SidReq.Ids[0]);
		return FAIL;
	}


	for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
	{
		// point to IPT structure in gOBDResponse
		pSid9 = (SID9 *)&gOBDResponse[EcuIndex].Sid9Inf[0];

		// check to ensure that gOBDResponse contains appropriate data
		if ( IsSid9InfSupported (EcuIndex, 0x12) == FALSE )
		{
			continue;
		}
		else if ( gOBDResponse[EcuIndex].Sid9InfSize == 0 ||
		          pSid9->INF != 0x12 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  Invalid response to SID $9 INF $12 request\n", GetEcuId (EcuIndex) );
			Result = FAIL;
		}
		else
		{
			if ( VerifyInf12Format (EcuIndex) == FAIL )
			{
				continue;
			}

			// if ISO15765 protocol
			if ( gOBDList[gOBDListIndex].Protocol == ISO15765 )
			{
				memcpy (&pSid9Ipt[EcuIndex], &(gOBDResponse[EcuIndex].Sid9Inf[0]), sizeof (SID9IPT));
			}
			// else Legacy protocols
			else
			{
				pSid9Ipt[EcuIndex].INF  = gOBDResponse[EcuIndex].Sid9Inf[0];
				pSid9Ipt[EcuIndex].NODI = (gOBDResponse[EcuIndex].Sid9InfSize / sizeof(SID9)) * 2; // MessageCount * 2
			}

			pSid9Ipt[EcuIndex].IPT[0] = (gOBDResponse[EcuIndex].Sid9Inf[2] * 256) + gOBDResponse[EcuIndex].Sid9Inf[3];

			// If NOT test 9.19, 10.12 or 11.2, log
			if ( ( TestPhase != eTestNoFault3DriveCycle || TestSubsection != 19 ) &&
			     ( TestPhase != eTestInUseCounters || TestSubsection != 12 ) &&
			     ( TestPhase != eTestPerformanceCounters || TestSubsection != 2 ) )
			{
				// log FEOCNTR
				Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  SID $9 INF $12 FEOCNTR = %d\n",
				     GetEcuId (EcuIndex),
				     pSid9Ipt[EcuIndex].IPT[0] );
			}
		}
	}

	return Result;
}


//******************************************************************************
// VerifySid9Inf12
//*****************************************************************************
STATUS VerifySid9Inf12 ( SID9IPT *pSid9Ipt )
{
	SID_REQ         SidReq;
	unsigned int    EcuIndex;
	SID9           *pSid9;
	unsigned int    FEOCNTR;
	STATUS Result = PASS;
	unsigned char   fInf12Supported = FALSE;


	/* Sid 9 Inf 12 request*/
	if ( IsSid9InfSupported (-1, 0x12) == FALSE )
	{
		if ( gUserInput.eComplianceType == US_OBDII &&
		     gOBDPlugInFlag == TRUE &&
		     gModelYear >= 2014 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $9 INF $12 not supported (Required for Plug-In Hybrid vehicles MY 2014 and later).\n" );
			return FAIL;
		}
		else
		{
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $9 INF $12 not supported\n" );
			return PASS;
		}
	}

	SidReq.SID    = 9;
	SidReq.NumIds = 1;
	SidReq.Ids[0] = 0x12;

	if ( SidRequest( &SidReq, SID_REQ_NORMAL ) == FAIL)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 INF $%X request\n", SidReq.Ids[0]);
		return FAIL;
	}


	for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
	{
		// point to IPT structure in gOBDResponse
		pSid9 = (SID9 *)&gOBDResponse[EcuIndex].Sid9Inf[0];


		// check to ensure that gOBDResponse contains appropriate data
		if ( IsSid9InfSupported (EcuIndex, 0x12) == FALSE )
		{
			continue;
		}
		else if ( gOBDResponse[EcuIndex].Sid9InfSize == 0 ||
		          pSid9->INF != 0x12 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  Invalid response to SID $9 INF $12 request\n", GetEcuId (EcuIndex) );
			Result = FAIL;
		}
		else
		{
			FEOCNTR = (gOBDResponse[EcuIndex].Sid9Inf[2] * 256) + gOBDResponse[EcuIndex].Sid9Inf[3];

			// log FEOCNTRs
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $12 FEOCNTR = %d\n",
			     GetEcuId (EcuIndex),
			     FEOCNTR );

			if ( FEOCNTR <= (unsigned int)(pSid9Ipt[EcuIndex].IPT[0]) )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  SID $9 INF $12 FEOCNTR has not incremented since Test 10.10.4\n", GetEcuId (EcuIndex) );
				Result = FAIL;
			}
		}
	}

	return Result;
}


/*
*******************************************************************************
** RunDynamicTest10
*******************************************************************************
*/
STATUS RunDynamicTest10 (unsigned long tEngineStartTimeStamp)
{
	unsigned char  fSubTestStatus = SUB_TEST_NORMAL;
	unsigned char  fLastTestStatus = SUB_TEST_INITIAL; // set to force an update the first time
	unsigned char  Sid1Pid1C;
	unsigned int   EcuIndex;
	unsigned int   TestState;
	unsigned int   bDone = 0;
	unsigned int   bLoop;
	unsigned int   bFail;
	unsigned int   bRunTimeSupport;
	unsigned int   bVSSSupport;
	unsigned int   bRPMSupport;
	BOOL           useRPM = FALSE;  // indicate use of RPM for Idle and Drive time calculations
	unsigned int   bSid9Ipt = 0;    // support INF for IPT data, default 0 = no support
	unsigned long  t1SecTimer, tDelayTimeStamp;

	BOOL           ErrorPrinted = FALSE;    // indicates whether or not the failure message has already been printed

	BOOL           bPermDTC;

	unsigned short tTestStartTime, tTestCompleteTime;
	unsigned short tTempTime;
	unsigned short tIdleTime;       // time at idle (stops at 30)
	unsigned short tAtSpeedTime;    // time at speeds > 25mph (stops at 300)
	unsigned short RPM;
	unsigned short SpeedMPH;
	unsigned short SpeedKPH;
	unsigned short RunTime;
	unsigned short tFueledRunTime;
	unsigned short tFRTempTime;
	unsigned short tObdCondTimestamp[OBD_MAX_ECUS];

	SID_REQ        SidReq;
	SID9IPT        Sid9Ipt[OBD_MAX_ECUS];
	SID9IPT        Sid9FEOCNTR[OBD_MAX_ECUS];
	SID1          *pSid1;

	STATUS eResult = PASS;


	// determine PID support
	bRunTimeSupport = IsSid1PidSupported (-1, 0x1F);

	bVSSSupport = IsSid1PidSupported (-1, 0x0D);

	bRPMSupport = IsSid1PidSupported (-1, 0x0C);

	if ( bRPMSupport == FALSE && bRunTimeSupport == FALSE )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $1 PIDs $0C (RPM) and $1F (RUNTM) not supported\n");
		return (FAIL);
	}

	//-------------------------------------------------------------------------
	// Request OBD_Type - SID 1 PID $1C.  Needed to determine wether to use
	// Vehicle Speed (KPH/MPH) or Egine Speed (RPM) to determine idel and drive time.
	//-------------------------------------------------------------------------
	SidReq.SID    = 1;
	SidReq.NumIds = 1;
	SidReq.Ids[0] = 0x1c;

	eResult = SidRequest( &SidReq, SID_REQ_NO_PERIODIC_DISABLE|SID_REQ_IGNORE_NO_RESPONSE );

	if (eResult == PASS)
	{
		for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
		{
			if (gOBDResponse[EcuIndex].Sid1PidSize > 0)
			{
				pSid1 = (SID1 *)&gOBDResponse[EcuIndex].Sid1Pid[0];

				if (pSid1->PID == 0x1c)
				{
					Sid1Pid1C = pSid1->Data[0];
					break;
				}
			}
		}
	}


	//-------------------------------------------------------------------------
	// Determine whether to use Vehicle Speed (KPH/MPH) or Egine Speed (RPM) for idel and drive time calculations.
	//-------------------------------------------------------------------------
	if ( (gUserInput.eComplianceType == HD_OBD && gOBDDieselFlag == TRUE) ||
	     (gUserInput.eComplianceType == US_OBDII && gOBDDieselFlag == TRUE && Sid1Pid1C == 0x22) )
	{
		useRPM = TRUE;
	}
	else if ( (gUserInput.eComplianceType == HD_EOBD) ||
	          (gUserInput.eComplianceType == US_OBDII && gOBDDieselFlag == TRUE && gUserInput.eVehicleType == MD) )
	{
		if ( bVSSSupport )
		{
			useRPM = FALSE;
		}
		else
		{
			useRPM = TRUE;
		}
	}
	else
	{
		useRPM = FALSE;
	}


	//-------------------------------------------
	/// Initialize Array */
	//-------------------------------------------
	memset ( Sid9Ipt, 0x00, sizeof(Sid9Ipt) );

	if ( IsSid9InfSupported ( -1, 0x08 ) == TRUE )
	{
		bSid9Ipt = 0x08;
	}
	else if ( IsSid9InfSupported ( -1, 0x0B ) == TRUE )
	{
		bSid9Ipt = 0x0B;
	}
	else if ( ( (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) &&
	            gModelYear >= 2005 ) ||
	          (gUserInput.eComplianceType == EOBD_WITH_IUMPR || gUserInput.eComplianceType == HD_EOBD) )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 IPT (INF $8 or INF $B) not supported\n" );
		return FAIL;
	}
	else
	{
		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 IPT (INF $8 or INF $B) not supported\n" );
	}


	//-------------------------------------------
	// initialize static text elements
	//-------------------------------------------
	if ( TestPhase == eTestNoFault3DriveCycle )
	{
		init_screen (_string_elements_1st_9, _num_string_elements_1st_9);
	}
	else
	{
		init_screen (_string_elements_1st_10, _num_string_elements_1st_10);
	}

	if ( useRPM == TRUE )
	{
		place_screen_text (_string_elements10RPM, _num_string_elements10RPM);
	}
	else
	{
		place_screen_text (_string_elements10, _num_string_elements10);
	}

	for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
	{
		// initialize ECU IDs
		SetFieldHex (ECU_ID+EcuIndex, GetEcuId (EcuIndex));

		// initialize initial OBDCONDs
		SetFieldDec (INITIAL_OBDCOND+EcuIndex, Test10_10_Sid9Ipt[EcuIndex].IPT[0]);

		// initialize initial IGNCTRs
		SetFieldDec (INITIAL_IGNCTR+EcuIndex, Test10_10_Sid9Ipt[EcuIndex].IPT[1]);

		if ( gOBDPlugInFlag == TRUE )
		{
			// initialize initial FEOCNTRs
			SetFieldDec (INITIAL_FEOCNTR+EcuIndex, Test10_10_Sid9FEOCNTR[EcuIndex].IPT[0]);
		}

		tObdCondTimestamp[EcuIndex] = 0;

		// if INF 8/B not supported, print 0 (INF8/B will never update)
		if ( bSid9Ipt == 0 )
		{
			SetFieldDec (CURRENT_OBDCOND+EcuIndex, Sid9Ipt[EcuIndex].IPT[0]);
			SetFieldDec (CURRENT_IGNCTR+EcuIndex,  Sid9Ipt[EcuIndex].IPT[1]);
		 }
	}

	// flush the STDIN stream of any user input before loop
	clear_keyboard_buffer ();

	RPM = SpeedMPH = SpeedKPH = RunTime = 0;
	tFueledRunTime = 0;
	tFRTempTime = 0;
	tAtSpeedTime = 0;
	tIdleTime = 0;
	tTempTime = 0;
	tTestCompleteTime = 0;
	tTestStartTime = (unsigned short)(tEngineStartTimeStamp / 1000);
	tDelayTimeStamp = t1SecTimer = GetTickCount ();

	TestState = 0xFF;

	//-------------------------------------------
	// loop until test completes
	//-------------------------------------------
	for (;;)
	{
		if ( TestPhase == eTestNoFault3DriveCycle )
		{
			//-------------------------------------------
			// request Permanent Codes - SID A
			//-------------------------------------------
			SidReq.SID    = 0x0A;
			SidReq.NumIds = 0;

			eResult = SidRequest( &SidReq, SID_REQ_NO_PERIODIC_DISABLE|SID_REQ_IGNORE_NO_RESPONSE );

			if ( eResult == FAIL )
			{
				if ( ErrorPrinted == FALSE )
				{
					Log( FAILURE, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
					     "SID $0A request\n" );
					fSubTestStatus |= SUB_TEST_FAIL;
				}
				ErrorPrinted = TRUE;
			}
			else
			{
				if (eResult != PASS)
				{
					/* cover the case where a response was early/late */
					fSubTestStatus |= SUB_TEST_FAIL;
				}

				bPermDTC = FALSE;
				for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
				{
					if ( (gOBDResponse[EcuIndex].SidA[0] != 0x00) || (gOBDResponse[EcuIndex].SidA[1] != 0x00) )
					{
						bPermDTC = TRUE;
					}
				}

				if ( bPermDTC == FALSE )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "SID $0A - DTC erased prematurely.\n" );
					return (FAIL);
				}
			}
		}


		//-------------------------------------------
		// request RPM - SID 1 PID $0C
		//-------------------------------------------
		if ( bRPMSupport == TRUE )
		{
			SidReq.SID    = 1;
			SidReq.NumIds = 1;
			SidReq.Ids[0] = 0x0c;

			eResult = SidRequest( &SidReq, SID_REQ_NO_PERIODIC_DISABLE|SID_REQ_IGNORE_NO_RESPONSE );

			if (eResult == FAIL)
			{
				Log( FAILURE, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
				     "SID $1 PID $0C request\n");
				fSubTestStatus |= SUB_TEST_FAIL;
			}
			else
			{
				if (eResult != PASS)
				{
					/* cover the case where a response was early/late */
					fSubTestStatus |= SUB_TEST_FAIL;
				}

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
							break;
						}
					}
				}

				if (EcuIndex >= gUserNumEcus)
				{
					Log( FAILURE, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
					     "SID $1 PID $0C missing response\n");
					fSubTestStatus |= SUB_TEST_FAIL;
				}
			}
		}

		//-------------------------------------------
		// request Speed - SID 1 PID $0D
		//-------------------------------------------
		if ( bVSSSupport == TRUE  )
		{
			SidReq.SID    = 1;
			SidReq.NumIds = 1;
			SidReq.Ids[0] = 0x0d;

			eResult = SidRequest( &SidReq, SID_REQ_NO_PERIODIC_DISABLE|SID_REQ_IGNORE_NO_RESPONSE );

			if (eResult == FAIL)
			{
				Log( FAILURE, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
				     "SID $1 PID $0D request\n");
				fSubTestStatus |= SUB_TEST_FAIL;
			}
			else
			{
				if (eResult != PASS)
				{
					/* cover the case where a response was early/late */
					fSubTestStatus |= SUB_TEST_FAIL;
				}

				for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
				{
					if (gOBDResponse[EcuIndex].Sid1PidSize > 0)
					{
						pSid1 = (SID1 *)&gOBDResponse[EcuIndex].Sid1Pid[0];

						if (pSid1->PID == 0x0d)
						{
							double dTemp = pSid1->Data[0];

							SpeedKPH = pSid1->Data[0];

							// convert from km/hr to mile/hr
							SpeedMPH = (unsigned short)( ((dTemp * 6214) / 10000) + 0.5 );
							break;
						}
					}
				}

				if (EcuIndex >= gUserNumEcus)
				{
					Log( FAILURE, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
					     "SID $1 PID $0D missing response\n");
					fSubTestStatus |= SUB_TEST_FAIL;
				}
			}
		}

		//-------------------------------------------
		// request engine RunTime - SID 1 PID $1F
		//-------------------------------------------
		if (bRunTimeSupport == TRUE)
		{
			SidReq.SID    = 1;
			SidReq.NumIds = 1;
			SidReq.Ids[0] = 0x1f;

			eResult = SidRequest( &SidReq, SID_REQ_NO_PERIODIC_DISABLE|SID_REQ_IGNORE_NO_RESPONSE );

			if (eResult == FAIL)
			{
				Log( FAILURE, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
				     "SID $1 PID $1F request\n");
				fSubTestStatus |= SUB_TEST_FAIL;
			}
			else
			{
				if (eResult != PASS)
				{
					/* cover the case where a response was early/late */
					fSubTestStatus |= SUB_TEST_FAIL;
				}

				for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
				{
					if (gOBDResponse[EcuIndex].Sid1PidSize > 0)
					{
						pSid1 = (SID1 *)&gOBDResponse[EcuIndex].Sid1Pid[0];

						if (pSid1->PID == 0x1f)
						{
							RunTime = pSid1->Data[0];
							RunTime = RunTime << 8 | pSid1->Data[1];    // 1 cnt = 1 sec
							break;
						}
					}
				}

				if (EcuIndex >= gUserNumEcus)
				{
					Log( FAILURE, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
					     "SID $1 PID $1F missing response\n");
					fSubTestStatus |= SUB_TEST_FAIL;
				}
			}
		}
		else
		{
			RunTime = (unsigned short)(GetTickCount () / 1000) - tTestStartTime;
		}

		//-------------------------------------------
		// Get SID 9 IPT
		//-------------------------------------------
		if ( bSid9Ipt != 0 )
		{
			SidReq.SID    = 9;
			SidReq.NumIds = 1;
			SidReq.Ids[0] = bSid9Ipt;

			eResult = SidRequest( &SidReq, SID_REQ_NO_PERIODIC_DISABLE|SID_REQ_IGNORE_NO_RESPONSE );

			if (eResult == FAIL)
			{
				Log( FAILURE, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
				     "SID $9 INF $%X request\n", SidReq.Ids[0] );
				fSubTestStatus |= SUB_TEST_FAIL;
			}
			else
			{
				if (eResult != PASS)
				{
					/* cover the case where a response was early/late */
					fSubTestStatus |= SUB_TEST_FAIL;
				}

				for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
				{
					if ( GetSid9IptData (EcuIndex, &Sid9Ipt[EcuIndex]) == PASS )
					{
						SetFieldDec (CURRENT_OBDCOND+EcuIndex, Sid9Ipt[EcuIndex].IPT[0]);
						SetFieldDec (CURRENT_IGNCTR+EcuIndex,  Sid9Ipt[EcuIndex].IPT[1]);

						// check when OBDCOND counters increment
						if ( (tObdCondTimestamp[EcuIndex] == 0) &&
						     (Sid9Ipt[EcuIndex].IPT[0] > Test10_10_Sid9Ipt[EcuIndex].IPT[0]) )
						{
							tObdCondTimestamp[EcuIndex] = RunTime;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "Idle Time = %d;  Speed Time = %d;  Run Time = %d  *OBDCOND DONE*\n",
							     tIdleTime, tAtSpeedTime, RunTime);
							/*
							 * check for extremely early increments of OBDCOND and FAIL
							 * (that is, OBDCOND that increment more than 30 seconds before the test will end)
							 */
							if (tIdleTime == 0 || tAtSpeedTime < 270 || RunTime < 570)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "OBDCOND incremented much too soon\n");
								fSubTestStatus |= SUB_TEST_FAIL;
							}
						}
					}
				}
			}
		}

		//-------------------------------------------
		// Get SID 9 FEOCNTR
		//-------------------------------------------
		if ( gOBDPlugInFlag == TRUE )
		{
			if ( LogSid9Inf12 ( Sid9FEOCNTR ) == PASS)
			{
				for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
				{
					SetFieldDec (CURRENT_FEOCNTR+EcuIndex, Sid9FEOCNTR[EcuIndex].IPT[0]);
				}
			}
		}

		//-------------------------------------------
		// Update current PIDs, total engine time
		//-------------------------------------------
		if ( bRunTimeSupport == FALSE || useRPM == TRUE )
		{
			SetFieldDec (RPM_INDEX, RPM);
		}
		else
		{
			SetFieldDec (SPEED_INDEX_MPH, SpeedMPH);
			SetFieldDec (SPEED_INDEX_KPH, SpeedKPH);
		}
		SetFieldDec (TOTAL_DRIVE_TIMER, RunTime);

		//-------------------------------------------
		// Update test status
		//-------------------------------------------
		if (fSubTestStatus != fLastTestStatus)
		{
			if ((fSubTestStatus & SUB_TEST_FAIL) == SUB_TEST_FAIL)
			{
				setrgb (HIGHLIGHTED_TEXT);
				SetFieldText (TESTSTATUS_INDEX, "FAILURE");
				setrgb (NORMAL_TEXT);
			}
			else
			{
				SetFieldText (TESTSTATUS_INDEX, "Normal");
			}

			fLastTestStatus = fSubTestStatus;
		}


		//-------------------------------------------
		// Determine phase of dynamic test
		// update screen
		//-------------------------------------------
		bLoop = TRUE;
		while (bLoop)
		{
			bLoop = FALSE;
			switch (TestState)
			{
				case 0xFF:   // First Time ONLY - covers the case were the engine has already been idling
					tTempTime = RunTime;
					tFRTempTime = RunTime;

					if ( (bRunTimeSupport == TRUE) ? (RunTime > 0) : (RPM > 450) )
					{
						if (SpeedKPH <= 1)
						{
							TestState = 1;
							tTempTime = RunTime;
							bLoop = TRUE;
							break;
						}
					}

					//
					// engine is not running or SpeedKPH >1... don't know if there was an idle
					// set-up to intentionally fall thru to case 0
					//
					TestState = 0;

				case 0:     // wait until idle  (RunTime > 0 or RPM > 450)
					if ( (bRunTimeSupport == TRUE) ? (RunTime > 0) : (RPM > 450) )
					{
						// check 600 seconds cumulative time
						if ( ((bDone & CUMULATIVE_TIME) != CUMULATIVE_TIME) && (RunTime >= 600) )
						{
							tTestCompleteTime = RunTime;
							bDone |= CUMULATIVE_TIME;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "Idle Time = %d;  Speed Time = %d;  Run Time = %d  *RUN TIME DONE*\n",
							     tIdleTime, tAtSpeedTime, RunTime);
						}

						// Check for idle speed
						if ( ( (useRPM == FALSE) ? (SpeedKPH <= 1 ) : (RPM > 450) ) &&
						     tIdleTime < 30 )
						{
							TestState = 1;
							tIdleTime = 0;
							bLoop = TRUE;
						}
						// Check for drive speed
						else if ( ( (useRPM == FALSE) ? (SpeedKPH >= 40 ) : (RPM > 1150) ) &&
						          tAtSpeedTime < 300 )
						{
							TestState = 2;
							bLoop = TRUE;
						}
						// Check for test complete
						else if ( bDone == (IDLE_TIME | ATSPEED_TIME | CUMULATIVE_TIME | FEO_TIME) )
						{
							TestState = 3;
							bLoop = TRUE;
						}
						else
						{
							tTempTime = RunTime;
						}
					}
					break;

				case 1:     // 30 seconds continuous time at idle
					// (   (VSS <= 1kph (if supported) or 450 < RPM < 1150 (if VSS not supported)) AND
					//     (RunTime > 0 (if supported) or RPM > 450 (if RUNTIME not supported)) )
					if ( ( (useRPM == FALSE) ? (SpeedKPH <= 1 ) : (RPM > 450 && RPM < 1150 ) ) &&
					     ( (bRunTimeSupport == FALSE) ? (RPM > 450) : 1) )
					{
						// check 600 seconds cumulative time
						if ( ((bDone & CUMULATIVE_TIME) != CUMULATIVE_TIME) && (RunTime >= 600) )
						{
							tTestCompleteTime = RunTime;
							bDone |= CUMULATIVE_TIME;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "Idle Time = %d;  Speed Time = %d;  Run Time = %d  *RUN TIME DONE*\n",
							     tIdleTime, tAtSpeedTime, RunTime);
						}

						// check idle time
						tIdleTime = min(tIdleTime + RunTime - tTempTime, 30);
						tTempTime = RunTime;

						SetFieldDec (IDLE_TIMER, tIdleTime);

						if (tIdleTime >= 30)
						{
							tTestCompleteTime = RunTime;
							bDone |= IDLE_TIME;
							TestState = 0;
							bLoop = TRUE;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "Idle Time = %d;  Speed Time = %d;  Run Time = %d  *IDLE DONE*\n",
							     tIdleTime, tAtSpeedTime, RunTime);
						}

					}
					else
					{
						TestState = 0;
						bLoop = TRUE;
					}
					break;

				case 2:     // 300 seconds cumulative time at SpeedKPH >= 40 KPH
					// (   (VSS >= 40kph (if supported) or RPM >= 1150 (if VSS not supported)) AND
					//     (RunTime > 0 (if supported) or RPM >= 450 (if RUNTIME not supported)) )
					if ( ( (useRPM == FALSE) ? (SpeedKPH >= 40) : (RPM >= 1150 ) ) &&
					     ( (bRunTimeSupport == FALSE) ? (RPM > 450 ) : 1) )
					{
						// check 600 seconds cumulative time
						if ( ((bDone & CUMULATIVE_TIME) != CUMULATIVE_TIME) && (RunTime >= 600) )
						{
							tTestCompleteTime = RunTime;
							bDone |= CUMULATIVE_TIME;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "Idle Time = %d;  Speed Time = %d;  Run Time = %d  *RUN TIME DONE*\n",
							     tIdleTime, tAtSpeedTime, RunTime);
						}

						// check at speed time
						tAtSpeedTime = min(tAtSpeedTime + RunTime - tTempTime, 300);
						tTempTime = RunTime;

						SetFieldDec (SPEED_25_MPH_TIMER, tAtSpeedTime);

						if (tAtSpeedTime >= 300)
						{
							tTestCompleteTime = RunTime;
							bDone |= ATSPEED_TIME;
							TestState = 0;
							bLoop = TRUE;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "Idle Time = %d;  Speed Time = %d;  Run Time = %d  *AT SPEED DONE*\n",
							     tIdleTime, tAtSpeedTime, RunTime);
						}
					}
					else
					{
						TestState = 0;
						bLoop = TRUE;
					}
					break;

				case 3:     // check for test pass/fail
					if (bSid9Ipt != 0)
					{
						// check if any OBDCOND counters increment too soon or too late
						bFail = FALSE;
						for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
						{
							if ( (Test10_10_Sid9Ipt[EcuIndex].Flags != 0) && (tObdCondTimestamp[EcuIndex] != 0) )
							{
								if ( tObdCondTimestamp[EcuIndex] < (tTestCompleteTime - 20) )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  OBDCOND incremented too soon (RUNTM = %u)\n",
									     GetEcuId(EcuIndex),
									     tObdCondTimestamp[EcuIndex] );
									bFail = TRUE;
								}

								if ( tObdCondTimestamp[EcuIndex] > (tTestCompleteTime + 20) )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  OBDCOND incremented too late (RUNTM = %u)\n",
									     GetEcuId(EcuIndex),
									     tObdCondTimestamp[EcuIndex]);
									bFail = TRUE;
								}
							}
						}

						if (bFail == TRUE)
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "Idle Time = %d;  Speed Time = %d;  Run Time = %d\n",
							     tIdleTime, tAtSpeedTime, RunTime);
							return FAIL;
						}

						// check for OBDCOND for increment
						for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
						{
							if ( (Test10_10_Sid9Ipt[EcuIndex].Flags != 0) && (tObdCondTimestamp[EcuIndex] == 0) )
							{
								break;      // OBDCOND counter not yet incremented - keep running test
							}
						}

						if (EcuIndex >= gUserNumEcus)
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "Idle Time = %d;  Speed Time = %d;  Run Time = %d  *OBDCOND TEST DONE*\n",
							     tIdleTime, tAtSpeedTime, RunTime);
							return PASS;  // Test complete
						}

						// check for timeout
						if ( RunTime >= (tTestCompleteTime + 20) )
						{
							if (TestPhase == eTestInUseCounters)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "More than 20 seconds has elapsed since the test completed "
								     "and not all the ECUs have incremented OBDCOND\n");
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "Idle Time = %d;  Speed Time = %d;  Run Time = %d\n",
								     tIdleTime, tAtSpeedTime, RunTime);
								return FAIL;
							}
							else
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "Idle Time = %d;  Speed Time = %d;  Run Time = %d  *PDTC TEST DONE*\n",
								     tIdleTime, tAtSpeedTime, RunTime);
								return PASS;
							}
						}
					}
					else
					{
						// check for timeout
						if (RunTime >= (tTestCompleteTime + 20) )
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "Idle Time = %d;  Speed Time = %d;  Run Time = %d  *TEST DONE*\n",
							     tIdleTime, tAtSpeedTime, RunTime);
							return PASS;
						}
					}

					break;

			} // end switch (TestState)

			// Check for 10s of fueled operation for PHEV
			if ( gOBDPlugInFlag == TRUE &&
			     (bDone & FEO_TIME) != FEO_TIME &&
			     tFueledRunTime < 10 )
			{
				if ( RPM > 450 )
				{
					tFueledRunTime = min(tFueledRunTime + (RunTime - tFRTempTime), 10);
					tFRTempTime = RunTime;

					SetFieldDec (PHEV_TIMER, tFueledRunTime);

					if ( tFueledRunTime >= 10 )
					{
						bDone |= FEO_TIME;
						Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "Idle Time = %d;  Speed Time = %d;  Run Time = %d  *FUELED ENGINE TIME DONE*\n",
						     tIdleTime, tAtSpeedTime, RunTime);
					}
				}
				else // RPM < 450
				{
					tFRTempTime = RunTime;
				}
			}
			else if ( gOBDPlugInFlag == FALSE )
			{
				bDone |= FEO_TIME;
			}

		} // end while (bLoop)

		//-------------------------------------------
		// Check for ESC key and sleep
		//-------------------------------------------
		do
		{
			if (_kbhit () != 0)
			{
				if (_getch () == 27)    // ESC key
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "Drive Cycle Test aborted by user\n\n");
					Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "Idle Time = %d;  Speed Time = %d;  Run Time = %d\n",
					     tIdleTime, tAtSpeedTime, RunTime);
					return FAIL;
				}
			}

			tDelayTimeStamp = GetTickCount ();

			Sleep ( min (1000 - (tDelayTimeStamp - t1SecTimer), 50) );

		} while (tDelayTimeStamp - t1SecTimer < 1000);

		t1SecTimer = tDelayTimeStamp;
	}

	Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "Error in Drive Cycle Test\n\n");
	return FAIL;
}


/*
*******************************************************************************
** DisplayDynamicTest10Results
*******************************************************************************
*/
void DisplayDynamicTest10Results ( void )
{
	unsigned int   EcuIndex;
	unsigned int   bSid9Ipt = 0;  // support INF for IPT data, default 0 = no support

	SID9IPT        Sid9Ipt;

	/* Initialize Array */
	memset ( &Sid9Ipt, 0x00, sizeof(Sid9Ipt) );


	for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
	{
		if ( IsSid9InfSupported ( EcuIndex, 0x08 ) == TRUE )
		{
			bSid9Ipt = 0x08;
		}
		else if ( IsSid9InfSupported ( EcuIndex, 0x0B ) == TRUE )
		{
			bSid9Ipt = 0x0B;
		}

		// if IPT data is supported
		if ( bSid9Ipt != 0 )
		{
			// ECU IDs
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %02X supports SID $9 INF $%X IPT data\n",
			     GetEcuId (EcuIndex),
			     bSid9Ipt );

			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "         Initial    Current\n");
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "OBDCOND  %-8d    %-8d\n",Test10_10_Sid9Ipt[EcuIndex].IPT[0], Last_Sid9IPT[EcuIndex].IPT[0] );
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "IGNCTR   %-8d    %-8d\n",Test10_10_Sid9Ipt[EcuIndex].IPT[1], Last_Sid9IPT[EcuIndex].IPT[1] );
		}

		// IPT data is not supported
		else
		{
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %02X does not support IPT data\n",
			     GetEcuId (EcuIndex) );
		}

		bSid9Ipt = 0;
	}
}