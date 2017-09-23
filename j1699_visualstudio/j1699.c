/*
**
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
** The following design parameters/rationale were used during the development
** of this source code:
** 1 - Keep it as simple as possible
** 2 - Single source code file (simplify publishing, compiling and revision
**		 control)
** 3 - Make response data look like ISO15765/ISO15031 to simplify processing
**		 wherever possible
** 4 - Log all screen text, user prompts and diagnostic message traffic
** 5 - Use globals for response information storage to simplify usage across
**		 functions
** 6 - Make all code capable of being compiled with the free GNU C-compiler
** 7 - Use only native C-language, ANSI-C runtime library and SAE J2534 API
**		 functions
**
** Where to get the free C-language compiler:
**     Go to www.cygwin.com website and download and install the Cygwin
**     environment.
**
** How to build:
**     From the Cygwin shell environment type:
**     "gcc -mno-cygwin *.c -o j1699.exe"
**     in the directory that contains the j1699.c source code file and a
**     j2534.h header file from your PassThru interface vendor.
**
** How to run:
**     First you will need to install the J2534 software from the vendor that
**     supplied your PassThru vehicle interface hardware. Then, from a MS-DOS
**     window, in the directory with the j1699.exe file, type "J1699" to run
**     the program.  User prompts will lead you through the program.
**
** Program flow:
**     The program flow is the same as the SAE J1699-3 document.  Please refer
**     to the document for more information.
**
** Log file:
**     The log filename created will be a concatenation of the vehicle
**     information that the user enters (model year, manufacturer and make)
**     along with a numeric value and a ".log" extension.  The numeric value
**     will automatically be incremented by the program to the next available
**     number.
**
********************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "j2534.h"
#include "j1699.h"

/*
** j1699 revision
*/
const char szJ1699_VER[] = "J1699-3";

const char gszAPP_REVISION[] = "16.00.01";

const char szBUILD_DATE[] = __DATE__;

/*
** j2534 interface pointers
*/
PTCONNECT PassThruConnect = 0;
PTDISCONNECT PassThruDisconnect = 0;
PTREADMSGS PassThruReadMsgs = 0;
PTWRITEMSGS PassThruWriteMsgs = 0;
PTSTARTPERIODICMSG PassThruStartPeriodicMsg = 0;
PTSTOPPERIODICMSG PassThruStopPeriodicMsg = 0;
PTSTARTMSGFILTER PassThruStartMsgFilter = 0;
PTSTOPMSGFILTER PassThruStopMsgFilter = 0;
PTSETPROGRAMMINGVOLTAGE PassThruSetProgrammingVoltage = 0;
PTREADVERSION PassThruReadVersion = 0;
PTGETLASTERROR PassThruGetLastError = 0;
PTIOCTL PassThruIoctl = 0;
PTOPEN  PassThruOpen = 0;
PTCLOSE PassThruClose = 0;

/*
** OBD type definitions
*/
const char *OBD_TYPE[MAX_OBD_TYPES] = {
"UNKNOWN",                      // 0x00
"OBD II",                       // 0x01
"OBD",                          // 0x02
"OBD and OBDII",                // 0x03
"OBD I",                        // 0x04
"NO OBD",                       // 0x05
"EOBD",                         // 0x06
"EOBD and OBD II",              // 0x07
"EOBD and OBD",                 // 0x08
"EOBD, OBD, and OBD II",        // 0x09
"JOBD",                         // 0x0A
"JOBD and OBD II",              // 0x0B
"JOBD and EOBD",                // 0x0C
"JOBD, EOBD, and OBD II",       // 0x0D
"OBD, EOBD, and KOBD",          // 0x0E
"OBD, OBDII, EOBD, and KOBD",   // 0x0F
"reserved",                     // 0x10
"EMD",                          // 0x11
"EMD+",                         // 0x12
"HD OBD-C",                     // 0x13
"HD OBD",                       // 0x14
"WWH OBD",                      // 0x15
"reserved",                     // 0x16
"HD EOBD-I",                    // 0x17
"HD EOBD-I N",                  // 0x18
"HD EOBD-II",                   // 0x19
"HD EOBD-II N",                 // 0x1A
"reserved",                     // 0x1B
"OBDBr-1",                      // 0x1C
"OBDBr-2 and 2+",               // 0x1D
"KOBD",                         // 0x1E
"IOBD I",                       // 0x1F
"IOBD II",                      // 0x20
"HD EOBD-VI",                   // 0x21
"OBD, OBDII, and HD OBD",       // 0x22
"OBDBr-3",                      // 0x23
"Motorcycle, Euro OBD-I",       // 0x24
"Motorcycle, Euro OBD-II",      // 0x25
"Motorcycle, China OBD-I",      // 0x26
"Motorcycle, Taiwan OBD-I",     // 0x27
"Motorcycle, Japan OBD-I",      // 0x28
"China Nationwide Stage 6",     // 0x29
"Brazil OBD Diesel"             // 0x2A
};

/* Global variables */
char gLogFileName[MAX_LOGFILENAME] = {0};
char gUserModelYear[80] = {0};
int  gModelYear = 0;
char gUserMake[80] = {0};
char gUserModel[80] = {0};
unsigned long gUserNumEcus = 0;

unsigned long gRespTimeOutofRange = 0;
unsigned long gRespTimeTooSoon = 0;
unsigned long gRespTimeTooLate = 0;
unsigned long gDetermineProtocol = 0;
/*
** Added variables to support the required operator prompts
*/
unsigned long gUserNumEcusReprgm = 0;

USER_INPUT gUserInput;                          // structure for user selected compliance test information


/*  Compliance Type definitions */
char *gComplianceTestListString[] =             // string of compliance test options
{
	"    1 US OBD-II\n"
	"    2 Heavy Duty US OBD (vehicle > 14,000 lb GVWR US, >= 2610 kg RW Europe)\n"
	"    3 European OBD with IUMPR\n"
	"    4 European OBD without IUMPR\n"
	"    5 Heavy Duty European OBD (vehicle > 14,000lb GVWR US, >= 2610kg RW Europe)\n"
	"    6 India OBD without IUMPR\n"
	"    7 Heavy Duty India OBD without IUMPR (vehicle > 3.5T GVW)\n"
	"    8 Brazil OBD without IUMPR\n"
};
#define COMPLIANCE_TEST_LIST_COUNT 8

/*  ISO15765 workaround options */
char *gISO15765Options[] =            // string of ISO15765 workaround options
{
	"    1 ISO15765 @ 500k and 250k\n"
	"    2 ISO15765 @ 500k only, BUS_OFF work-around)\n"
	"    3 ISO15765 @ 250k only, BUS_OFF work-around)\n"
};
#define ISO15765OPTIONS 3


char *gComplianceTestTypeStrings[] =            // strings that represent the user selected compliance test
{
	"UNKNOWN",
	"US OBD II",
	"European OBD with IUMPR",
	"European OBD without IUMPR",
	"HD OBD",
	"HD EOBD",
	"India OBD without IUMPR",
	"HD IOBD without IUMPR",
	"Brazil OBD without IUMPR",
};


/*  Engine Type definitions */
char *gEngineListString[] =            // string of engine type options
{
	"    1 Spark Ignition\n"
	"    2 Compression Ignition (i.e. Diesel)\n"
};
#define ENGINE_LIST_COUNT 2

char *gEngineTypeStrings[] =            // strings that represent the user selected engine type
{
	"UNKNOWN",
	"GASOLINE",
	"DIESEL"
};


/*  Powertrain Type definitions */
char *gPwrtrnListString[] =            // string of powertrain type options
{
	"    1 Conventional\n"
	"    2 Stop/Start Vehicle\n"
	"    3 Hybrid Electric Vehicle\n"
	"    4 Plug-In Hybrid Electric Vehicle\n"
};
#define PWRTRN_LIST_COUNT 4

char *gPwrTrnTypeStrings[] =            // strings that represent the user selected powertrain type
{
	"UNKNOWN",
	"CONVENTIONAL",
	"STOP/START",
	"HEV",
	"PHEV"
};


/*  Vehicle Type definitions */
char *gVehicleListString[] =            // string of vehicle type options
{
	"    1 Light/Medium Duty Vehicle Chassis Certified\n"
	"    2 Medium Duty Truck Engine Dyno Certified\n"
};
#define VEHICLE_LIST_COUNT 2

char *gVehicleTypeStrings[] =            // strings that represent the user selected vehicle type
{
	"UNKNOWN",
	"LIGHT/MEDIUM DUTY VEHICLE",
	"MEDIUM DUTY TRUCK ENGINE DYNO CERTIFIED",
	"HEAVY DUTY TRUCK"
};


/*  String Definitions */
char *g_rgpcDisplayStrings[] =  // strings used for display/logging, kept here to insure consistent text
{
	"Model Year of this vehicle? " ,                                       // DSPSTR_PRMPT_MODEL_YEAR
	"How many OBD-II ECUs are on this vehicle (1 to 8)?  ",                // DSPSTR_PRMPT_OBD_ECU
	"How many reprogrammable OBD-II ECUs are on this vehicle (1 to 8)?  ", // DSPSTR_PRMPT_RPGM_ECU
	"What type of engine is in this vehicle?  ",                           // DSPSTR_PRMPT_ENG_TYPE
	"What type of powertrain is in this vehicle?  ",                       // DSPSTR_PRMPT_PWRTRN_TYPE
	"What type of compliance test is to be performed?  ",                  // DSPSTR_STMT_COMPLIANCE_TYPE
	"What type of vehicle is being tested?  ",                             // DSPSTR_PRMPT_VEH_TYPE
	"What type of ISO15765 is to be used?  ",         // DSPSTR_PRMPT_ISO15765_TYPE
	"Total number of USER WARNINGS = ",               // DSPSTR_RSLT_TOT_USR_ERROR
	"Total number of J2534 FAILURES = ",              // DSPSTR_RSLT_TOT_J2534_FAIL
	"Total number of WARNINGS = ",                    // DSPSTR_RSLT_TOT_WARN
	"Total number of FAILURES = ",                    // DSPSTR_RSLT_TOT_FAIL
	"Total number of COMMENTS = "                     // DSPSTR_RSLT_TOT_COMMENTS
};


unsigned char gOBDKeywords[2] = {0};

unsigned char gOBDTestAborted = FALSE;          // set (in ABORT_RETURN only) if any test was aborted,
                                                // here is the only place this should be set to FALSE

unsigned char gOBDTestSectionAborted = FALSE;   // set (in ABORT_RETURN only) if a test section (Static/Dynamic) was aborted,
                                                // clear only before Static test and before Dynamic tests

unsigned char gOBDTestFailed = FALSE;           // set if any test failed,
                                                // here is the only place this should be set to FALSE

unsigned char gOBDTestSectionFailed = FALSE;    // set if a test (static/dynamic) failed,
                                                // clear only before Static test and before Dynamic test

unsigned char gOBDTestSubsectionFailed = FALSE; // set if a test subsection failed,
                                                // clear before each subsection (ie test 5.10, test 7.4, etc)


unsigned char gOBDDetermined = FALSE;           // set if a OBD protocol found
unsigned long gOBDRequestDelay = 100;
unsigned long gOBDMaxResponseTimeMsecs = 100;
unsigned long gOBDMinResponseTimeMsecs = 0;     // min response time
unsigned long gOBDListIndex = 0;                // index of the OBD protocol being used
unsigned long gOBDFoundIndex = 0;               // index of OBD protocol found
PROTOCOL_LIST gOBDList[OBD_MAX_PROTOCOLS];
unsigned long gOBDNumEcus = 0;                  // number of responding ECUs
unsigned long gOBDNumEcusResp = 0;              // number of responding ECUs
unsigned char gOBDEngineRunning = FALSE;        // set if engine should be running
unsigned char gOBDEngineWarm = FALSE;           // set if engine should be warm
unsigned char gOBDDTCPending = FALSE;           // set if a DTC should be pending
unsigned char gOBDDTCStored = FALSE;            // set if a DTC should be stored
unsigned char gOBDDTCHistorical = FALSE;
unsigned char gOBDDTCPermanent = FALSE;         // set if a DTC should be permanent

unsigned char gOBDDieselFlag = FALSE;           // set by the user if this is a compression ignition (diesel) vehicle
unsigned char gOBDHybridFlag = FALSE;           // set by the user if this is a hybrid vehicle
unsigned char gOBDPlugInFlag = FALSE;
unsigned char gOBDDynoCertFlag = FALSE;         // set by the user if this is a Dyno Certified vehicle

unsigned long gOBDProtocolOrder = 0;
unsigned long gOBDMonitorCount = 0;
ECU_TIMING_DATA gEcuTimingData[OBD_MAX_ECUS];
unsigned long gLastLogTime = 0;
unsigned char gIgnoreUnsupported = FALSE;
unsigned char gSuspendScreenOutput = FALSE;
unsigned char gSuspendLogOutput = FALSE;
OBD_DATA gOBDResponse[OBD_MAX_ECUS];
OBD_DATA gOBDCompareResponse[OBD_MAX_ECUS];
char gVIN[18] = {0};
FILE *ghLogFile = NULL;
FILE *ghTempLogFile = NULL;
PASSTHRU_MSG gTesterPresentMsg = {0};

/*********************************************/
/* Option to turn off Tester Present Message */
BOOL gPeriodicMsgEnabled = TRUE;
/*********************************************/

unsigned long gOBDNumEcusCan = 0  ;               /* by Honda */
unsigned char gOBDResponseTA[OBD_MAX_ECUS] = {0}; /* by Honda */

long gSid1VariablePidSize = 0;

DTC_LIST gDTCList[OBD_MAX_ECUS];

unsigned long gulDeviceID = 0;      // global storage for J2534-1 Device ID

TEST_PHASE  TestPhase = eTestNone;
unsigned char  TestSubsection;      // test subsection to be used in conjunction with TestPhase (ie test phase 5, subsection 14 - 5.14)

char *gBanner =
"\n\n"
"  JJJJJJJJJJJ     1      6666666666   9999999999   9999999999    3333333333\n"
"       J         11      6        6   9        9   9        9             3\n"
"       J        1 1      6            9        9   9        9             3\n"
"       J          1      6            9        9   9        9             3\n"
"       J          1      6            9        9   9        9             3\n"
"       J          1      6            9        9   9        9             3\n"
"       J          1      6666666666   9999999999   9999999999 --   33333333\n"
"       J          1      6        6            9            9             3\n"
"       J          1      6        6            9            9             3\n"
"       J          1      6        6            9            9             3\n"
"       J          1      6        6            9            9             3\n"
"  J    J          1      6        6   9        9   9        9             3\n"
"  JJJJJJ       1111111   6666666666   9999999999   9999999999    3333333333\n"
"\n"
"  Copyright (C) 2002 Drew Technologies. http://j1699-3.sourceforge.net/\n"
"\n"
"  This program is free software; you can redistribute it and/or modify\n"
"  it under the terms of the GNU General Public License as published by\n"
"  the Free Software Foundation; either version 2 of the License, or\n"
"  (at your option) any later version.\n"
"\n"
"  This program is distributed in the hope that it will be useful,\n"
"  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"  GNU General Public License for more details.\n"
"\n\n";

char gszTempLogFilename[MAX_PATH] = {0};

STATUS RunStaticTests (void);
STATUS RunDynamicTests (void);

STATUS OpenTempLogFile (void);

BOOL WINAPI HandlerRoutine (DWORD dwCtrlType);

/*
********************************************************************************
** Main function
********************************************************************************
*/
int main(int argc, char **argv)
{
	unsigned long LogFileIndex;
	unsigned long EcuIndex;
	int           nTestChoice;
	int           nOBDType = 0;
	int           nISOType = 0;
	int           nEngType = 0;
	int           nPwrTrnType = 0;
	int           nVehType = 0;
	STATUS        RetCode = FAIL;
	char buf[256];
	char characterset[] = " !#$%&'()+,-.0123456789;=@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]_`abcdefghijklmnopqrstuvwxyz{}~^";

	/* setup ctrl-C handler */
	SetConsoleCtrlHandler (HandlerRoutine, TRUE);

	/* use an unbuffered stdout */
	setbuf(stdout, NULL);

	/* initialize required variables */
	memset(&gDTCList[0], 0x00, (sizeof(DTC_LIST)) * OBD_MAX_ECUS);
	memset(&gOBDResponse[0], 0x00, (sizeof(OBD_DATA)) * OBD_MAX_ECUS);
	memset(&gOBDCompareResponse[0], 0x00, (sizeof(OBD_DATA)) * OBD_MAX_ECUS);
	memset(&gOBDList[0], 0x00, (sizeof(PROTOCOL_LIST)) * OBD_MAX_PROTOCOLS);
	memset(&gEcuTimingData[0], 0x00, (sizeof(ECU_TIMING_DATA)) * OBD_MAX_ECUS);

	ClearTransactionBuffer();	/* initialize log file ring buffer for Mfg. Spec. Drive Cycle */

	gLastLogTime = GetTickCount();	/* Get the start time for the log file */

	/* test  */
	printf("what is your birth month?\n");
	fflush(stdout);

	/* Send out the banner */
	printf (gBanner);
	fflush(stdout);

	/* open temp log file */
	if (OpenTempLogFile () != PASS)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTOFF, NO_PROMPT,
		     "Cannot open temp log file\n");
		exit (FAIL);
	}

	/* Find the J2534 interface and attach to the DLL */
	/*
	if (FindJ2534Interface() != PASS)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "J2534 interface not found\n");
		exit (FAIL);
	}
	*/

	/* Open J2534 device */
	/*
	{
		unsigned long RetVal = PassThruOpen (NULL, &gulDeviceID);
		if (RetVal != STATUS_NOERROR)
		{
			Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "%s returned %ld", "PassThruOpen", RetVal);
			exit (FAIL);
		}
	}
	*/
	
	/* Get user input to determine which test to run */
	do
	{
		clear_keyboard_buffer ();
		Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTOFF, CUSTOM_PROMPT,
		     "(Q02) Select the sections of test to be run from following list:\n"
		     "    1. Run Static  Tests contained in Sections 5  through 9.\n"
		     "    2. Run Dynamic Tests contained in Sections 10 through 11.\n"
		     "Enter choice (1 or 2): ");
		fflush(stdout);
		scanf  ("%d", &nTestChoice);
		Log( BLANK, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "\n");
	} while (nTestChoice < 1 || nTestChoice > 2);


	/* Get the user input to create the log filename */


	/* Collect user input for vehicle model year */
	do
	{
		clear_keyboard_buffer ();
		Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTOFF, CUSTOM_PROMPT,
		     "(Q03) Enter model year of vehicle (%d to %d): ", MIN_MODEL_YEAR, MAX_MODEL_YEAR);
		gets (gUserModelYear);
		Log( BLANK, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "\n");
		gModelYear = atoi(gUserModelYear);
	} while ( gModelYear < MIN_MODEL_YEAR || gModelYear > MAX_MODEL_YEAR );


	/* Collect user input for static tests */
	if (nTestChoice == 1)
	{
		clear_keyboard_buffer ();

		do
		{
			Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTOFF, CUSTOM_PROMPT,
			     "(Q04) Enter make of vehicle (e.g. Dodge, Ford, GMC, Honda, Toyota...): ");
			gets ( buf );
			Log( BLANK, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "\n");
		} while ( strspn ( buf, characterset ) < strlen (buf) || // contains nonalphanumeric characters
		          (strlen (buf) + 1) > MAX_LOGFILENAME );        // longer than max filename
		strcpy (gUserMake, buf);


		do
		{
			Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTOFF, CUSTOM_PROMPT,
			     "(Q05) Enter vehicle model: ");
			gets ( buf );
			Log( BLANK, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "\n");
		} while ( strspn ( buf, characterset ) < strlen (buf) || // contains nonalphanumeric characters
		          (strlen (buf) + 1) > MAX_LOGFILENAME );        // longer than max filename
		strcpy (gUserModel, buf);
	}


	/* Ask the user for number of ECUs on the vehicle */
	while ((gUserNumEcus < 1) || (gUserNumEcus > OBD_MAX_ECUS))
	{
		clear_keyboard_buffer ();
		Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTOFF, CUSTOM_PROMPT,
		     "(Q06) %s",
		     g_rgpcDisplayStrings[DSPSTR_PRMPT_OBD_ECU]);
		scanf  ("%d", &gUserNumEcus);
		Log( BLANK, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "\n");
	}


	/* Ask the user for number of reprogamable ECUs on the vehicle */
	while ((gUserNumEcusReprgm < 1) || (gUserNumEcusReprgm > OBD_MAX_ECUS))
	{
		clear_keyboard_buffer ();
		Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTOFF, CUSTOM_PROMPT,
		     "(Q07) %s",
		     g_rgpcDisplayStrings[DSPSTR_PRMPT_RPGM_ECU]);
		scanf  ("%d", &gUserNumEcusReprgm);
		Log( BLANK, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "\n");
	}


	/* Ask the user for the type of engine in the vehicle */
	while ((nEngType < 1) || (nEngType > ENGINE_LIST_COUNT))
	{
		clear_keyboard_buffer ();
		Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTOFF, CUSTOM_PROMPT,
		     "(Q08) %s\n%s"
		     "Enter selection number (1 to %d)? ",
		     g_rgpcDisplayStrings[DSPSTR_PRMPT_ENG_TYPE],
		     gEngineListString[0],
		     ENGINE_LIST_COUNT );
		scanf  ("%d", &nEngType);
		Log( BLANK, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "\n");

		gUserInput.eEngineType = nEngType;
	}
	switch ( nEngType )
	{
		case GASOLINE:  // Spark Ignition (Gasoline)
			gOBDDieselFlag = FALSE;
			break;
		case DIESEL:     // Compression Ignition (Diesel)
			gOBDDieselFlag = TRUE;
			break;
	}


	/* Ask the user for the type of powertrain in the vehicle */
	while ((nPwrTrnType < 1) || (nPwrTrnType > PWRTRN_LIST_COUNT))
	{
		clear_keyboard_buffer ();
		Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTOFF, CUSTOM_PROMPT,
		     "(Q09) %s\n"
		     "%s"
		     "Enter selection number (1 to %d)? ",
		     g_rgpcDisplayStrings[DSPSTR_PRMPT_PWRTRN_TYPE],
		     gPwrtrnListString[0],
		     PWRTRN_LIST_COUNT );
		scanf  ("%d", &nPwrTrnType);
		Log( BLANK, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "\n");
		
		gUserInput.ePwrTrnType = nPwrTrnType;
	}
	switch ( nPwrTrnType )
	{
		case CONV:  // Conventional
			gOBDHybridFlag = FALSE;
			gOBDPlugInFlag = FALSE;
			break;
		case SS:    // Stop/Start
			gOBDHybridFlag = TRUE;
			gOBDPlugInFlag = FALSE;
			break;
		case HEV:   // Hybrid Electric Vehicle
			gOBDHybridFlag = TRUE;
			gOBDPlugInFlag = FALSE;
			break;
		case PHEV:  // Plug-In Hybrid Electric Vehicle
			gOBDHybridFlag = TRUE;
			gOBDPlugInFlag = TRUE;
			break;
	}
	printf("finished untile power train check.");

	/* Ask the user for the type of compliance test to be run */
	while ((nOBDType < 1) || (nOBDType > COMPLIANCE_TEST_LIST_COUNT))
	{
		clear_keyboard_buffer ();
		Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTOFF, CUSTOM_PROMPT,
		     "(Q10) %s\n%s"
		     "Enter selection number (1 to %d)? ",
		     g_rgpcDisplayStrings[DSPSTR_STMT_COMPLIANCE_TYPE],
		     gComplianceTestListString[0],
		     COMPLIANCE_TEST_LIST_COUNT );
		scanf  ("%d", &nOBDType);
	}
	/* build the complaince information */
	switch ( nOBDType )
	{
		case 1: /* Legacy + ISO 15765 @ 500k only */
			gUserInput.eComplianceType = US_OBDII;
			gUserInput.eScanTable = USOBD;
			strcpy(gUserInput.szNumDCToSetMIL, "two");
			break;

		case 2: /* Heavy Duty OBD */
			gUserInput.eComplianceType = HD_OBD;
			gUserInput.eScanTable = USOBD;
			strcpy(gUserInput.szNumDCToSetMIL, "two");
			break;

		case 3: /* Legacy + ISO 15765 */
			gUserInput.eComplianceType = EOBD_WITH_IUMPR;
			gUserInput.eScanTable = EOBD;
			strcpy(gUserInput.szNumDCToSetMIL, "three");
			break;

		case 4: /* Legacy + ISO 15765 */
			gUserInput.eComplianceType = EOBD_NO_IUMPR;
			gUserInput.eScanTable = EOBD;
			strcpy(gUserInput.szNumDCToSetMIL, "three");
			break;

		case 5: /* Heavy Duty EOBD */
			gUserInput.eComplianceType = HD_EOBD;
			gUserInput.eScanTable = EOBD;
			strcpy(gUserInput.szNumDCToSetMIL, "three");
			break;

		case 6: /* Light Duty IOBD */
			gUserInput.eComplianceType = IOBD_NO_IUMPR;
			gUserInput.eScanTable = EOBD;
			strcpy(gUserInput.szNumDCToSetMIL, "three");
			break;
		
		case 7: /* Heavy Duty IOBD */
			gUserInput.eComplianceType = HD_IOBD_NO_IUMPR;
			gUserInput.eScanTable = EOBD;
			strcpy(gUserInput.szNumDCToSetMIL, "three");
			break;
		
		case 8: /* Light Duty OBDBr */
			gUserInput.eComplianceType = OBDBr_NO_IUMPR;
			gUserInput.eScanTable = EOBD;
			strcpy(gUserInput.szNumDCToSetMIL, "three");
			break;

		default:
			gUserInput.eComplianceType = UNKNOWN;
			gUserInput.eScanTable = USOBD;
			strcpy(gUserInput.szNumDCToSetMIL, "two");
			break;
	}


	// if the OBD Type is one that has BUS OFF problems, select the type of ISO15765 to use
	if ( nOBDType >= 3 && nOBDType <= 8 )
	{
		/* Ask the user for the type of ISO15765 to be used */
		while ((nISOType < 1) || (nISOType > ISO15765OPTIONS))
		{
			clear_keyboard_buffer ();
			Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTOFF, CUSTOM_PROMPT,
			     "(Q14) %s\n%s"
			     "Enter selection number (1 to %d)? ",
			     g_rgpcDisplayStrings[DSPSTR_PRMPT_ISO15765_TYPE],
			     gISO15765Options[0],
			     ISO15765OPTIONS );
			scanf  ("%d", &nISOType);
		}

		/* set the scantable for the selected type of ISO15765 */
		switch ( nISOType )
		{
			case 1: /* Legacy + ISO 15765 @ 500k and 250k */
				gUserInput.eScanTable = EOBD;
				break;

			case 2: /* Legacy + ISO 15765 @ 500k only, BUS_OFF work-around */
				gUserInput.eScanTable = USOBD;
				break;

			case 3: /* Legacy + ISO 15765 @ 250k only, BUS_OFF work-around */
				gUserInput.eScanTable = EOBD_250K;
				break;
		}
	}


	// If the chosen OBD Type is for Heavy Duty, set Vehicle Type to Heavy Duty
	if ( (nOBDType == 2) || (nOBDType == 5) || (nOBDType == 7))
	{
		nVehType = VEHICLE_LIST_COUNT+1;
	}
	else
	{
		/* Ask the user for the type of vehicle */
		while ((nVehType < 1) || (nVehType > VEHICLE_LIST_COUNT))
		{
			clear_keyboard_buffer ();
			Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTOFF, CUSTOM_PROMPT,
			     "(Q11) %s\n%s"
			     "Enter selection number (1 to %d)? ",
			     g_rgpcDisplayStrings[DSPSTR_PRMPT_VEH_TYPE],
			     gVehicleListString[0],
			     VEHICLE_LIST_COUNT );
			scanf  ("%d", &nVehType);
			Log( BLANK, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "\n");
		}
	}
	switch ( nVehType )
	{
		case 1:       // Light Duty Vehicle
			gUserInput.eVehicleType = LD;
			gOBDDynoCertFlag = FALSE;
			break;
		case 2:       // Medium Duty Truck Engine Dyno Certified
			gUserInput.eVehicleType = MD;
			gOBDDynoCertFlag = TRUE;
			break;
		case (VEHICLE_LIST_COUNT+1):       // Heavy Duty Truck (Always Engine Dyno Certified)
			gUserInput.eVehicleType = HD;
			gOBDDynoCertFlag = TRUE;
			break;
	}




	/*********************************************/
	/* Option to turn off Tester Present Message */
	if ( Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_PROMPT,
	          "(Q13) Disable Tester Present Messages for ISO15765 vehicles?\n"
	          "(All other protocols will send Tester Present Messages regardless\n"
	          "of the response to this prompt)  ") == 'Y' )
	{
		gPeriodicMsgEnabled = FALSE;
	}
	/*********************************************/




	/* initialize reverse order NOT requested */
	for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
	{
		gReverseOrderState[EcuIndex] = NOT_REVERSE_ORDER;
	}

	/* Run Static test */
	if (nTestChoice == 1)
	{
		/* Create the log file name from the user input */
		ghLogFile = NULL;
		for (LogFileIndex = 1; LogFileIndex < 1000000; LogFileIndex++)
		{
			sprintf (gLogFileName, "%s-%s-%s-%ld.log", gUserModelYear, gUserMake, gUserModel, LogFileIndex);

			/* Check if log file already exists */
			if ((ghLogFile = fopen (gLogFileName, "r")) != NULL)
			{
				fclose (ghLogFile);
			}
			else
			{
				break;
			}
		}

		/* Open the log file */
		ghLogFile = fopen (gLogFileName, "w+");
		if (ghLogFile == NULL)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTOFF, NO_PROMPT,
			     "Cannot open log file %s\n", gLogFileName);
			StopTest(FAIL, TestPhase);
			exit (FAIL);
		}

		/* Application version, build date, OS, etc */
		LogVersionInformation (SCREENOUTPUTON, LOGOUTPUTON);

		/* Copy temp log file to actual log file */
		if (AppendLogFile () != PASS)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTOFF, NO_PROMPT,
			     "Unable to copy temp log file to %s\n", gLogFileName);
			StopTest(FAIL, TestPhase);
			exit (FAIL);
		}

		/* Echo user responses to the log file */
		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "%s%d\n", g_rgpcDisplayStrings[DSPSTR_PRMPT_MODEL_YEAR], gModelYear);

		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "%s%d\n", g_rgpcDisplayStrings[DSPSTR_PRMPT_OBD_ECU], gUserNumEcus);

		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "%s%d\n", g_rgpcDisplayStrings[DSPSTR_PRMPT_RPGM_ECU], gUserNumEcusReprgm);

		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "%s%s\n", g_rgpcDisplayStrings[DSPSTR_PRMPT_ENG_TYPE], gEngineTypeStrings[gUserInput.eEngineType] );

		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "%s%s\n", g_rgpcDisplayStrings[DSPSTR_PRMPT_PWRTRN_TYPE], gPwrTrnTypeStrings[gUserInput.ePwrTrnType] );

		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "%s%s\n", g_rgpcDisplayStrings[DSPSTR_PRMPT_VEH_TYPE], gVehicleTypeStrings[gUserInput.eVehicleType] );

		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "%s%s\n", g_rgpcDisplayStrings[DSPSTR_STMT_COMPLIANCE_TYPE],
		     gComplianceTestTypeStrings[gUserInput.eComplianceType]);

		/* Ask for optional user information */
		Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
		     "(Q12) Enter your name and/or contact information (optional) " );


		/* Run Static test */
		RetCode = RunStaticTests ();
		if ( RetCode == FAIL )
		{
			Log( SECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "Static");
		}
		else
		{
			Log( SECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "Static");
		}

		// cannot continue to dynamic tests if DetermineProtocol failed
		if ( gOBDDetermined == FALSE )
		{
			Log( BLANK, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "\n\n");
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Protocol determination unsuccessful. Unable to continue to Dynamic Tests.\n\n");
		}
		/* Prompt user to run dynamic test */
		else if (
		          Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_PROMPT,
		               "Would you like to run dynamic tests?") == 'Y')
		{
			nTestChoice = 2;
			LogStats ();
			fclose (ghLogFile);
			ghLogFile = ghTempLogFile;
			strcpy (gLogFileName, gszTempLogFilename);
		}

	}

	/* Run Dynamic test */
	if (nTestChoice == 2)
	{
		/* run dynamic tests */
		RetCode = RunDynamicTests ();
		if ( RetCode == FAIL )
		{
			Log( SECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "Dynamic");
		}
		else if ( RetCode == ABORT )
		{
			Log( SECTION_INCOMPLETE_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "Dynamic");
		}
		else if ( RetCode == EXIT )
		{
			/* do nothing */
		}
		else
		{
			Log( SECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "Dynamic");
		}
	}

	/* Done - passed */
	LogStats();
	StopTest (RetCode, TestPhase);

	return RetCode;//
}

/*
********************************************************************************
** RunStaticTests function
********************************************************************************
*/
STATUS RunStaticTests (void)
{
	// Clear Test Failure Flag
	gOBDTestSectionFailed = FALSE;

	// Clear Test Aborted Flag
	gOBDTestSectionAborted = FALSE;

	/* Reset vehicle state */
	gOBDEngineRunning = FALSE;
	gOBDDTCPending    = FALSE;
	gOBDDTCStored     = FALSE;
	gOBDDTCHistorical = FALSE;
	gOBDProtocolOrder = 0;

	/* Run tests 5.XX */
	TestPhase = eTestNoDTC;
	if (TestWithNoDtc() != PASS)
	{
		return FAIL;
	}

	/*
	** Sleep 5 seconds between each test to "drop out" of diagnostic session
	** so we can see if a different OBD protocol is found on the next search
	*/
	Sleep (5000);

	/* Run tests 6.XX */
	TestPhase = eTestPendingDTC;
	if (TestWithPendingDtc() != PASS)
	{
		return FAIL;
	}

	/*
	** Sleep 5 seconds between each test to "drop out" of diagnostic session
	** so we can see if a different OBD protocol is found on the next search
	*/
	Sleep (5000);

	/* Run tests 7.XX */
	TestPhase = eTestConfirmedDTC;
	if (TestWithConfirmedDtc() != PASS)
	{
		return FAIL;
	}

	/*
	** Sleep 5 seconds between each test to "drop out" of diagnostic session
	** so we can see if a different OBD protocol is found on the next search
	*/
	Sleep (5000);

	/* Run tests 8.XX */
	TestPhase = eTestFaultRepaired;
	if (TestWithFaultRepaired() != PASS)
	{
		return FAIL;
	}

	/*
	** Sleep 5 seconds between each test to "drop out" of diagnostic session
	** so we can see if a different OBD protocol is found on the next search
	*/
	Sleep (5000);

	/* Run tests 9.XX */
	TestPhase = eTestNoFault3DriveCycle;
	if (TestWithNoFaultsAfter3DriveCycles() != PASS)
	{
		return FAIL;
	}

	if ( gOBDTestSectionFailed == TRUE )
	{
		return FAIL;
	}

	return PASS;
}

/*
********************************************************************************
** RunDynamicTests function
********************************************************************************
*/
STATUS RunDynamicTests (void)
{
	STATUS RetCode;

	unsigned char gOBDTestSectionFailed_Copy;   // copy of gOBDTestSectionFailed to determine if either dynamic test failed
	unsigned char gOBDTestSectionAborted_Copy;  // copy of gOBDTestSectionAborted to determine if either dynamic test aborted
	BOOL bReEnterTest = FALSE;                  // set to TRUE if test in being re-entered

	// Clear Test Failure Flag
	gOBDTestSectionFailed = FALSE;

	// Clear Test Aborted Flag
	gOBDTestSectionAborted = FALSE;

	// Run tests 10.XX
	TestPhase = eTestInUseCounters;
	RetCode = TestToVerifyInUseCounters(&bReEnterTest);
	if ( (RetCode == FAIL) || (RetCode == EXIT) )
	{
		return(RetCode);
	}

	// Save copy of Test Failure Flag
	gOBDTestSectionFailed_Copy = gOBDTestSectionFailed;
	// Clear Test Failure Flag
	gOBDTestSectionFailed = FALSE;

	// Save copy of Test Aborted Flag
	gOBDTestSectionAborted_Copy = gOBDTestSectionAborted;
	// Clear Test Aborted Flag
	gOBDTestSectionAborted = FALSE;

	// Proceed to tests 11.XX
	TestPhase = eTestPerformanceCounters;
	RetCode = TestToVerifyPerformanceCounters(&bReEnterTest);
	if ( RetCode == FAIL || gOBDTestSectionFailed == TRUE || gOBDTestSectionFailed_Copy == TRUE)
	{
		return FAIL;
	}
	else if (RetCode == ABORT || gOBDTestSectionAborted == TRUE || gOBDTestSectionAborted_Copy == TRUE )
	{
		return ABORT;
	}

	return PASS;
}

/*
********************************************************************************
** OpenTempLogFile
********************************************************************************
*/
STATUS OpenTempLogFile (void)
{
	/* Get temp log filename */
	if (GetTempFileName (".", "j1699", 0, gszTempLogFilename) == 0)
		return FAIL;

	/* Open the log file */
	ghTempLogFile = ghLogFile = fopen (gszTempLogFilename, "w+");
	if (ghTempLogFile == NULL)
	{
		return FAIL;
	}

	return PASS;
}

/*
********************************************************************************
** AppendLogFile
********************************************************************************
*/
STATUS AppendLogFile (void)
{
	char buf[1024];

	/* move to beginning of temp log file */
	fflush (ghTempLogFile);
	fseek (ghTempLogFile, 0, SEEK_SET);

	/* move to end of log file */
	fseek (ghLogFile, 0, SEEK_END);

	/* append temp log file to official log file */
	while (fgets (buf, sizeof(buf), ghTempLogFile) != NULL)
		fputs (buf, ghLogFile);

	return PASS;
}


/*
********************************************************************************
** HandlerRoutine
********************************************************************************
*/
BOOL WINAPI HandlerRoutine (DWORD dwCtrlType)
{

	 // ctrl-C or other system events which terminate the test
	Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "Ctrl-C :: terminating application\n\n");

	gOBDTestAborted = TRUE;
	LogStats();
	StopTest (CTRL_C, TestPhase);

	exit (CTRL_C);
}


/*
********************************************************************************
** AbortTest
********************************************************************************
*/
void AbortTest (void)
{

	// terminate the test
	Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "User Aborted :: terminating application\n\n");

	gOBDTestAborted = TRUE;
	LogStats();
	StopTest (ABORT, TestPhase);

	exit (ABORT);
}



/*
********************************************************************************
** LogSoftwareVersion - Log the J1699-3 software version information
********************************************************************************
*/
void LogSoftwareVersion
	(
	  SCREENOUTPUT bDisplay,
	  LOGOUTPUT bLog
	)
{

#ifdef _DEBUG
	Log( INFORMATION, bDisplay, bLog, NO_PROMPT,
	     "**** SAE %s DEBUG Build Revision %s  (Build Date: %s) ****\n\n", szJ1699_VER, gszAPP_REVISION, szBUILD_DATE);
#else
	Log( INFORMATION, bDisplay, bLog, NO_PROMPT,
	     "**** SAE %s Build Revision %s  (Build Date: %s) ****\n\n", szJ1699_VER, gszAPP_REVISION, szBUILD_DATE);
#endif

}


/*
********************************************************************************
** LogVersionInformation
********************************************************************************
*/
void LogVersionInformation
	(
	  SCREENOUTPUT bDisplay,
	  LOGOUTPUT bLog
	)
{
	time_t current_time;
	struct tm *current_tm;
	unsigned long version;

	/* Put the date / time in the log file */
	time (&current_time);
	current_tm = localtime (&current_time);
	Log( INFORMATION, bDisplay, LOGOUTPUTON, NO_PROMPT,
	     "%s\n\n", asctime (current_tm));

	/* Let the user know what version is being run and the log file name */
	Log( INFORMATION, bDisplay, bLog, NO_PROMPT,
	     "**** LOG FILENAME %s ****\n", gLogFileName);

	LogSoftwareVersion( bDisplay, bLog );

	Log( INFORMATION, bDisplay, bLog, NO_PROMPT,
	     "**** NOTE: Timestamp on left is from the PC ****\n");
	Log( INFORMATION, bDisplay, bLog, NO_PROMPT,
	     "**** NOTE: Timestamp with messages is from the J2534 interface ****\n\n");

	/* Log Microsoft Windows OS */
	version = GetVersion();
	if (version & 0x80000000)
	{
		Log( WARNING, bDisplay, bLog, NO_PROMPT,
		     "Windows 9X/ME (%08X)\n", version);
	}
	else
	{
		Log( INFORMATION, bDisplay, bLog, NO_PROMPT,
		     "Windows NT/2K/XP (%08X)\n", version);
	}
}


/*
*******************************************************************************
** substring
**
** Returns: a pointer to the location of 'substr' in 'str'
*******************************************************************************
*/
char * substring (char * str, const char * substr)
{
	int tok_len = strlen (substr);
	int n       = strlen (str) - tok_len;
	int i;

	for (i=0; i<=n; i++)
	{
		if (str[i] == substr[0])
			if (strncmp (&str[i], substr, tok_len) == 0)
				return &str[i];
	}

	return 0;
}


/*
*******************************************************************************
** clear_keyboard_buffer - remove all pending data from input buffer
*******************************************************************************
*/
void clear_keyboard_buffer (void)
{
	while (_kbhit()) {_getch();}
	fflush(stdin);
}