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
** 04/26/04      Added comments: Corrections required for in-use performace
**               counter testing.
** 05/01/04      Renumber all test cases to reflect specification.  This section
**               has been indicated as section 11 in Draft 15.4.
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

extern STATUS VerifyInf14Data (unsigned long EcuIndex);
STATUS EvaluateInf14 ( void );


enum IM_Status {Complete=0, Incomplete, NotSupported, Disabled, Invalid};

enum IM_Status Test11IMStatus[OBD_MAX_ECUS][11];
enum IM_Status CurrentIMStatus[11];

SID1    Test11_Sid1Pid1[OBD_MAX_ECUS];

SID9IPT Test10_10_Sid9Ipt[OBD_MAX_ECUS];
SID9IPT Test11_5_Sid9Ipt[OBD_MAX_ECUS];
SID9IPT Test11_11_Sid9Ipt[OBD_MAX_ECUS];
SID9IPT Test11CurrentDisplayData[OBD_MAX_ECUS];

SID1    PreviousSid1Pid1[OBD_MAX_ECUS];
SID1    Sid1Pid41[OBD_MAX_ECUS];   // capture the response from SID01 PID41 response.


typedef struct
{
	BOOL bCatBank1;
	BOOL bCatBank2;
	BOOL bO2Bank1;
	BOOL bO2Bank2;
	BOOL bSO2Bank1;
	BOOL bSO2Bank2;
	BOOL bNCAT;
	BOOL bNADS;
	BOOL bAFRIBank1;
	BOOL bAFRIBank2;
	BOOL bPFBank1;
	BOOL bPFBank2;
} BANK_SUPPORT;
static BANK_SUPPORT BankSupport[OBD_MAX_ECUS];


const char * szIM_Status[] = {"Complete", "Incomplete", "Not Supported", "Disabled", "Invalid"};
const char * szTest_Status[] = {"Normal", "FAILURE"};

BOOL IsIM_ReadinessComplete (SID1 * pSid1);
BOOL SaveSid9IptData (unsigned int EcuIndex, SID9IPT * pSid9Ipt);
STATUS EvaluateSid9Ipt (int EcuIndex, unsigned int *pDoneFlags, int test_stage, BOOL bDisplayErrorMsg);
STATUS RunDynamicTest11 (BOOL bDisplayCARBTimers, unsigned long ulEngineStartTimeStamp);
void PrintEcuData ( unsigned int  IMReadyDoneFlags, unsigned int  Sid9IptDoneFlags,
                    unsigned int  EcuDone, unsigned int  bSid9Ipt );

//-----------------------------------------------------------------------------
// Dynamic Test 11.x screen
//-----------------------------------------------------------------------------
#define COL1            1
#define COL2            25
#define COL3            40
#define COL4            60
#define COL5            65
#define COL6            70
#define COL7            75

#define COL_ECU         (COL1+12)
#define SPACE           9

#define STATUS_ROW      5
#define PRESS_ESC_ROW   26

#define STRING_WIDTH    13
#define NUMERIC_WIDTH   5
#define HEX_WIDTH       4
#define ECU_WIDTH       8


StaticTextElement  _string_elements11_INF856[] =
{
	// ECU List
	{"ECU List:",   COL1, 2},
	{"ECU Status:", COL1, 3},
	{"1", COL_ECU + 0 * SPACE, 1},
	{"2", COL_ECU + 1 * SPACE, 1},
	{"3", COL_ECU + 2 * SPACE, 1},
	{"4", COL_ECU + 3 * SPACE, 1},
	{"5", COL_ECU + 4 * SPACE, 1},
	{"6", COL_ECU + 5 * SPACE, 1},
	{"7", COL_ECU + 6 * SPACE, 1},
	{"8", COL_ECU + 7 * SPACE, 1},

	// column 1
	{"I/M STATUS",                COL1, STATUS_ROW+0},
	{"Oxygen Sensor Monitor",     COL1, STATUS_ROW+1},
	{"Oxygen Sensor Heater",      COL1, STATUS_ROW+2},
	{"Catalyst Monitor",          COL1, STATUS_ROW+3},
	{"Catalyst Heater",           COL1, STATUS_ROW+4},
	{"Reserved",                  COL1, STATUS_ROW+5},
	{"Evaporative Emissions",     COL1, STATUS_ROW+6},
	{"EGR/VVT",                   COL1, STATUS_ROW+7},
	{"AIR",                       COL1, STATUS_ROW+8},
	{"Fuel Trim",                 COL1, STATUS_ROW+9},
	{"Misfire",                   COL1, STATUS_ROW+10},
	{"Comp / Comp",               COL1, STATUS_ROW+11},

	// column 2
	{"RATE BASED COUNTER",        COL3, STATUS_ROW+0},
	{"OBD Monitoring Conditions", COL3, STATUS_ROW+1},
	{"Ignition Counter",          COL3, STATUS_ROW+2},

	{"Initial",                   COL4, STATUS_ROW+4},
	{"Current",                   COL6, STATUS_ROW+4},

	{"N",                         COL4, STATUS_ROW+5},
	{"D",                         COL5, STATUS_ROW+5},

	{"N",                         COL6, STATUS_ROW+5},
	{"D",                         COL7, STATUS_ROW+5},

	// column 2 - two values
	{"Catalyst B1",               COL3, STATUS_ROW+6},
	{"Catalyst B2",               COL3, STATUS_ROW+7},
	{"Oxygen Sensor B1",          COL3, STATUS_ROW+8},
	{"Oxygen Sensor B2",          COL3, STATUS_ROW+9},
	{"EGR/VVT",                   COL3, STATUS_ROW+10},
	{"AIR",                       COL3, STATUS_ROW+11},
	{"EVAP",                      COL3, STATUS_ROW+12},
	{"Sec O2 Sensor B1",          COL3, STATUS_ROW+13},
	{"Sec O2 Sensor B2",          COL3, STATUS_ROW+14},
	{"AFRI Sensor B1",            COL3, STATUS_ROW+15},
	{"AFRI Sensor B2",            COL3, STATUS_ROW+16},
	{"PF Sensor B1",              COL3, STATUS_ROW+17},
	{"PF Sensor B2",              COL3, STATUS_ROW+18},

	// misc
	{"Test Status:",              COL1, PRESS_ESC_ROW-1},
	{"Press ESC to exit, a number to change ECU display, or F to FAIL", COL1, PRESS_ESC_ROW},
	{"", 0, PRESS_ESC_ROW+1}
};

const int _num_string_elements11_INF856 = sizeof(_string_elements11_INF856)/sizeof(_string_elements11_INF856[0]);


StaticTextElement  _string_elements11_INFB36[] =
{
	// ECU List
	{"ECU List:",   COL1, 2},
	{"ECU Status:", COL1, 3},
	{"1", COL_ECU + 0 * SPACE, 1},
	{"2", COL_ECU + 1 * SPACE, 1},
	{"3", COL_ECU + 2 * SPACE, 1},
	{"4", COL_ECU + 3 * SPACE, 1},
	{"5", COL_ECU + 4 * SPACE, 1},
	{"6", COL_ECU + 5 * SPACE, 1},
	{"7", COL_ECU + 6 * SPACE, 1},
	{"8", COL_ECU + 7 * SPACE, 1},

	// column 1
	{"I/M STATUS",                COL1, STATUS_ROW+0},
	{"EGS",                       COL1, STATUS_ROW+1},
	{"PM",                        COL1, STATUS_ROW+2},
	{"NMHC Catalyst",             COL1, STATUS_ROW+3},
	{"NOx Aftertreatment",        COL1, STATUS_ROW+4},
	{"Reserved",                  COL1, STATUS_ROW+5},
	{"Reserved",                  COL1, STATUS_ROW+6},
	{"EGR/VVT",                   COL1, STATUS_ROW+7},
	{"BP",                        COL1, STATUS_ROW+8},
	{"Fuel Trim",                 COL1, STATUS_ROW+9},
	{"Misfire",                   COL1, STATUS_ROW+10},
	{"Comp / Comp",               COL1, STATUS_ROW+11},

	// column 2
	{"RATE BASED COUNTER",        COL3, STATUS_ROW+0},
	{"OBD Monitoring Conditions", COL3, STATUS_ROW+1},
	{"Ignition Counter",          COL3, STATUS_ROW+2},

	{"Initial",                   COL4, STATUS_ROW+4},
	{"Current",                   COL6, STATUS_ROW+4},

	{"N",                         COL4, STATUS_ROW+5},
	{"D",                         COL5, STATUS_ROW+5},

	{"N",                         COL6, STATUS_ROW+5},
	{"D",                         COL7, STATUS_ROW+5},

	// column 2 - two values
	{"HCCAT",                     COL3, STATUS_ROW+6},
	{"NCAT",                      COL3, STATUS_ROW+7},
	{"NADS",                      COL3, STATUS_ROW+8},
	{"PM",                        COL3, STATUS_ROW+9},
	{"EGS",                       COL3, STATUS_ROW+10},
	{"EGR/VVT",                   COL3, STATUS_ROW+11},
	{"BP",                        COL3, STATUS_ROW+12},
	{"FUEL",                      COL3, STATUS_ROW+13},
	{"                ",          COL3, STATUS_ROW+14},

	// misc
	{"Test Status:",              COL1, PRESS_ESC_ROW-1},
	{"Press ESC to exit, a number to change ECU display, or F to FAIL", COL1, PRESS_ESC_ROW},
	{"", 0, PRESS_ESC_ROW+1}
};

const int _num_string_elements11_INFB36 = sizeof(_string_elements11_INFB36)/sizeof(_string_elements11_INFB36[0]);

StaticTextElement  _string_elements_timers[] =
{
	{"Cont. Idle (Sec.):",         COL1, PRESS_ESC_ROW-2},
	{"At Speed (Sec.):",             30, PRESS_ESC_ROW-2},
	{"Run Time (Sec.):",             56, PRESS_ESC_ROW-2}
};

const int _num_string_elements_timers = sizeof(_string_elements_timers)/sizeof(_string_elements_timers[0]);


DynamicValueElement  _dynamic_elements11[] =
{
	// ECUs
	{COL_ECU + 0 * SPACE, 2, ECU_WIDTH}, // ECU 1
	{COL_ECU + 1 * SPACE, 2, ECU_WIDTH}, // ECU 2
	{COL_ECU + 2 * SPACE, 2, ECU_WIDTH}, // ECU 3
	{COL_ECU + 3 * SPACE, 2, ECU_WIDTH}, // ECU 4
	{COL_ECU + 4 * SPACE, 2, ECU_WIDTH}, // ECU 5
	{COL_ECU + 5 * SPACE, 2, ECU_WIDTH}, // ECU 6
	{COL_ECU + 6 * SPACE, 2, ECU_WIDTH}, // ECU 7
	{COL_ECU + 7 * SPACE, 2, ECU_WIDTH}, // ECU 8

	// ECU Done
	{COL_ECU + 0 * SPACE, 3, HEX_WIDTH}, // ECU Status 1
	{COL_ECU + 1 * SPACE, 3, HEX_WIDTH}, // ECU Status 2
	{COL_ECU + 2 * SPACE, 3, HEX_WIDTH}, // ECU Status 3
	{COL_ECU + 3 * SPACE, 3, HEX_WIDTH}, // ECU Status 4
	{COL_ECU + 4 * SPACE, 3, HEX_WIDTH}, // ECU Status 5
	{COL_ECU + 5 * SPACE, 3, HEX_WIDTH}, // ECU Status 6
	{COL_ECU + 6 * SPACE, 3, HEX_WIDTH}, // ECU Status 7
	{COL_ECU + 7 * SPACE, 3, HEX_WIDTH}, // ECU Status 8

	// column 1                             INF 8                 / INF B
	{COL2, STATUS_ROW+1,  STRING_WIDTH}, // Oxygen Sensor Monitor / EGS
	{COL2, STATUS_ROW+2,  STRING_WIDTH}, // Oxygen Sensor Heater  / PM
	{COL2, STATUS_ROW+3,  STRING_WIDTH}, // Catalyst Monitor      / NMHC
	{COL2, STATUS_ROW+4,  STRING_WIDTH}, // Catalyst Heater       / NOx
	{COL2, STATUS_ROW+5,  STRING_WIDTH}, // Reserved              / Reserved
	{COL2, STATUS_ROW+6,  STRING_WIDTH}, // Evaportive Emissions  / Reserved
	{COL2, STATUS_ROW+7,  STRING_WIDTH}, // EGR/VVT               / EGR/VVT
	{COL2, STATUS_ROW+8,  STRING_WIDTH}, // AIR                   / BP
	{COL2, STATUS_ROW+9,  STRING_WIDTH}, // Fuel Trim
	{COL2, STATUS_ROW+10, STRING_WIDTH}, // Misfire
	{COL2, STATUS_ROW+11, STRING_WIDTH}, // Comp / Comp

	// column 2
	{COL6, STATUS_ROW+1, NUMERIC_WIDTH}, // OBD Monitoring Conditions
	{COL6, STATUS_ROW+2, NUMERIC_WIDTH}, // Ignition Counter

	// column 2 - initial values               INF 8              / INF B
	{COL4, STATUS_ROW+6, NUMERIC_WIDTH},    // Catalyst B1 N      / HCCAT N
	{COL5, STATUS_ROW+6, NUMERIC_WIDTH},    // Catalyst B1 D      / HCCAT D

	{COL4, STATUS_ROW+7, NUMERIC_WIDTH},    // Catalyst B2 N      / NCAT N
	{COL5, STATUS_ROW+7, NUMERIC_WIDTH},    // Catalyst B2 D      / NCAT D

	{COL4, STATUS_ROW+8, NUMERIC_WIDTH},    // Oxygen Sensor B1 N / NADS N
	{COL5, STATUS_ROW+8, NUMERIC_WIDTH},    // Oxygen Sensor B1 D / NADS N

	{COL4, STATUS_ROW+9, NUMERIC_WIDTH},    // Oxygen Sensor B2 N / PM N
	{COL5, STATUS_ROW+9, NUMERIC_WIDTH},    // Oxygen Sensor B2 D / PM D

	{COL4, STATUS_ROW+10, NUMERIC_WIDTH},   // EGR/VVT N          / EGS N
	{COL5, STATUS_ROW+10, NUMERIC_WIDTH},   // EGR/VVT D          / EGS D

	{COL4, STATUS_ROW+11, NUMERIC_WIDTH},   // AIR N              / EGR N
	{COL5, STATUS_ROW+11, NUMERIC_WIDTH},   // AIR D              / EGR D

	{COL4, STATUS_ROW+12, NUMERIC_WIDTH},   // EVAP N             / BP N
	{COL5, STATUS_ROW+12, NUMERIC_WIDTH},   // EVAP D             / BP D

	{COL4, STATUS_ROW+13, NUMERIC_WIDTH},   // Sec O2 Sensor B1 N / FUEL N
	{COL5, STATUS_ROW+13, NUMERIC_WIDTH},   // Sec O2 Sensor B1 D / FUEL D

	{COL4, STATUS_ROW+14, NUMERIC_WIDTH},   // Sec O2 Sensor B2 N
	{COL5, STATUS_ROW+14, NUMERIC_WIDTH},   // Sec O2 Sensor B2 D

	{COL4, STATUS_ROW+15, NUMERIC_WIDTH},   // AFRI Sensor B1 N
	{COL5, STATUS_ROW+15, NUMERIC_WIDTH},   // AFRI Sensor B1 D

	{COL4, STATUS_ROW+16, NUMERIC_WIDTH},   // AFRI Sensor B2 N
	{COL5, STATUS_ROW+16, NUMERIC_WIDTH},   // AFRI Sensor B2 D

	{COL4, STATUS_ROW+17, NUMERIC_WIDTH},   // PF Sensor B1 N
	{COL5, STATUS_ROW+17, NUMERIC_WIDTH},   // PF Sensor B1 D

	{COL4, STATUS_ROW+18, NUMERIC_WIDTH},   // PF Sensor B2 N
	{COL5, STATUS_ROW+18, NUMERIC_WIDTH},   // PF Sensor B2 D

	// column 2 - current values               INF 8              / INF B
	{COL6, STATUS_ROW+6, NUMERIC_WIDTH},    // Catalyst B1 N      / HCCAT N
	{COL7, STATUS_ROW+6, NUMERIC_WIDTH},    // Catalyst B1 D      / HCCAT D

	{COL6, STATUS_ROW+7, NUMERIC_WIDTH},    // Catalyst B2 N      / NCAT N
	{COL7, STATUS_ROW+7, NUMERIC_WIDTH},    // Catalyst B2 D      / NCAT D

	{COL6, STATUS_ROW+8, NUMERIC_WIDTH},    // Oxygen Sensor B1 N / NADS N
	{COL7, STATUS_ROW+8, NUMERIC_WIDTH},    // Oxygen Sensor B1 D / NADS N

	{COL6, STATUS_ROW+9, NUMERIC_WIDTH},    // Oxygen Sensor B2 N / PM N
	{COL7, STATUS_ROW+9, NUMERIC_WIDTH},    // Oxygen Sensor B2 D / PM D

	{COL6, STATUS_ROW+10, NUMERIC_WIDTH},   // EGR/VVT N          / EGS N
	{COL7, STATUS_ROW+10, NUMERIC_WIDTH},   // EGR/VVT D          / EGS D

	{COL6, STATUS_ROW+11, NUMERIC_WIDTH},   // AIR N              / EGR N
	{COL7, STATUS_ROW+11, NUMERIC_WIDTH},   // AIR D              / EGR D

	{COL6, STATUS_ROW+12, NUMERIC_WIDTH},   // EVAP N             / BP N
	{COL7, STATUS_ROW+12, NUMERIC_WIDTH},   // EVAP D             / BP D

	{COL6, STATUS_ROW+13, NUMERIC_WIDTH},   // Sec O2 Sensor B1 N / FUEL N
	{COL7, STATUS_ROW+13, NUMERIC_WIDTH},   // Sec O2 Sensor B1 D / FUEL D

	{COL6, STATUS_ROW+14, NUMERIC_WIDTH},   // Sec O2 Sensor B2 N
	{COL7, STATUS_ROW+14, NUMERIC_WIDTH},   // Sec O2 Sensor B2 D

	{COL6, STATUS_ROW+15, NUMERIC_WIDTH},   // AFRI Sensor B1 N
	{COL7, STATUS_ROW+15, NUMERIC_WIDTH},   // AFRI Sensor B1 D

	{COL6, STATUS_ROW+16, NUMERIC_WIDTH},   // AFRI Sensor B2 N
	{COL7, STATUS_ROW+16, NUMERIC_WIDTH},   // AFRI Sensor B2 D

	{COL6, STATUS_ROW+17, NUMERIC_WIDTH},   // PF Sensor B1 N
	{COL7, STATUS_ROW+17, NUMERIC_WIDTH},   // PF Sensor B1 D

	{COL6, STATUS_ROW+18, NUMERIC_WIDTH},   // PF Sensor B2 N
	{COL7, STATUS_ROW+18, NUMERIC_WIDTH},   // PF Sensor B2 D


	{20, PRESS_ESC_ROW-2, NUMERIC_WIDTH},   // CARB Idle Timer
	{47, PRESS_ESC_ROW-2, NUMERIC_WIDTH},   // CARB At Speed Timer
	{73, PRESS_ESC_ROW-2, NUMERIC_WIDTH},   // CARB Run Time


	{14, PRESS_ESC_ROW-1, STRING_WIDTH},    // Test Status
	{COL3, PRESS_ESC_ROW-1, STRING_WIDTH}   // DTC Status
};

const int _num_dynamic_elements11 = sizeof(_dynamic_elements11)/sizeof(_dynamic_elements11[0]);

#define ECU_ID_INDEX                0
#define ECU_STATUS_INDEX            8

#define IM_READINESS_INDEX          16
// O2_SENSOR_RESP_INDEX             16
// O2_SENSOR_HEATER_INDEX           17
// CATALYST_MONITOR_INDEX           18
// CATALYST_HEATER_INDEX            19
// AIR_COND_INDEX                   20
// EVAP_EMMISION_INDEX              21
// EGR_INDEX                        22
// AIR_INDEX                        23
// FUEL_TRIM_INDEX                  24
// MISFIRE_INDEX                    25
// COMP_COMP_INDEX                  26

#define OBD_MONITOR_COND_INDEX      27
#define IGNITION_COUNTER_INDEX      28

#define IPT_INIT_INDEX              29
// CATCOMP1   /  HCCATCOMP          29
// CATCOND1   /  HCCATCOND
// CATCOMP2   /  NCATCOMP           31
// CATCOND2   /  NCATCOND
// O2SCOMP1   /  NADSCOMP           33
// O2SCOND1   /  NADSCOND
// O2SCOMP2   /  PMCOMP             35
// O2SCOND2   /  PMCOND
// EGRCOMP    /  EGSCOMP            37
// EGRCOND    /  EGSCOND
// AIRCOMP    /  EGRCOMP            39
// AIRCOND    /  EGRCOND
// EVAPCOMP   /  BPCOMP             41
// EVAPCOND   /  BPCOND
// SO2SCOMP1  /  FUEL               43
// SO2SCOND1  /  FUEL
// SO2SCOMP2                        45
// SO2SCOND2
// AFRICOMP1                        47
// AFRICOND1
// AFRICOMP2                        49
// AFRICOND2
// PFCOMP1                          51
// PFCOND1
// PFCOMP2                          53
// PFCOND2

#define IPT_CUR_INDEX               55
// CATCOMP1   /  HCCATCOMP          55
// CATCOND1   /  HCCATCOND
// CATCOMP2   /  NCATCOMP           57
// CATCOND2   /  NCATCOND
// O2SCOMP1   /  NADSCOMP           59
// O2SCOND1   /  NADSCOND
// O2SCOMP2   /  PMCOMP             61
// O2SCOND2   /  PMCOND
// EGRCOMP    /  EGSCOMP            63
// EGRCOND    /  EGSCOND
// AIRCOMP    /  EGRCOMP            65
// AIRCOND    /  EGRCOND
// EVAPCOMP   /  BPCOMP             67
// EVAPCOND   /  BPCOND
// SO2SCOMP1  /  FUEL               69
// SO2SCOND1  /  FUEL
// SO2SCOMP2                        71
// SO2SCOND2
// AFRICOMP1                        73
// AFRICOND1
// AFRICOMP2                        75
// AFRICOND2
// PFCOMP1                          77
// PFCOND1
// PFCOMP2                          79
// PFCOND2

#define CARB_TIMER_INDEX            81
#define IDLE_TIMER                  81
#define SPEED_25_MPH_TIMER          82
#define TOTAL_DRIVE_TIMER           83

#define IPT_TESTSTATUS_INDEX        84
// Normal / FAILURE                 84
// blank / DTC Detected             85


#define OBDCOND_NOT_DONE    0xFFFFFFFE
#define IGNCNTR_NOT_DONE    0xFFFFFFFD

#define CATCOMP1_NOT_DONE   0xFFFFFFFB
#define CATCOND1_NOT_DONE   0xFFFFFFF7
#define CATCOMP2_NOT_DONE   0xFFFFFFEF
#define CATCOND2_NOT_DONE   0xFFFFFFDF
#define O2SCOMP1_NOT_DONE   0xFFFFFFBF
#define O2SCOND1_NOT_DONE   0xFFFFFF7F
#define O2SCOMP2_NOT_DONE   0xFFFFFEFF
#define O2SCOND2_NOT_DONE   0xFFFFFDFF
#define EGRCOMP8_NOT_DONE   0xFFFFFBFF
#define EGRCOND8_NOT_DONE   0xFFFFF7FF
#define AIRCOMP_NOT_DONE    0xFFFFEFFF
#define AIRCOND_NOT_DONE    0xFFFFDFFF
#define EVAPCOMP_NOT_DONE   0xFFFFBFFF
#define EVAPCOND_NOT_DONE   0xFFFF7FFF
#define SO2SCOMP1_NOT_DONE  0xFFFEFFFF
#define SO2SCOND1_NOT_DONE  0xFFFDFFFF
#define SO2SCOMP2_NOT_DONE  0xFFFBFFFF
#define SO2SCOND2_NOT_DONE  0xFFF7FFFF
#define AFRICOMP1_NOT_DONE  0xFFEFFFFF
#define AFRICOND1_NOT_DONE  0xFFDFFFFF
#define AFRICOMP2_NOT_DONE  0xFFBFFFFF
#define AFRICOND2_NOT_DONE  0xFF7FFFFF
#define PFCOMP1_NOT_DONE    0xFEFFFFFF
#define PFCOND1_NOT_DONE    0xFDFFFFFF
#define PFCOMP2_NOT_DONE    0xFBFFFFFF
#define PFCOND2_NOT_DONE    0xF7FFFFFF

#define HCCATCOMP_NOT_DONE  0xFFFFFFFB
#define HCCATCOND_NOT_DONE  0xFFFFFFF7
#define NCATCOMP_NOT_DONE   0xFFFFFFEF
#define NCATCOND_NOT_DONE   0xFFFFFFDF
#define NADSCOMP_NOT_DONE   0xFFFFFFBF
#define NADSCOND_NOT_DONE   0xFFFFFF7F
#define PMCOMP_NOT_DONE     0xFFFFFEFF
#define PMCOND_NOT_DONE     0xFFFFFDFF
#define EGSCOMP_NOT_DONE    0xFFFFFBFF
#define EGSCOND_NOT_DONE    0xFFFFF7FF
#define EGRCOMPB_NOT_DONE   0xFFFFEFFF
#define EGRCONDB_NOT_DONE   0xFFFFDFFF
#define BPCOMP_NOT_DONE     0xFFFFBFFF
#define BPCOND_NOT_DONE     0xFFFF7FFF
#define FUELCOMP_NOT_DONE   0xFFFEFFFF
#define FUELCOND_NOT_DONE   0xFFFDFFFF


// used with the on-screen test status
#define SUB_TEST_NORMAL     0x00
#define SUB_TEST_FAIL       0x01
#define SUB_TEST_DTC        0x02
#define SUB_TEST_INITIAL    0x80


#define SetFieldDec(index,val)  (update_screen_dec(_dynamic_elements11,_num_dynamic_elements11,index,val))
#define SetFieldHex(index,val)  (update_screen_hex(_dynamic_elements11,_num_dynamic_elements11,index,val))
#define SetFieldText(index,val) (update_screen_text(_dynamic_elements11,_num_dynamic_elements11,index,val))


#define NORMAL_TEXT         -1
#define HIGHLIGHTED_TEXT    8
#define RED_TEXT            1


/*
*******************************************************************************
** TestToVerifyPerformanceCounters - Tests 11.x
** Function to run test to verify performance counters
*******************************************************************************
*/
STATUS TestToVerifyPerformanceCounters(BOOL *pbTestReEntered)
{
	unsigned long ulEngineStartTime = 0;
	unsigned int  EcuIndex;
	unsigned int  IMReadinessIndex;
	STATUS        ret_code;
	BOOL          bRunTest11_2;
	unsigned int  DoneFlags;        // Bit Map of Items Completed

	SID_REQ       SidReq;
	SID1        * pSid1;

	BOOL          bSubTestFailed = FALSE;

	// initialize arrays
	memset (BankSupport, FALSE, sizeof(BankSupport));
	memset (Test11_Sid1Pid1, 0x00, sizeof(Test11_Sid1Pid1));
	memset (PreviousSid1Pid1, 0x00, sizeof(PreviousSid1Pid1));

	memset (Test11_5_Sid9Ipt, 0x00, sizeof(Test11_5_Sid9Ipt));
	memset (Test11_11_Sid9Ipt, 0x00, sizeof(Test11_11_Sid9Ipt));
	memset (Test11CurrentDisplayData, 0x00, sizeof(Test11CurrentDisplayData));

	for (IMReadinessIndex = 0; IMReadinessIndex < 11; IMReadinessIndex++)
	{
		for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
		{
			Test11IMStatus[EcuIndex][IMReadinessIndex] = NotSupported;
		}
		CurrentIMStatus[IMReadinessIndex] = Invalid;
	}


//*******************************************************************************
	// Test 11.1
	TestSubsection = 1;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify performance counters)");

	if (gOBDEngineRunning == FALSE)
	{
		Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
		     "Turn ignition to crank position and start engine.\n");
		gOBDEngineRunning = TRUE;
		ulEngineStartTime = GetTickCount();
	}

	// Verify still connected
	if ( VerifyLinkActive () == FAIL )
	{
		bSubTestFailed = TRUE;

		/* lost connection, probably due to crank - so re-establish connection */
		DisconnectProtocol ();

		if ( ConnectProtocol () == FAIL )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Connect Protocol unsuccessful\n");
			Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
			return FAIL;
		}
	}

	/* check if need to run test 11.2 */
	SidReq.SID    = 1;
	SidReq.NumIds = 1;
	SidReq.Ids[0] = 1;

	if (SidRequest (&SidReq, SID_REQ_NORMAL) == FAIL)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID 1 PID 1 request unsuccessful\n");
		Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
		return FAIL;
	}

	bRunTest11_2 = FALSE;
	for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
	{
		if (IsSid1PidSupported (EcuIndex, 1) == TRUE)
		{
			pSid1 = (SID1 *)&gOBDResponse[EcuIndex].Sid1Pid[0];

			/* save for EvaluateSid9Ipt () */
			Test11_Sid1Pid1[EcuIndex].PID = pSid1->PID;
			Test11_Sid1Pid1[EcuIndex].Data[0] = pSid1->Data[0];
			Test11_Sid1Pid1[EcuIndex].Data[1] = pSid1->Data[1];
			Test11_Sid1Pid1[EcuIndex].Data[2] = pSid1->Data[2];
			Test11_Sid1Pid1[EcuIndex].Data[3] = pSid1->Data[3];

			if (IsIM_ReadinessComplete (pSid1) == FALSE)
				bRunTest11_2 = TRUE;
		}
	}

	if ( LogSid9Ipt () != PASS )
	{
		bSubTestFailed = TRUE;
		gOBDTestSubsectionFailed = FALSE;
	}

	/* Retrieve Sid 9 IPT from the first run of test 10.10 */
	if ( ReadSid9IptFromLogFile ( "**** Test 10.10 (", "**** Test 10.10", Test10_10_Sid9Ipt) == FALSE )
	{
		bSubTestFailed = TRUE;
		gOBDTestSubsectionFailed = FALSE;
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Could not find Test 10.10 SID $9 IPT data in log file.\n");
	}


	// collect current Sid 9 IPT data
	for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
	{
		if ( IsSid9InfSupported ( EcuIndex, 0x08 ) == TRUE ||
		     IsSid9InfSupported ( EcuIndex, 0x0B ) == TRUE )
		{
			if ( GetSid9IptData ( EcuIndex, &Test11_5_Sid9Ipt[EcuIndex] ) == PASS )
			{
				// Initialize Test11CurrentDisplayData for first DisplayEcuData
				SaveSid9IptData ( EcuIndex, &Test11_5_Sid9Ipt[EcuIndex] );

				ret_code = EvaluateSid9Ipt ( EcuIndex, &DoneFlags, 1, FALSE );
				// if conditions require running of test 11.2
				if ( (ret_code & FAIL) == FAIL )
				{
					bRunTest11_2 = TRUE;
				}
				// if conditions indicate that test 11.2 may not finish normally
				if ( (ret_code & ERRORS) == ERRORS )
				{
					bSubTestFailed = TRUE;
				}
			}
		}
	}


	if ( VerifyVehicleState(gOBDEngineRunning, gOBDHybridFlag) != PASS )
	{
		bSubTestFailed = TRUE;
	}

	if ( bSubTestFailed == TRUE || gOBDTestSubsectionFailed == TRUE )
	{
		if (
		     (Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT,
		           "Errors detected that may cause the next test,\n"
		           "Test 11.2 (Complete Drive Cycle and Clear IM Readiness Bits),\n"
		           "to fail even when the test exits normally.\n" )) == 'N'
		   )
		{
			return FAIL;
		}
	}
	else
	{
		Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
	}


//*******************************************************************************
	// Test 11.2
	TestSubsection = 2;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Manufacture Specific Drive Cycle)");
	if (bRunTest11_2 == TRUE)
	{
		// tester-present message should already be active

		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "\n"
		     "Drive the vehicle in the manufacturer-specified manner to complete all the\n"
		     "OBD monitors required to set all the supported I/M Readiness bits to a \"Ready\"\n"
		     "condition. The vehicle may have to \"soak\" with the ignition off (engine off).\n"
		     "Allow vehicle to soak according to the manufacturer-specified conditions in\n"
		     "order to run any engine-off diagnostics and/or prepare the vehicle for any \n"
		     "engine-running diagnostics on the next drive cycle that requires an engine-off\n"
		     "soak period.\n\n");

		if (
		     Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_PROMPT,
		          "Would you like to use the J1699 software as a monitor to view\n"
		          "the status of the I/M Readiness bits?\n"
		          "(Enter 'N' to exit the software if you are going to turn the vehicle off\n"
		          "or don't need a monitor. You can return to this point, at any time,\n"
		          "by re-starting the J1699 software and selecting 'Dynamic Tests'.)\n") != 'Y'
		   )
		{
			Log( SUBSECTION_INCOMPLETE_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
			ABORT_RETURN;
		}


		// stop tester-present message
		StopPeriodicMsg (TRUE);
		Sleep (gOBDRequestDelay);


		ret_code = RunDynamicTest11 (*pbTestReEntered, ulEngineStartTime);


		// re-start tester-present message
		StartPeriodicMsg ();


		if ( VerifyVehicleState(gOBDEngineRunning, gOBDHybridFlag) != PASS )
		{
			bSubTestFailed = TRUE;
		}

		if ( (ret_code == FAIL) || (gOBDTestSubsectionFailed == TRUE) || (bSubTestFailed == TRUE) )
		{
			if ( Log( SUBSECTION_FAILED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT, "") == 'N' )
			{
				return FAIL;
			}
		}
		else if ( (ret_code == ABORT) || (gOBDTestSectionAborted == TRUE) )
		{
			Log( SUBSECTION_INCOMPLETE_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
			ABORT_RETURN;
		}
		else
		{
			Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
		}
	}
	else
	{
		Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");
	}


//*******************************************************************************

	// Test 11.3
	TestSubsection = 3;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Turn engine off to allow drive cycle data to be collected without it changing during data collection process.))");


	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Pull over to a safe spot and turn the ignition off for at least 30 sec.\n"
	     "Keep tool connected to the J1962 connector.  ");


	Log( PROMPT, SCREENOUTPUTON, LOGOUTPUTON, ENTER_PROMPT,
	     "Turn key on without cranking or starting engine.\n");

	Log( SUBSECTION_PASSED_RESULT, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "");

	/* Engine should now be not running */
	gOBDEngineRunning = FALSE;


//*******************************************************************************

	// Test 11.4
	TestSubsection = 4;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Re-establish communication, ignition on, engine off)");

	if ( DetermineProtocol() != PASS )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Protocol determination unsuccessful.\n" );
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

	// Test 11.5
	TestSubsection = 5;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Diagnostic Data (Service $01), Engine Off)");
	if ( VerifyIM_Ready () != PASS )
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
	// Test 11.6
	TestSubsection = 6;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Monitor Test Results (Service $06), Engine Off)");
	if ( VerifyMonitorTestSupportAndResults () != PASS )
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
	// Test 11.7
	TestSubsection = 7;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Vehicle Information (Sevice $09), Engine Off)");

	if ( LogSid9Ipt () != PASS || gOBDTestSubsectionFailed == TRUE )
	{
		bSubTestFailed = TRUE;
		gOBDTestSubsectionFailed = FALSE;
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 IPT data error\n" );
	}
	else
	{
		for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
		{
			if ( GetSid9IptData ( EcuIndex, &Test11_5_Sid9Ipt[EcuIndex] ) == PASS )
			{
				if ( EvaluateSid9Ipt ( EcuIndex, &DoneFlags, 5, TRUE ) != PASS )
				{
					bSubTestFailed = TRUE;
				}
			}
		}
	}

	if ( EvaluateInf14 () != PASS )
	{
		bSubTestFailed = TRUE;
	}
	
	if ( bSubTestFailed == TRUE || gOBDTestSubsectionFailed == TRUE )
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
	// Test 11.8
	TestSubsection = 8;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Diagnostic Data (Service $01), Engine Off)");

	if ( VerifyIM_Ready () != PASS )
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
	// Test 11.9
	TestSubsection = 9;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify DTCs (Service $03), Engine Off)");
	if ( VerifyDTCStoredData () != PASS )
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
	// Test 11.10
	TestSubsection = 10;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Pending DTCs (Service $07), Engine Off)");
	if ( VerifyDTCPendingData () != PASS )
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
	// Test 11.11
	TestSubsection = 11;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Clear DTCs (Service $04), Engine Off)");
	if ( ClearCodes () != PASS )
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
	// Test 11.12
	TestSubsection = 12;
	gOBDTestSubsectionFailed = FALSE;
	bSubTestFailed = FALSE;
	Log( SUBSECTION_BEGIN, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "(Verify Vehicle Information (Service $09), Engine Off)");
	if ( LogSid9Ipt () != PASS || gOBDTestSubsectionFailed == TRUE )
	{
		bSubTestFailed = TRUE;
		gOBDTestSubsectionFailed = FALSE;
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 IPT data error\n");
	}

	else
	{
		for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
		{
			if ( GetSid9IptData ( EcuIndex, &Test11_11_Sid9Ipt[EcuIndex] ) == PASS )
			{
				if ( EvaluateSid9Ipt ( EcuIndex, &DoneFlags, 11, TRUE ) != PASS )
				{
					bSubTestFailed = TRUE;
				}
			}
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
** IsIM_ReadinessComplete
**
**  supported bits :: 1 -> supported,    0 -> not supported
**
**    status  bits :: 1 -> not complete, 0 -> complete (or not applicable)
**
*******************************************************************************
*/
BOOL IsIM_ReadinessComplete (SID1 * pSid1)
{
	if (pSid1->PID != 1)
		return FALSE;

	//     supported bits    status bits
	if ( ((pSid1->Data[1] & (pSid1->Data[1] >> 4)) & 0x07) != 0)
		return FALSE;

	//    supported bits   status bits
	if ( (pSid1->Data[2] & pSid1->Data[3]) != 0)
		return FALSE;

	return TRUE;
}


//*****************************************************************************
// EvaluateSid9Ipt
//*****************************************************************************
STATUS EvaluateSid9Ipt ( int EcuIndex, unsigned int *pDoneFlags, int test_stage, BOOL bDisplayErrorMsg )
{
	BOOL bEvapCounts = FALSE;
	unsigned int usECUIndexLoop;
	int IptIndex;
	int count;
	int count1;
	int count2;
	int count3;
	int count4;

	STATUS test_status = PASS;


	// if not supported, then cannot do comparsions
	if ( IsSid9InfSupported (EcuIndex, 0x08) == TRUE || IsSid9InfSupported (EcuIndex, 0x0B) )
	{
		// Test 11.1 & 11.5
		if ( test_stage == 1 || test_stage == 5 )
		{
			// IGNCNTR must be >= OBDCOND
			if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_IGNCNTR_INDEX] < Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_OBDCOND_INDEX] )
			{
				if ( bDisplayErrorMsg == TRUE )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  IGNCNTR less than OBDCOND\n", GetEcuId(EcuIndex) );
				}
				test_status |= FAIL;
				*pDoneFlags &= IGNCNTR_NOT_DONE;
			}

			// IGNCNTR must be greater than the value in test 10.10
			if ( Test11_5_Sid9Ipt[EcuIndex].Flags != 0 &&
			     Test10_10_Sid9Ipt[EcuIndex].Flags != 0 )
			{
				count = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_IGNCNTR_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_IGNCNTR_INDEX];
				if ( count < 1 )
				{
					if ( bDisplayErrorMsg == TRUE )
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  IGNCNTR incremented less than 1 since Test 10.10\n", GetEcuId(EcuIndex) );
					}
					test_status |= FAIL;
					*pDoneFlags &= IGNCNTR_NOT_DONE;
				}
			}

			// OBDCOND must be >= other monitor condition counters
			for ( IptIndex = 3; IptIndex < Test11_5_Sid9Ipt[EcuIndex].NODI; IptIndex += 2 )
			{
				if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_OBDCOND_INDEX] < Test11_5_Sid9Ipt[EcuIndex].IPT[IptIndex] )
				{
					if ( bDisplayErrorMsg == TRUE )
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  OBDCOND not greater than all other monitor condition counters\n", GetEcuId(EcuIndex) );
					}
					test_status |= FAIL;
					*pDoneFlags &= OBDCOND_NOT_DONE;
				}
			}

			// OBDCOND must have incremented by at least 1 since test 10.10
			if ( Test11_5_Sid9Ipt[EcuIndex].Flags != 0 &&
			     Test10_10_Sid9Ipt[EcuIndex].Flags != 0 )
			{
				count = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_OBDCOND_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_OBDCOND_INDEX];
				if ( count < 1 )
				{
					if ( bDisplayErrorMsg == TRUE )
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  OBDCOND incremented less than 1 since Test 10.10\n", GetEcuId(EcuIndex) );
					}
					test_status |= FAIL;
					*pDoneFlags &= OBDCOND_NOT_DONE;
				}
			}

			// if this vehicle supports SID 9 INF 8
			if ( Test11_5_Sid9Ipt[EcuIndex].INF == 0x08 )
			{
				// if CAT monitoring not supported (CAT_SUP and HCAT_SUP == 0), must be zero
				if ( (Test11_Sid1Pid1[EcuIndex].Data[2] & 0x03) == 0 )
				{
					if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_CATCOMP1_INDEX] != 0 ||  // CATCOMP1
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_CATCOND1_INDEX] != 0 ||  // CATCOND1
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_CATCOMP2_INDEX] != 0 ||  // CATCOMP2
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_CATCOND2_INDEX] != 0 )   // CATCOND2
					{
						if ( bDisplayErrorMsg == TRUE || test_stage == 1 )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  CATCOMP1, CATCOMP2, CATCOND1, CATCOND2 not supported, must be zero\n", GetEcuId(EcuIndex) );
						}
						test_status |= ERRORS;
					}
				}
				// if CAT monitoring is supported, at least one bank must have increased
				else
				{
					// Check if Bank 1 is supported
					if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_CATCOMP1_INDEX] > 0 ||  // CATCOMP1
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_CATCOND1_INDEX] > 0 ||  // CATCOND1
					     Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_CATCOMP1_INDEX] > 0 ||  // CATCOMP1
					     Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_CATCOND1_INDEX] > 0 )   // CATCOND1
					{
						BankSupport[EcuIndex].bCatBank1 = TRUE;
					}

					// Check if Bank 2 is supported
					if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_CATCOMP2_INDEX] > 0 ||  // CATCOMP2
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_CATCOND2_INDEX] > 0 ||  // CATCOND2
					     Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_CATCOMP2_INDEX] > 0 ||  // CATCOMP2
					     Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_CATCOND2_INDEX] > 0 )   // CATCOND2
					{
						BankSupport[EcuIndex].bCatBank2 = TRUE;
					}

					// Check for change
					count1 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_CATCOMP1_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_CATCOMP1_INDEX];
					count2 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_CATCOND1_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_CATCOND1_INDEX];
					count3 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_CATCOMP2_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_CATCOMP2_INDEX];
					count4 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_CATCOND2_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_CATCOND2_INDEX];

					if ( count1 == 0 && count2 == 0 && BankSupport[EcuIndex].bCatBank1 == FALSE &&
						 count3 == 0 && count4 == 0 && BankSupport[EcuIndex].bCatBank2 == FALSE)
					{
						// can't tell which bank is supported, so highlight them both till we see one change
						if (bDisplayErrorMsg == TRUE)
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Neither bank of the CAT monitor changed since Test 10.10\n", GetEcuId(EcuIndex) );
						}
						*pDoneFlags &= CATCOMP1_NOT_DONE;
						*pDoneFlags &= CATCOND1_NOT_DONE;
						*pDoneFlags &= CATCOMP2_NOT_DONE;
						*pDoneFlags &= CATCOND2_NOT_DONE;
						test_status |= FAIL;
					}
					else
					{
						if ( ( count1 == 0 || count2 == 0 ) && BankSupport[EcuIndex].bCatBank1 )
						{
							if (bDisplayErrorMsg == TRUE)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Bank 1 of the CAT monitor has not changed since Test 10.10\n", GetEcuId(EcuIndex) );
							}
							test_status |= FAIL;
						}

						if ( ( count3 == 0 || count4 == 0 ) && BankSupport[EcuIndex].bCatBank2 )
						{
							if (bDisplayErrorMsg == TRUE)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Bank 2 of the CAT monitor has not changed since Test 10.10\n", GetEcuId(EcuIndex) );
							}
							test_status |= FAIL;
						}

						if ( count1 == 0 && BankSupport[EcuIndex].bCatBank1 == TRUE )
							*pDoneFlags &= CATCOMP1_NOT_DONE;
						if ( count2 == 0 && BankSupport[EcuIndex].bCatBank1 == TRUE )
							*pDoneFlags &= CATCOND1_NOT_DONE;
						if ( count3 == 0 && BankSupport[EcuIndex].bCatBank2 == TRUE )
							*pDoneFlags &= CATCOMP2_NOT_DONE;
						if ( count4 == 0 && BankSupport[EcuIndex].bCatBank2 == TRUE )
							*pDoneFlags &= CATCOND2_NOT_DONE;
					}
				}

				// if O2 monitoring not supported, must be zero
				if ( (Test11_Sid1Pid1[EcuIndex].Data[2] & 0x20) == 0 )
				{
					if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_O2SCOMP1_INDEX] != 0 ||  // O2SCOMP1
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_O2SCOND1_INDEX] != 0 ||  // O2SCOND1
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_O2SCOMP2_INDEX] != 0 ||  // O2SCOMP2
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_O2SCOND2_INDEX] != 0 )   // O2SCOND2
					{
						if ( bDisplayErrorMsg == TRUE || test_stage == 1 )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  O2SCOMP1, O2SCOMP2, O2SCOND1, O2SCOND2 not supported, must be zero\n", GetEcuId(EcuIndex) );
						}
						test_status |= ERRORS;
					}

					if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_SO2SCOMP1_INDEX] != 0 ||  // SO2SCOMP1
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_SO2SCOND1_INDEX] != 0 ||  // SO2SCOND1
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_SO2SCOMP2_INDEX] != 0 ||  // SO2SCOMP2
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_SO2SCOND2_INDEX] != 0 )   // SO2SCOND2
					{
						if ( bDisplayErrorMsg == TRUE || test_stage == 1 )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SO2SCOMP1, SO2SCOMP2, SO2SCOND1, SO2SCOND2 not supported, must be zero\n", GetEcuId(EcuIndex) );
						}
						test_status |= ERRORS;
					}
				}
				// if O2 monitoring is supported, at least one bank must have increased
				else
				{
					//
					// O2 checks
					//
					// Check if Bank 1 is supported
					if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_O2SCOMP1_INDEX] > 0 ||  // O2SCOMP1
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_O2SCOND1_INDEX] > 0 ||  // O2SCOND1
					     Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_O2SCOMP1_INDEX] > 0 ||  // O2SCOMP1
					     Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_O2SCOND1_INDEX] > 0 )   // O2SCOND1
					{
						BankSupport[EcuIndex].bO2Bank1 = TRUE;
					}

					// Check if Bank 2 is supported
					if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_O2SCOMP2_INDEX] > 0 ||  // O2SCOMP2
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_O2SCOND2_INDEX] > 0 ||  // O2SCOND2
					     Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_O2SCOMP2_INDEX] > 0 ||  // O2SCOMP2
					     Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_O2SCOND2_INDEX] > 0 )   // O2SCOND2
					{
						BankSupport[EcuIndex].bO2Bank2 = TRUE;
					}

					// Check for change
					count1 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_O2SCOMP1_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_O2SCOMP1_INDEX];
					count2 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_O2SCOND1_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_O2SCOND1_INDEX];
					count3 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_O2SCOMP2_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_O2SCOMP2_INDEX];
					count4 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_O2SCOND2_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_O2SCOND2_INDEX];

					if ( count1 == 0 && count2 == 0 && BankSupport[EcuIndex].bO2Bank1 == FALSE &&
						 count3 == 0 && count4 == 0 && BankSupport[EcuIndex].bO2Bank2 == FALSE )
					{
						if ( bDisplayErrorMsg == TRUE )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Neither bank of the O2 monitor changed since Test 10.10\n", GetEcuId(EcuIndex) );
						}
						*pDoneFlags &= O2SCOMP1_NOT_DONE;
						*pDoneFlags &= O2SCOND1_NOT_DONE;
						*pDoneFlags &= O2SCOMP2_NOT_DONE;
						*pDoneFlags &= O2SCOND2_NOT_DONE;
						test_status |= FAIL;
					}
					else
					{
						if ( ( count1 == 0 || count2 == 0 ) && BankSupport[EcuIndex].bO2Bank1 )
						{
							if (bDisplayErrorMsg == TRUE)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Bank 1 of the O2 monitor has not changed since Test 10.10\n", GetEcuId(EcuIndex) );
							}
							test_status |= FAIL;
						}

						if ( ( count3 == 0 || count4 == 0 ) && BankSupport[EcuIndex].bO2Bank2 )
						{
							if (bDisplayErrorMsg == TRUE)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Bank 2 of the O2 monitor has not changed since Test 10.10\n", GetEcuId(EcuIndex) );
							}
							test_status |= FAIL;
						}

						if ( count1 == 0 && BankSupport[EcuIndex].bO2Bank1 == TRUE )
							*pDoneFlags &= O2SCOMP1_NOT_DONE;
						if ( count2 == 0 && BankSupport[EcuIndex].bO2Bank1 == TRUE )
							*pDoneFlags &= O2SCOND1_NOT_DONE;
						if ( count3 == 0 && BankSupport[EcuIndex].bO2Bank2 == TRUE )
							*pDoneFlags &= O2SCOMP2_NOT_DONE;
						if ( count4 == 0 && BankSupport[EcuIndex].bO2Bank2 == TRUE )
							*pDoneFlags &= O2SCOND2_NOT_DONE;
					}

					//
					// SO2 checks
					//
					if (Test11_5_Sid9Ipt[EcuIndex].NODI == 0x14 || Test11_5_Sid9Ipt[EcuIndex].NODI == 0x1C )
					{
						// Check if Bank 1 is supported
						if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_SO2SCOMP1_INDEX] > 0 ||  // SO2SCOMP1
							 Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_SO2SCOND1_INDEX] > 0 ||  // SO2SCOND1
							 Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_SO2SCOMP1_INDEX] > 0 ||  // SO2SCOMP1
							 Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_SO2SCOND1_INDEX] > 0 )   // SO2SCOND1
						{
							BankSupport[EcuIndex].bSO2Bank1 = TRUE;
						}

						// Check if Bank 2 is supported
						if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_SO2SCOMP2_INDEX] > 0 ||  // SO2SCOMP2
							 Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_SO2SCOND2_INDEX] > 0 ||  // SO2SCOND2
							 Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_SO2SCOMP2_INDEX] > 0 ||  // SO2SCOMP2
							 Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_SO2SCOND2_INDEX] > 0 )   // SO2SCOND2
						{
							BankSupport[EcuIndex].bSO2Bank2 = TRUE;
						}

						// Check for change
						count1 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_SO2SCOMP1_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_SO2SCOMP1_INDEX];
						count2 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_SO2SCOND1_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_SO2SCOND1_INDEX];
						count3 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_SO2SCOMP2_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_SO2SCOMP2_INDEX];
						count4 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_SO2SCOND2_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_SO2SCOND2_INDEX];

						if ( count1 == 0 && count2 == 0 && BankSupport[EcuIndex].bSO2Bank1 == FALSE &&
							 count3 == 0 && count4 == 0 && BankSupport[EcuIndex].bSO2Bank2 == FALSE )
						{
							if ( bDisplayErrorMsg == TRUE )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Neither bank of the SO2 monitor changed since Test 10.10\n", GetEcuId(EcuIndex) );
							}
							*pDoneFlags &= SO2SCOMP1_NOT_DONE;
							*pDoneFlags &= SO2SCOND1_NOT_DONE;
							*pDoneFlags &= SO2SCOMP2_NOT_DONE;
							*pDoneFlags &= SO2SCOND2_NOT_DONE;
							test_status |= FAIL;
						}
						else
						{
							if ( ( count1 == 0 || count2 == 0 ) && BankSupport[EcuIndex].bSO2Bank1 )
							{
								if (bDisplayErrorMsg == TRUE)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  Bank 1 of the SO2 monitor has not changed since Test 10.10\n", GetEcuId(EcuIndex) );
								}
								test_status |= FAIL;
							}

							if ( ( count3 == 0 || count4 == 0 ) && BankSupport[EcuIndex].bSO2Bank2 )
							{
								if (bDisplayErrorMsg == TRUE)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  Bank 2 of the SO2 monitor has not changed since Test 10.10\n", GetEcuId(EcuIndex) );
								}
								test_status |= FAIL;
							}

							if ( count1 == 0 && BankSupport[EcuIndex].bSO2Bank1 == TRUE )
								*pDoneFlags &= SO2SCOMP1_NOT_DONE;
							if ( count2 == 0 && BankSupport[EcuIndex].bSO2Bank1 == TRUE )
								*pDoneFlags &= SO2SCOND1_NOT_DONE;
							if ( count3 == 0 && BankSupport[EcuIndex].bSO2Bank2 == TRUE )
								*pDoneFlags &= SO2SCOMP2_NOT_DONE;
							if ( count4 == 0 && BankSupport[EcuIndex].bSO2Bank2 == TRUE )
								*pDoneFlags &= SO2SCOND2_NOT_DONE;
						}
					} // end if (Test11_5_Sid9Ipt[EcuIndex].NODI == 0x14 || 0x1C)
				}

				//
				// AFRI & PF checks
				//
				if (Test11_5_Sid9Ipt[EcuIndex].NODI == 0x1C )
				{
					// Check if AFRI Bank 1 is supported
					if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_AFRICOMP1_INDEX] > 0 ||   // AFRICOMP1
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_AFRICOND1_INDEX] > 0 ||   // AFRICOND1
					     Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_AFRICOMP1_INDEX] > 0 ||  // AFRICOMP1
					     Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_AFRICOND1_INDEX] > 0 )   // AFRICOND1
					{
						BankSupport[EcuIndex].bAFRIBank1 = TRUE;
					}

					// Check if AFRI Bank 2 is supported
					if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_AFRICOMP2_INDEX] > 0 ||   // AFRICOMP2
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_AFRICOND2_INDEX] > 0 ||   // AFRICOND2
					     Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_AFRICOMP2_INDEX] > 0 ||  // AFRICOMP2
					     Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_AFRICOND2_INDEX] > 0 )   // AFRICOND2
					{
						BankSupport[EcuIndex].bAFRIBank2 = TRUE;
					}

					// Check for AFRI change
					count1 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_AFRICOMP1_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_AFRICOMP1_INDEX];
					count2 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_AFRICOND1_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_AFRICOND1_INDEX];
					count3 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_AFRICOMP2_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_AFRICOMP2_INDEX];
					count4 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_AFRICOND2_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_AFRICOND2_INDEX];

					if ( count1 == 0 && count2 == 0 && BankSupport[EcuIndex].bAFRIBank1 == FALSE &&
					     count3 == 0 && count4 == 0 && BankSupport[EcuIndex].bAFRIBank2 == FALSE )
					{
						if ( bDisplayErrorMsg == TRUE )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Neither bank of the AFRI monitor changed since Test 10.10\n", GetEcuId(EcuIndex) );
						}
						*pDoneFlags &= AFRICOMP1_NOT_DONE;
						*pDoneFlags &= AFRICOND1_NOT_DONE;
						*pDoneFlags &= AFRICOMP2_NOT_DONE;
						*pDoneFlags &= AFRICOND2_NOT_DONE;
						test_status |= FAIL;
					}
					else
					{
						if ( ( count1 == 0 || count2 == 0 ) && BankSupport[EcuIndex].bAFRIBank1 )
						{
							if (bDisplayErrorMsg == TRUE)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Bank 1 of the AFRI monitor has not changed since Test 10.10\n", GetEcuId(EcuIndex) );
							}
							test_status |= FAIL;
						}

						if ( ( count3 == 0 || count4 == 0 ) && BankSupport[EcuIndex].bAFRIBank2 )
						{
							if (bDisplayErrorMsg == TRUE)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Bank 2 of the AFRI monitor has not changed since Test 10.10\n", GetEcuId(EcuIndex) );
							}
							test_status |= FAIL;
						}

						if ( count1 == 0 && BankSupport[EcuIndex].bAFRIBank1 == TRUE )
							*pDoneFlags &= AFRICOMP1_NOT_DONE;
						if ( count2 == 0 && BankSupport[EcuIndex].bAFRIBank1 == TRUE )
							*pDoneFlags &= AFRICOND1_NOT_DONE;
						if ( count3 == 0 && BankSupport[EcuIndex].bAFRIBank2 == TRUE )
							*pDoneFlags &= AFRICOMP2_NOT_DONE;
						if ( count4 == 0 && BankSupport[EcuIndex].bAFRIBank2 == TRUE )
							*pDoneFlags &= AFRICOND2_NOT_DONE;
					}

					// Check if PF Bank 1 is supported
					if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_PFCOMP1_INDEX] > 0 ||   // PFCOMP1
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_PFCOND1_INDEX] > 0 ||   // PFCOND1
					     Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_PFCOMP1_INDEX] > 0 ||  // PFCOMP1
					     Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_PFCOND1_INDEX] > 0 )   // PFCOND1
					{
						BankSupport[EcuIndex].bPFBank1 = TRUE;
					}

					// Check if PF Bank 2 is supported
					if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_PFCOMP2_INDEX] > 0 ||   // PFCOMP2
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_PFCOND2_INDEX] > 0 ||   // PFCOND2
					     Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_PFCOMP2_INDEX] > 0 ||  // PFCOMP2
					     Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_PFCOND2_INDEX] > 0 )   // PFCOND2
					{
						BankSupport[EcuIndex].bPFBank2 = TRUE;
					}

					// Check for PF change
					count1 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_PFCOMP1_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_PFCOMP1_INDEX];
					count2 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_PFCOND1_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_PFCOND1_INDEX];
					count3 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_PFCOMP2_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_PFCOMP2_INDEX];
					count4 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_PFCOND2_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_PFCOND2_INDEX];

					if ( count1 == 0 && count2 == 0 && BankSupport[EcuIndex].bPFBank1 == FALSE &&
					     count3 == 0 && count4 == 0 && BankSupport[EcuIndex].bPFBank2 == FALSE )
					{
						if ( bDisplayErrorMsg == TRUE )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Neither bank of the PF monitor changed since Test 10.10\n", GetEcuId(EcuIndex) );
						}
						*pDoneFlags &= PFCOMP1_NOT_DONE;
						*pDoneFlags &= PFCOND1_NOT_DONE;
						*pDoneFlags &= PFCOMP2_NOT_DONE;
						*pDoneFlags &= PFCOND2_NOT_DONE;
						test_status |= FAIL;
					}
					else
					{
						if ( ( count1 == 0 || count2 == 0 ) && BankSupport[EcuIndex].bPFBank1 )
						{
							if (bDisplayErrorMsg == TRUE)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Bank 1 of the PF monitor has not changed since Test 10.10\n", GetEcuId(EcuIndex) );
							}
							test_status |= FAIL;
						}

						if ( ( count3 == 0 || count4 == 0 ) && BankSupport[EcuIndex].bPFBank2 )
						{
							if (bDisplayErrorMsg == TRUE)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Bank 2 of the PF monitor has not changed since Test 10.10\n", GetEcuId(EcuIndex) );
							}
							test_status |= FAIL;
						}

						if ( count1 == 0 && BankSupport[EcuIndex].bPFBank1 == TRUE )
							*pDoneFlags &= PFCOMP1_NOT_DONE;
						if ( count2 == 0 && BankSupport[EcuIndex].bPFBank1 == TRUE )
							*pDoneFlags &= PFCOND1_NOT_DONE;
						if ( count3 == 0 && BankSupport[EcuIndex].bPFBank2 == TRUE )
							*pDoneFlags &= PFCOMP2_NOT_DONE;
						if ( count4 == 0 && BankSupport[EcuIndex].bPFBank2 == TRUE )
							*pDoneFlags &= PFCOND2_NOT_DONE;
					}
				} // end if (Test11_5_Sid9Ipt[EcuIndex].NODI == 0x1C)


				// if EGR monitor not supported, must be zero
				if ( (Test11_Sid1Pid1[EcuIndex].Data[2] & 0x80) == 0 )
				{
					if ( gModelYear >= 2011 )
					{
						if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_INF8_EGRCOMP_INDEX] != 0 || // EGRCOMP
						     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_INF8_EGRCOND_INDEX] != 0 )  // EGRCOND
						{
							if ( bDisplayErrorMsg == TRUE || test_stage == 1 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGRCOMP and EGRCOND not supported, must be zero\n", GetEcuId(EcuIndex) );
							}
							test_status |= ERRORS;
						}
					}
				}
				// if EGR monitor supported, EGRCOMP, EGRCOND must have changed since test 10.10
				else
				{
					count = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_INF8_EGRCOMP_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_INF8_EGRCOMP_INDEX];
					if ( count == 0 )
					{
						if ( bDisplayErrorMsg == TRUE )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EGRCOMP unchanged since Test 10.10\n" , GetEcuId(EcuIndex) );
						}
						test_status |= FAIL;
						*pDoneFlags &= EGRCOMP8_NOT_DONE;
					}

					count = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_INF8_EGRCOND_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_INF8_EGRCOND_INDEX];
					if ( count == 0 )
					{
						if ( bDisplayErrorMsg == TRUE )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EGRCOND unchanged since Test 10.10\n", GetEcuId(EcuIndex) );
						}
						test_status |= FAIL;
						*pDoneFlags &= EGRCOND8_NOT_DONE;
					}
				}

				// if AIR monitoring not supported, must be zero
				if ( (Test11_Sid1Pid1[EcuIndex].Data[2] & 0x08) == 0 )
				{
					if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_AIRCOMP_INDEX] != 0 || // AIRCOMP
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_AIRCOND_INDEX] != 0 )  // AIRCOND
					{
						if ( bDisplayErrorMsg == TRUE || test_stage == 1 )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  AIRCOMP and AIRCOND not supported, must be zero\n", GetEcuId(EcuIndex) );
						}
						test_status |= ERRORS;
					}
				}
				// if Air monitor supported, AIRCOMP, AIRCOND must have changed since test 10.10
				else
				{
					count = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_AIRCOMP_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_AIRCOMP_INDEX];
					if ( count == 0 )
					{
						if ( bDisplayErrorMsg == TRUE )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  AIRCOMP unchanged since Test 10.10\n", GetEcuId(EcuIndex) );
						}
						test_status |= FAIL;
						*pDoneFlags &= AIRCOMP_NOT_DONE;
					}

					count = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_AIRCOND_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_AIRCOND_INDEX];
					if ( count == 0 )
					{
						if (bDisplayErrorMsg == TRUE)
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  AIRCOND unchanged since Test 10.10\n", GetEcuId(EcuIndex) );
						}
						test_status |= FAIL;
						*pDoneFlags &= AIRCOND_NOT_DONE;
					}
				}

				// if EVAP monitoring not supported, must be zero
				if ( (Test11_Sid1Pid1[EcuIndex].Data[2] & 0x04) == 0 )
				{
					if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_EVAPCOMP_INDEX] != 0 || // EVAPCOMP
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_EVAPCOND_INDEX] != 0 )  // EVAPCOND
					{
						if ( bDisplayErrorMsg == TRUE || test_stage == 1 )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EVAPCOMP and EVAPCOND not supported, must be zero\n", GetEcuId(EcuIndex) );
						}
						test_status |= ERRORS;
					}
				}
				// if EVAP monitor supported, EVAPCOMP & EVAPCOND for at least one ECU must have changed since test 10.10
				else
				{
					for (usECUIndexLoop = 0; usECUIndexLoop < gUserNumEcus; usECUIndexLoop++)
					{
						if ( (Test11_Sid1Pid1[usECUIndexLoop].Data[2] & 0x04) == 0x04 )
						{
							// EVAP is supported
							if (
							     (Test11_5_Sid9Ipt[usECUIndexLoop].IPT[IPT_EVAPCOMP_INDEX] != 0) ||
							     (Test11_5_Sid9Ipt[usECUIndexLoop].IPT[IPT_EVAPCOND_INDEX] != 0)
							   )
							{
								// at least one count is non-zero
								bEvapCounts = TRUE;
								break;
							}

						}
					}

					count = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_EVAPCOMP_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_EVAPCOMP_INDEX];
					if (
					     ((bEvapCounts == TRUE) && (Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_EVAPCOMP_INDEX] == 0) &&
					      (Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_EVAPCOND_INDEX] == 0) ) ||
					     (count != 0)
					   )
					{
						// skip this, don't highlight
					}
					else
					{
						if (bDisplayErrorMsg == TRUE)
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EVAPCOMP unchanged since Test 10.10\n", GetEcuId(EcuIndex) );
						}
						test_status |= FAIL;
						*pDoneFlags &= EVAPCOMP_NOT_DONE;
					}

					count = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_EVAPCOND_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_EVAPCOND_INDEX];
					if (
					     ((bEvapCounts == TRUE) && (Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_EVAPCOMP_INDEX] == 0) &&
					      (Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_EVAPCOND_INDEX] == 0) ) ||
					     (count != 0)
					   )
					{
						// skip this, don't highlight
					}
					else
					{
						if (bDisplayErrorMsg == TRUE)
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EVAPCOND unchanged since Test 10.10\n", GetEcuId(EcuIndex) );
						}
						test_status |= FAIL;
						*pDoneFlags &= EVAPCOND_NOT_DONE;
					}
				}
			} // end if this vehicle supports SID 9 INF 8

			// if this vehicle supports SID 9 INF B
			else if ( Test11_5_Sid9Ipt[EcuIndex].INF == 0x0B )
			{
				// if HCAT monitoring not supported, must be zero
				if ( (Test11_Sid1Pid1[EcuIndex].Data[2] & 0x01) == 0 )
				{
					if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_HCCATCOMP_INDEX] != 0 ||  // HCCATCOMP
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_HCCATCOND_INDEX] != 0 )   // HCCATCOND
					{
						if ( bDisplayErrorMsg == TRUE || test_stage == 1 )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  HCCATCOMP, HCCATCOND not supported, must be zero\n", GetEcuId(EcuIndex) );
						}
						test_status |= ERRORS;
					}
				}

				// if NCAT/NADS monitoring not supported, must be zero
				if ( (Test11_Sid1Pid1[EcuIndex].Data[2] & 0x02) == 0 )
				{
					if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_NCATCOMP_INDEX] != 0 ||  // NCATCOMP
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_NCATCOND_INDEX] != 0 ||  // NCATCOND
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_NADSCOMP_INDEX] != 0 ||  // NADSCOMP
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_NADSCOND_INDEX] != 0 )   // NADSCOND
					{
						if ( bDisplayErrorMsg == TRUE || test_stage == 1 )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  NCATCOMP, NADSCOMP, NCATCOND, NADSCOND not supported, must be zero\n", GetEcuId(EcuIndex) );
						}
						test_status |= ERRORS;
					}
				}
				// if NCAT monitoring is supported, at least one bank must have increased
				else
				{
					// Check if NCAT is supported
					if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_NCATCOMP_INDEX] > 0 ||  // NCATCOMP
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_NCATCOND_INDEX] > 0 ||  // NCATCOND
					     Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_NCATCOMP_INDEX] > 0 ||  // NCATCOMP
					     Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_NCATCOND_INDEX] > 0 )   // NCATCOND
					{
						BankSupport[EcuIndex].bNCAT = TRUE;
					}

					// Check if NADS is supported
					if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_NADSCOMP_INDEX] > 0 ||  // NADSCOMP
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_NADSCOND_INDEX] > 0 ||  // NADSCOND
					     Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_NADSCOMP_INDEX] > 0 ||  // NADSCOMP
					     Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_NADSCOND_INDEX] > 0 )   // NADSCOND
					{
						BankSupport[EcuIndex].bNADS = TRUE;
					}

					count1 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_NCATCOMP_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_NCATCOMP_INDEX];
					count2 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_NCATCOND_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_NCATCOND_INDEX];
					count3 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_NADSCOMP_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_NADSCOMP_INDEX];
					count4 = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_NADSCOND_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_NADSCOND_INDEX];

					if ( count1 == 0 && count2 == 0 && BankSupport[EcuIndex].bNCAT == FALSE &&
						 count3 == 0 && count4 == 0 && BankSupport[EcuIndex].bNADS == FALSE )
					{
						if ( bDisplayErrorMsg == TRUE )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Neither NCAT or NADS changed since Test 10.10\n", GetEcuId(EcuIndex) );
						}
						*pDoneFlags &= NCATCOMP_NOT_DONE;
						*pDoneFlags &= NCATCOND_NOT_DONE;
						*pDoneFlags &= NADSCOMP_NOT_DONE;
						*pDoneFlags &= NADSCOND_NOT_DONE;
						test_status |= FAIL;
					}
					else
					{
						if ( ( count1 == 0 || count2 == 0 ) && BankSupport[EcuIndex].bNCAT )
						{
							if ( bDisplayErrorMsg == TRUE )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  NCAT monitor has not changed since Test 10.10\n", GetEcuId(EcuIndex) );
							}
							test_status |= FAIL;
						}

						if ( ( count3 == 0 || count4 == 0 ) && BankSupport[EcuIndex].bNADS )
						{
							if ( bDisplayErrorMsg == TRUE )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  NADS monitor has not changed since Test 10.10\n", GetEcuId(EcuIndex) );
							}
							test_status |= FAIL;
						}

						if ( count1 == 0 && BankSupport[EcuIndex].bNCAT )
							*pDoneFlags &= NCATCOMP_NOT_DONE;
						if ( count2 == 0 && BankSupport[EcuIndex].bNCAT )
							*pDoneFlags &= NCATCOND_NOT_DONE;
						if ( count3 == 0 && BankSupport[EcuIndex].bNADS )
							*pDoneFlags &= NADSCOMP_NOT_DONE;
						if ( count4 == 0 && BankSupport[EcuIndex].bNADS )
							*pDoneFlags &= NADSCOND_NOT_DONE;
					}
				}

				// if PM monitoring not supported, must be zero
				if ( (Test11_Sid1Pid1[EcuIndex].Data[2] & 0x40) == 0 )
				{
					if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_PMCOMP_INDEX] != 0 ||  // PMCOMP
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_PMCOND_INDEX] != 0 )   // PMCOND
					{
						if ( bDisplayErrorMsg == TRUE || test_stage == 1 )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  PMCOMP and PMCOND not supported, must be zero\n", GetEcuId(EcuIndex) );
						}
						test_status |= ERRORS;
					}
				}

				// if EGS monitoring not supported, must be zero
				if ( (Test11_Sid1Pid1[EcuIndex].Data[2] & 0x20) == 0 )
				{
					if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_EGSCOMP_INDEX] != 0 || // EGSCOMP
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_EGSCOND_INDEX] != 0 )  // EGSCOMP
					{
						if ( bDisplayErrorMsg == TRUE || test_stage == 1 )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EGSCOMP and EGSCOND not supported, must be zero\n", GetEcuId(EcuIndex) );
						}
						test_status |= ERRORS;
					}
				}

				// if EGR monitor not supported, must be zero MY 2011 and later
				if ( (Test11_Sid1Pid1[EcuIndex].Data[2] & 0x80) == 0 )
				{
					if ( gModelYear >= 2011 )
					{
						if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_INFB_EGRCOMP_INDEX] != 0 || // EGRCOMP
						     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_INFB_EGRCOND_INDEX] != 0 )  // EGRCOND
						{
							if ( bDisplayErrorMsg == TRUE || test_stage == 1 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGRCOMP and EGRCOND not supported, must be zero\n", GetEcuId(EcuIndex) );
							}
							test_status |= ERRORS;
						}
					}
				}
				// if EGR monitor supported, EGRCOMP, EGRCOND must have incremented since test 10.10
				else
				{
					count = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_INFB_EGRCOMP_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_INFB_EGRCOMP_INDEX];
					if ( count == 0 )
					{
						if ( bDisplayErrorMsg == TRUE )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EGRCOMP unchanged since Test 10.10\n", GetEcuId(EcuIndex) );
						}
						test_status |= FAIL;
						*pDoneFlags &= EGRCOMPB_NOT_DONE;
					}

					count = Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_INFB_EGRCOND_INDEX] - Test10_10_Sid9Ipt[EcuIndex].IPT[IPT_INFB_EGRCOND_INDEX];
					if ( count == 0 )
					{
						if ( bDisplayErrorMsg == TRUE )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EGRCOND unchanged since Test 10.10\n", GetEcuId(EcuIndex) );
						}
						test_status |= FAIL;
						*pDoneFlags &= EGRCONDB_NOT_DONE;
					}
				}

				// if BP monitoring not supported, must be zero
				if ( (Test11_Sid1Pid1[EcuIndex].Data[2] & 0x08) == 0 )
				{
					if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_BPCOMP_INDEX] != 0 ||   // BPCOMP
					     Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_BPCOND_INDEX] != 0 )    // BPCOND
					{
						if ( bDisplayErrorMsg == TRUE || test_stage == 1 )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  BPCOMP and BPCOND not supported, must be zero\n", GetEcuId(EcuIndex) );
						}
						test_status |= ERRORS;
					}

				// No Tests for FUEL

				}
			}  // end if this vehicle supports SID 9 INF B
		}  // end if (test_stage == 2 || test_stage == 5)  // Test 11.2 & 11.5

		// Test 11.11
		if ( test_stage == 11 )
		{
			// IGNCNTR must be >= OBDCOND
			if ( Test11_11_Sid9Ipt[EcuIndex].IPT[IPT_IGNCNTR_INDEX] < Test11_11_Sid9Ipt[EcuIndex].IPT[IPT_OBDCOND_INDEX] )
			{
				if ( bDisplayErrorMsg == TRUE )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  IGNCNTR less than OBDCOND\n", GetEcuId(EcuIndex) );
				}
				test_status |= FAIL;
			}

			// OBDCOND must be >= other monitor condition counters
			for ( IptIndex = 3; IptIndex < Test11_11_Sid9Ipt[EcuIndex].NODI; IptIndex += 2 )
			{
				if ( Test11_11_Sid9Ipt[EcuIndex].IPT[IPT_OBDCOND_INDEX] < Test11_11_Sid9Ipt[EcuIndex].IPT[IptIndex] )
				{
					if ( bDisplayErrorMsg == TRUE )
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  OBDCOND not greater than all other monitor condition counters\n", GetEcuId(EcuIndex) );
					}
					test_status |= FAIL;
				}
			}

			// IGNCNTR and OBDCOND must have the same values as recorded in step 11.5
			if ( Test11_11_Sid9Ipt[EcuIndex].IPT[IPT_IGNCNTR_INDEX]  != Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_IGNCNTR_INDEX] )
			{
				if ( bDisplayErrorMsg == TRUE )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  IGNCNTR different from value in test 11.5\n", GetEcuId(EcuIndex) );
				}
				test_status |= FAIL;
			}

			if ( Test11_11_Sid9Ipt[EcuIndex].IPT[IPT_OBDCOND_INDEX]  != Test11_5_Sid9Ipt[EcuIndex].IPT[IPT_OBDCOND_INDEX] )
			{
				if ( bDisplayErrorMsg == TRUE )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  OBDCOND different from value in test 11.5\n", GetEcuId(EcuIndex) );
				}
				test_status |= FAIL;
			}

			// OBD condition counters must continue to be non-zero if they were non-zero in test 11.5
			for ( IptIndex = 2; IptIndex < Test11_11_Sid9Ipt[EcuIndex].NODI; IptIndex++ )
			{
				if ( Test11_5_Sid9Ipt[EcuIndex].IPT[IptIndex] !=0 &&
				     Test11_11_Sid9Ipt[EcuIndex].IPT[IptIndex] == 0 )
				{
					if ( bDisplayErrorMsg == TRUE )
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  An OBD counter which was non-zero in test 11.5 is now zero\n", GetEcuId(EcuIndex) );
					}
					test_status |= FAIL;
				}
			}
		}  // end if ( test_stage == 11 )  // Test 11.11
	}  // end if INF $8 or INF $B supported

	return test_status;
}


/*
*******************************************************************************
** EvaluateInf14
*******************************************************************************
*/
STATUS EvaluateInf14 ( void )
{
	STATUS RetCode = PASS;
	SID_REQ       SidReq;
	unsigned int  EcuIndex;

	SidReq.SID    = 9;
	SidReq.NumIds = 1;
	SidReq.Ids[0] = 0x14;

	/* Sid 9 Inf 14 request*/
	if ( IsSid9InfSupported (-1, 0x14) == TRUE )
	{
		if ( SidRequest( &SidReq, SID_REQ_NORMAL ) == FAIL)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $9 INF $%X request\n", SidReq.Ids[0]);
			return FAIL;
		}

		for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
		{
			if ( IsSid9InfSupported (EcuIndex, 0x14) == TRUE &&
			     VerifyInf14Data (EcuIndex) == FAIL )
			{
				RetCode = FAIL;
			}
		}
	}

	return RetCode;
}

/*
*******************************************************************************
** SelectECU
*******************************************************************************
*/
void SelectECU (int new_index, int old_index)
{
	int index;

	// un-highlite previous selection
	setrgb (NORMAL_TEXT);
	index = ECU_ID_INDEX + old_index;
	if ((ECU_ID_INDEX <= index) && (index <= ECU_ID_INDEX+OBD_MAX_ECUS))
		SetFieldHex (index, GetEcuId(old_index));

	// highlite new selection
	setrgb (HIGHLIGHTED_TEXT);
	index = ECU_ID_INDEX + new_index;
	if ((ECU_ID_INDEX <= index) && (index <= ECU_ID_INDEX+OBD_MAX_ECUS))
		SetFieldHex (index, GetEcuId(new_index));

	// restore screen attributes
	setrgb (NORMAL_TEXT);
}


/*
*******************************************************************************
** DisplayEcuData
*******************************************************************************
*/
void DisplayEcuData ( int EcuIndex, unsigned int DoneFlags )
{
	int index;

	// write IM Status
	for ( index = 0; index < 11; index++ )
	{
		// copy IM Status for active ECUs
		CurrentIMStatus[index] = Test11IMStatus[EcuIndex][index];

		if ( Test11IMStatus[EcuIndex][index] == Incomplete )
		{
			setrgb (HIGHLIGHTED_TEXT);
		}
		else if ( Test11IMStatus[EcuIndex][index] == Disabled )
		{
			setrgb (RED_TEXT);
		}

		SetFieldText ( IM_READINESS_INDEX+index, szIM_Status[Test11IMStatus[EcuIndex][index]] );
		setrgb (NORMAL_TEXT);
	}

	// write rate based counter
	if ( (DoneFlags & 0x01) == 0 )
		setrgb (HIGHLIGHTED_TEXT);
	SetFieldDec ( OBD_MONITOR_COND_INDEX, Test11CurrentDisplayData[EcuIndex].IPT[0] );
	setrgb (NORMAL_TEXT);

	if ( ((DoneFlags>>=1) & 0x01) == 0 )
		setrgb (HIGHLIGHTED_TEXT);
	SetFieldDec ( IGNITION_COUNTER_INDEX, Test11CurrentDisplayData[EcuIndex].IPT[1] );
	setrgb (NORMAL_TEXT);

	// write initial values for rate based counters
	for ( index = 2; index < Test10_10_Sid9Ipt[EcuIndex].NODI; index++ )
	{
		SetFieldDec ( OBD_MONITOR_COND_INDEX+index, Test10_10_Sid9Ipt[EcuIndex].IPT[index] );
	}

	// write current values for rate based counters
	for ( index = 2; index < Test11CurrentDisplayData[EcuIndex].NODI; index++ )
	{
		if ( ((DoneFlags>>=1) & 0x01) == 0 )
		{
			setrgb (HIGHLIGHTED_TEXT);
		}
		SetFieldDec ( (IPT_CUR_INDEX+(index-2)), Test11CurrentDisplayData[EcuIndex].IPT[index] );
		setrgb (NORMAL_TEXT);
	}

	// set the remaining fields to 'blank'
	for ( ; index < INF_TYPE_IPT_NODI; index++)
	{
		SetFieldText ( OBD_MONITOR_COND_INDEX+index, "               " );
	}
}


/*
*******************************************************************************
** SaveSid9IptData
*******************************************************************************
*/
BOOL SaveSid9IptData (unsigned int EcuIndex, SID9IPT * pSid9Ipt)
{
	int index;
	BOOL rc = FALSE;


	// save Infotype and NODI
	Test11CurrentDisplayData[EcuIndex].INF = pSid9Ipt->INF;
	Test11CurrentDisplayData[EcuIndex].NODI = pSid9Ipt->NODI;

	// copy current counter values for active ECUs
	for ( index = 0; index < pSid9Ipt->NODI; index++ )
	{
		// note changes
		if ( Test11CurrentDisplayData[EcuIndex].IPT[index] != pSid9Ipt->IPT[index] )
		{
			// save current value of rate based counters
			Test11CurrentDisplayData[EcuIndex].IPT[index] = pSid9Ipt->IPT[index];

			rc = TRUE;
		}
	}

	return rc;
}


/*
*******************************************************************************
** UpdateSid9IptDisplay
*******************************************************************************
*/
void UpdateSid9IptDisplay ( unsigned int EcuIndex, unsigned int DoneFlags, SID9IPT * pSid9Ipt )
{
	int index;

	// update OBDCOND counter
	if ( (DoneFlags & 0x01) == 0 )
		setrgb (HIGHLIGHTED_TEXT);
	SetFieldDec ( OBD_MONITOR_COND_INDEX, Test11CurrentDisplayData[EcuIndex].IPT[0] );
	setrgb (NORMAL_TEXT);

	// update IGN counter
	if ( ((DoneFlags>>=1) & 0x01) == 0 )
		setrgb (HIGHLIGHTED_TEXT);
	SetFieldDec ( IGNITION_COUNTER_INDEX, Test11CurrentDisplayData[EcuIndex].IPT[1] );
	setrgb (NORMAL_TEXT);

	// update rate based counter values
	for ( index = 2; index < pSid9Ipt->NODI; index++ )
	{
		if ( ((DoneFlags>>=1) & 0x01) == 0 )
			setrgb (HIGHLIGHTED_TEXT);
		SetFieldDec ( (IPT_CUR_INDEX+(index-2)), pSid9Ipt->IPT[index] );
		setrgb (NORMAL_TEXT);
	}
}


/*
*******************************************************************************
** SaveSid1Pid1Data
*******************************************************************************
*/
BOOL SaveSid1Pid1Data (unsigned int EcuIndex)
{
	SID1 * pSid1;
	BOOL   rc = FALSE;

	pSid1 = (SID1 *)&gOBDResponse[EcuIndex].Sid1Pid[0];

	if (pSid1->PID == 1)
	{
		// note any differences
		if ((PreviousSid1Pid1[EcuIndex].Data[0] != pSid1->Data[0]) ||
		    (PreviousSid1Pid1[EcuIndex].Data[1] != pSid1->Data[1]) ||
		    (PreviousSid1Pid1[EcuIndex].Data[2] != pSid1->Data[2]) ||
		    (PreviousSid1Pid1[EcuIndex].Data[3] != pSid1->Data[3]) )
		{
			PreviousSid1Pid1[EcuIndex].Data[0] = pSid1->Data[0];
			PreviousSid1Pid1[EcuIndex].Data[1] = pSid1->Data[1];
			PreviousSid1Pid1[EcuIndex].Data[2] = pSid1->Data[2];
			PreviousSid1Pid1[EcuIndex].Data[3] = pSid1->Data[3];
			rc = TRUE;
		}

		// if Oxygen Sensor (spark) or Exhaust Gas Sensor (compression) supported
		if (pSid1->Data[2] & 1<<5)
		{
			// if completed
			if ( !(pSid1->Data[3] & 1<<5) )
			{
				Test11IMStatus[EcuIndex][0] = Complete;
			}
			// if disabled for this monitoring cycle
			else if ( !(Sid1Pid41[EcuIndex].Data[2] & 1<<5) )
			{
				Test11IMStatus[EcuIndex][0] = Disabled;
			}
			else
			{
				Test11IMStatus[EcuIndex][0] = Incomplete;
			}
		}
		else
		{
			Test11IMStatus[EcuIndex][0] = NotSupported;
		}

		// if Oxygen Sensor Heater (spark) or PM Filter (compression) supported
		if (pSid1->Data[2] & 1<<6)
		{
			// if completed
			if ( !(pSid1->Data[3] & 1<<6) )
			{
				Test11IMStatus[EcuIndex][1] = Complete;
				
			}
			// if disabled for this monitoring cycle
			else if ( !(Sid1Pid41[EcuIndex].Data[2] & 1<<6) )
			{
				Test11IMStatus[EcuIndex][1] = Disabled;
			}
			else
			{
				Test11IMStatus[EcuIndex][1] = Incomplete;
			}
		}
		else
		{
			Test11IMStatus[EcuIndex][1] = NotSupported;
		}

		// if Catalyst (spark) or NMHC Catalyst (compression) supported
		if (pSid1->Data[2] & 1<<0)
		{
			// if completed
			if ( !(pSid1->Data[3] & 1<<0) )
			{
				Test11IMStatus[EcuIndex][2] = Complete;
			}
			// if disabled for this monitoring cycle
			else if ( !(Sid1Pid41[EcuIndex].Data[2] & 1<<0) )
			{
				Test11IMStatus[EcuIndex][2] = Disabled;
			}
			else
			{
				Test11IMStatus[EcuIndex][2] = Incomplete;
			}
		}
		else
		{
			Test11IMStatus[EcuIndex][2] = NotSupported;
		}

		// if Heated Catalyst (spark) or NOx/SCR Aftertreatment (compression) supported
		if (pSid1->Data[2] & 1<<1)
		{
			// if completed
			if ( !(pSid1->Data[3] & 1<<1) )
			{
				Test11IMStatus[EcuIndex][3] = Complete;
			}
			// if disabled for this monitoring cycle
			else if ( !(Sid1Pid41[EcuIndex].Data[2] & 1<<1) )
			{
				Test11IMStatus[EcuIndex][3] = Disabled;
			}
			else
			{
				Test11IMStatus[EcuIndex][3] = Incomplete;
			}
		}
		else
		{
			Test11IMStatus[EcuIndex][3] = NotSupported;
		}

		// if A/C supported
		if (pSid1->Data[2] & 1<<4)
		{
			// if completed
			if ( !(pSid1->Data[3] & 1<<4) )
			{
				Test11IMStatus[EcuIndex][4] = Complete;
			}
			// if disabled for this monitoring cycle
			else if ( !(Sid1Pid41[EcuIndex].Data[2] & 1<<4) )
			{
				Test11IMStatus[EcuIndex][4] = Disabled;
			}
			else
			{
				Test11IMStatus[EcuIndex][4] = Incomplete;
			}
		}
		else
		{
			Test11IMStatus[EcuIndex][4] = NotSupported;
		}

		// if Evaportive Emissions (spark) supported
		if (pSid1->Data[2] & 1<<2)
		{
			// if completed
			if ( !(pSid1->Data[3] & 1<<2) )
			{
				Test11IMStatus[EcuIndex][5] = Complete;
			}
			// if disabled for this monitoring cycle
			else if ( !(Sid1Pid41[EcuIndex].Data[2] & 1<<2) )
			{
				Test11IMStatus[EcuIndex][5] = Disabled;
			}
			else
			{
				Test11IMStatus[EcuIndex][5] = Incomplete;
			}
		}
		else
		{
			Test11IMStatus[EcuIndex][5] = NotSupported;
		}

		// if EGR/VVT (spark and compression) supported
		if (pSid1->Data[2] & 1<<7)
		{
			// if completed
			if ( !(pSid1->Data[3] & 1<<7) )
			{
				Test11IMStatus[EcuIndex][6] = Complete;
			}
			// if disabled for this monitoring cycle
			else if ( !(Sid1Pid41[EcuIndex].Data[2] & 1<<7) )
			{
				Test11IMStatus[EcuIndex][6] = Disabled;
			}
			else
			{
				Test11IMStatus[EcuIndex][6] = Incomplete;
			}
		}
		else
		{
			Test11IMStatus[EcuIndex][6] = NotSupported;
		}

		// if Secondary Air (spark) or Boost Pressure (compression) supported
		if (pSid1->Data[2] & 1<<3)
		{
			// if completed
			if ( !(pSid1->Data[3] & 1<<3) )
			{
				Test11IMStatus[EcuIndex][7] = Complete;
			}
			// if disabled for this monitoring cycle
			else if ( !(Sid1Pid41[EcuIndex].Data[2] & 1<<3) )
			{
				Test11IMStatus[EcuIndex][7] = Disabled;
			}
			else
			{
				Test11IMStatus[EcuIndex][7] = Incomplete;
			}
		}
		else
		{
			Test11IMStatus[EcuIndex][7] = NotSupported;
		}

		// if Fuel Trim supported
		if (pSid1->Data[1] & 1<<1)
		{
			// if completed
			if ( !(pSid1->Data[1] & 1<<5) )
			{
				Test11IMStatus[EcuIndex][8] = Complete;
			}
			// if disabled for this monitoring cycle
			else if ( !(Sid1Pid41[EcuIndex].Data[1] & 1<<1) )
			{
				Test11IMStatus[EcuIndex][8] = Disabled;
			}
			else
			{
				Test11IMStatus[EcuIndex][8] = Incomplete;
			}
		}
		else
		{
			Test11IMStatus[EcuIndex][8] = NotSupported;
		}

		// if Misfire supported
		if (pSid1->Data[1] & 1<<0)
		{
			// if completed
			if ( !(pSid1->Data[1] & 1<<4) )
			{
				Test11IMStatus[EcuIndex][9] = Complete;
			}
			// if disabled for this monitoring cycle
			else if ( !(Sid1Pid41[EcuIndex].Data[1] & 1<<0) )
			{
				Test11IMStatus[EcuIndex][9] = Disabled;
			}
			else
			{
				Test11IMStatus[EcuIndex][9] = Incomplete;
			}
		}
		else
		{
			Test11IMStatus[EcuIndex][9] = NotSupported;
		}

		// if Comp / Comp supported
		if (pSid1->Data[1] & 1<<2)
		{
			Test11IMStatus[EcuIndex][10] = (pSid1->Data[1] & 1<<6) ? Incomplete : Complete;
		}
		else
		{
			Test11IMStatus[EcuIndex][10] = NotSupported;
		}
	}

	return rc;
}


/*
*******************************************************************************
** SaveSid1Pid41Data
*******************************************************************************
*/
BOOL SaveSid1Pid41Data ( void )
{
	unsigned int EcuIndex;
	SID_REQ   SidReq;
	SID1    * pSid1;
	BOOL      rc = FALSE;

	//-------------------------------------------
	// request SID 1 PID 41
	//-------------------------------------------
	SidReq.SID    = 1;
	SidReq.NumIds = 1;
	SidReq.Ids[0] = 0x41;

	if ( (rc = SidRequest (&SidReq, SID_REQ_NO_PERIODIC_DISABLE)) == PASS )
	{
		for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
		{
			if ( IsSid1PidSupported (EcuIndex, 0x41) )
			{
				if (gOBDResponse[EcuIndex].Sid1PidSize > 0)
				{
					pSid1 = (SID1 *)&gOBDResponse[EcuIndex].Sid1Pid[0];
					
					Sid1Pid41[EcuIndex].Data[0] = pSid1->Data[0];
					Sid1Pid41[EcuIndex].Data[1] = pSid1->Data[1];
					Sid1Pid41[EcuIndex].Data[2] = pSid1->Data[2];
					Sid1Pid41[EcuIndex].Data[3] = pSid1->Data[3];
				}
			}
			else
			{
				Sid1Pid41[EcuIndex].Data[0] = 0xFF;
				Sid1Pid41[EcuIndex].Data[1] = 0xFF;
				Sid1Pid41[EcuIndex].Data[2] = 0xFF;
				Sid1Pid41[EcuIndex].Data[3] = 0xFF;
			}
		}
	}
	else
	{
		for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
		{
			Sid1Pid41[EcuIndex].Data[0] = 0xFF;
			Sid1Pid41[EcuIndex].Data[1] = 0xFF;
			Sid1Pid41[EcuIndex].Data[2] = 0xFF;
			Sid1Pid41[EcuIndex].Data[3] = 0xFF;
		}
	}
	
	return rc;
}


/*
*******************************************************************************
** UpdateSid1Pid1Display
*******************************************************************************
*/
void UpdateSid1Pid1Display (unsigned int EcuIndex)
{
	int index;

	for (index = 0; index < 11; index++)
	{
		CurrentIMStatus[index] = Test11IMStatus[EcuIndex][index];
		if ( Test11IMStatus[EcuIndex][index] == Incomplete )
		{
			setrgb (HIGHLIGHTED_TEXT);
		}
		else if ( Test11IMStatus[EcuIndex][index] == Disabled )
		{
			setrgb (RED_TEXT);
		}
		SetFieldText (IM_READINESS_INDEX+index, szIM_Status[Test11IMStatus[EcuIndex][index]]);
		setrgb (NORMAL_TEXT);
	}
}

/*
*******************************************************************************
** RunDynamicTest11
*******************************************************************************
*/
STATUS RunDynamicTest11 ( BOOL bDisplayCARBTimers, unsigned long ulEngineStartTimeStamp)
{
	unsigned int  bLogMessage;
	unsigned long t1SecTimer, tDelayTimeStamp;
	unsigned int  EcuIndex, EcuMask, CurrentEcuIndex;
	unsigned int  DoneFlags;         // Bit Map of ITP items completed for current EcuIndex
	unsigned int  IMReadyDoneFlags;  // Bit Map of ECUs that have completed IM Readiness
	unsigned int  Sid9IptDoneFlags;  // Bit Map of ECUs that have completed IPT changes
	unsigned int  EcuDone;           // Bit Map of ECUs that have completed IM Readiness & IPT changes
	unsigned int  bSid9Ipt = 0;      // supported IPT data INF (8 or B), default 0 = no support
	int iTimeToCheckDTCs = 0;        // # of seconds until DTCs are checked

	const unsigned int EcuDoneMask = (1 << gUserNumEcus) - 1;

	SID_REQ       SidReq;
	SID1         *pSid1;

	unsigned char fInf8Supported = FALSE;
	unsigned char fInfBSupported = FALSE;

	unsigned char fSubTestStatus = SUB_TEST_NORMAL;
	unsigned char fLastTestStatus = SUB_TEST_INITIAL; // set to force an update the first time

	STATUS        mReturn = PASS;
	STATUS        ret_code = PASS;

	/* variables for CARB Timers */
	unsigned short usTestStartTime;  // time the test started (in seconds)
	unsigned short usTempTime = 0;
	unsigned short usIdleTime = 0;   // time at idle (in seconds, stops at 30)
	unsigned short usAtSpeedTime = 0;// time at speeds > 25mph (in seconds, stops at 300)
	unsigned short RPM = 0;          // engine RPM (used if RUNTM is not supported)
	unsigned short SpeedMPH = 0;     // vehicle speed in MPH
	unsigned short SpeedKPH = 0;     // vehicle speed in KPH
	unsigned short usRunTime = 0;    // engine run time (in seconds)
	unsigned int   bRunTimeSupport;  // 1 = RUNTM PID supported
	unsigned int   TestState = 0xFF;
	unsigned int   bDone = 0;
	unsigned int   bLoop;



	usTestStartTime = (unsigned short)(ulEngineStartTimeStamp / 1000);

	EcuDone          = 0;
	IMReadyDoneFlags = 0;
	Sid9IptDoneFlags = 0;


	// determine PID support
	bRunTimeSupport = IsSid1PidSupported (-1, 0x1F);

	if ((IsSid1PidSupported (-1, 0x0C) == FALSE) && (bRunTimeSupport == FALSE))
	{
		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $1 PIDs $0C (RPM) and $1F (RUNTM) not supported\n");
		bDisplayCARBTimers = FALSE;
	}

	// check for SID $9 INF support
	if ( IsSid9InfSupported (-1, 0x08) == TRUE )
	{
		fInf8Supported = TRUE;
	}

	if ( IsSid9InfSupported (-1, 0x0B) == TRUE )
	{
		fInfBSupported = TRUE;
	}

	if ( fInf8Supported == FALSE &&
	     gOBDDieselFlag == FALSE &&
	     gModelYear >= 2010 &&
	     ( gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD ||
	       gUserInput.eComplianceType == EOBD_WITH_IUMPR || gUserInput.eComplianceType == HD_EOBD) )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 INF $8 not supported (Required for MY 2010 and later Spark Ignition Vehicles)\n" );
	}
	else if ( fInfBSupported == FALSE &&
	          gOBDDieselFlag == TRUE &&
	          gModelYear >= 2010 &&
	          ( gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD ||
	            gUserInput.eComplianceType == EOBD_WITH_IUMPR || gUserInput.eComplianceType == HD_EOBD) )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 INF $B not supported (Required for MY 2010 and later Compression Ignition Vehicles)\n" );
	}

	// initialize static text elements based on INF support
	if ( fInf8Supported == TRUE && fInfBSupported == FALSE )
	{
		bSid9Ipt = 0x08;
		init_screen (_string_elements11_INF856, _num_string_elements11_INF856);
	}

	else if ( fInf8Supported == FALSE && fInfBSupported == TRUE )
	{
		bSid9Ipt = 0x0B;
		init_screen (_string_elements11_INFB36, _num_string_elements11_INFB36);
	}

	else if ( fInf8Supported == FALSE && fInfBSupported == FALSE )
	{
		// Verify that INF $8 or INF $B (IPT) is supported by at least one ECU for MY 2005 and later - OBD II
		// Verify that INF $8 or INF $B (IPT) is supported by at least one ECU - EOBD with IUMPR
		if ( ( (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) &&
		       gModelYear >= 2005 ) ||
		     (gUserInput.eComplianceType == EOBD_WITH_IUMPR || gUserInput.eComplianceType == HD_EOBD) )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $9 IPT (INF $8 or INF $B) not supported\n" );
		}
		else
		{
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $9 IPT (INF $8 or INF $B) not supported\n" );
		}

		if ( gOBDDieselFlag == FALSE )
		{
			init_screen (_string_elements11_INF856, _num_string_elements11_INF856);
		}
		else
		{
			init_screen (_string_elements11_INFB36, _num_string_elements11_INFB36);
		}
	}

	else if ( fInf8Supported == TRUE && fInfBSupported == TRUE )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 IPT (INF $8 and INF $B) both supported\n" );

		if ( gOBDDieselFlag == FALSE )
		{
			bSid9Ipt = 0x08;
			init_screen (_string_elements11_INF856, _num_string_elements11_INF856);
		}
		else
		{
			bSid9Ipt = 0x0B;
			init_screen (_string_elements11_INFB36, _num_string_elements11_INFB36);
		}
	}


	/* display CARB Timers, if required */
	if (bDisplayCARBTimers == TRUE)
	{
		short x, y;

		// add CARB Timer display text to the screen
		get_cursor_pos (&x, &y);
		place_screen_text (_string_elements_timers, _num_string_elements_timers);
		gotoxy (x, y);
		SetFieldDec (IDLE_TIMER, usIdleTime);
		SetFieldDec (SPEED_25_MPH_TIMER, usAtSpeedTime);
		SetFieldDec (TOTAL_DRIVE_TIMER, usRunTime);
	}


	// save SID 1 PID $41
	SaveSid1Pid41Data ( );


	for ( EcuIndex=0, EcuMask=1; EcuIndex < gUserNumEcus; EcuIndex++, EcuMask<<=1 )
	{
		SetFieldHex (ECU_ID_INDEX+EcuIndex, GetEcuId(EcuIndex));

		if (IsSid1PidSupported (EcuIndex, 1) == FALSE)
		{
			IMReadyDoneFlags |= EcuMask;
		}

		if ( IsSid9InfSupported (EcuIndex, 0x08) == FALSE &&
		     IsSid9InfSupported (EcuIndex, 0x0B) == FALSE)
		{
			Sid9IptDoneFlags |= EcuMask;

			if ( gOBDDieselFlag == FALSE )
			{
				Test11CurrentDisplayData[EcuIndex].NODI = 20;
				Test10_10_Sid9Ipt[EcuIndex].NODI = 20;
			}
			else
			{
				Test11CurrentDisplayData[EcuIndex].NODI = 18;
				Test10_10_Sid9Ipt[EcuIndex].NODI = 18;
			}
		}

		if ( (IMReadyDoneFlags & Sid9IptDoneFlags & EcuMask) != 0)
		{
			EcuDone |= EcuMask;
			SetFieldText (ECU_STATUS_INDEX+EcuIndex, "N/A");
		}
	}

	CurrentEcuIndex = 0;

	DoneFlags = 0xFFFFFFFF;   // by default all items completed
	EvaluateSid9Ipt ( CurrentEcuIndex, &DoneFlags, 5, FALSE );

	SelectECU ( CurrentEcuIndex, -1 );
	DisplayEcuData ( CurrentEcuIndex, DoneFlags );

	// flush the STDIN stream of any user input before loop
	clear_keyboard_buffer ();

	tDelayTimeStamp = t1SecTimer = GetTickCount ();

	gSuspendLogOutput = TRUE;
	gSuspendScreenOutput = TRUE;
	//-------------------------------------------
	// loop until test completes
	//-------------------------------------------
	for (;;)
	{
		//-------------------------------------------
		// request SID 1 PID 1
		//-------------------------------------------
		SidReq.SID    = 1;
		SidReq.NumIds = 1;
		SidReq.Ids[0] = 1;

		ret_code = SidRequest (&SidReq, SID_REQ_NO_PERIODIC_DISABLE);

		bLogMessage = FALSE;

		if (ret_code == FAIL)
		{
			Log( FAILURE, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
			     "SID $1 PID $1 request\n");
			DumpTransactionBuffer();
		}
		else
		{
			if (ret_code == ERRORS)
			{
				/* early/late message, note it and keep going */
				fSubTestStatus |= SUB_TEST_FAIL;
				bLogMessage = TRUE;
			}

			for (EcuIndex = 0, EcuMask=1; EcuIndex < gUserNumEcus; EcuIndex++, EcuMask<<=1)
			{
				if (gOBDResponse[EcuIndex].Sid1PidSize > 0)
				{
					if (
					     (SaveSid1Pid1Data (EcuIndex) == TRUE) ||
					     (fLastTestStatus == SUB_TEST_INITIAL)
					   )
					{
						bLogMessage = TRUE;

						pSid1 = (SID1 *)&gOBDResponse[EcuIndex].Sid1Pid[0];
						if ( IsIM_ReadinessComplete ( pSid1 ) == TRUE )
						{
							IMReadyDoneFlags |= EcuMask;
						}

						if ( EcuIndex == CurrentEcuIndex )
							UpdateSid1Pid1Display ( EcuIndex );
					}
				}
			}

			if (bLogMessage == TRUE)
			{
				LogLastTransaction();
			}
		}

		//-------------------------------------------
		// Get SID 9 IPT
		//-------------------------------------------
		if ( bSid9Ipt != 0 )
		{
			SidReq.SID    = 9;
			SidReq.NumIds = 1;
			SidReq.Ids[0] = bSid9Ipt;

			ret_code = SidRequest (&SidReq, SID_REQ_NO_PERIODIC_DISABLE);

			bLogMessage = FALSE;

			if (ret_code == FAIL)
			{
				Log( FAILURE, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
				     "SID $9 IPT request\n");
				DumpTransactionBuffer();
			}
			else
			{
				if (ret_code == ERRORS)
				{
					/* early/late message, note it and keep going */
					fSubTestStatus |= SUB_TEST_FAIL;
					bLogMessage = TRUE;
				}

				for ( EcuIndex = 0, EcuMask=1; EcuIndex < gUserNumEcus; EcuIndex++, EcuMask<<=1 )
				{
					if ( GetSid9IptData ( EcuIndex, &Test11_5_Sid9Ipt[EcuIndex] ) == PASS )
					{
						if (
						     (SaveSid9IptData ( EcuIndex, &Test11_5_Sid9Ipt[EcuIndex] ) == TRUE) ||
						     (fLastTestStatus == SUB_TEST_INITIAL)
						   )
						{
							bLogMessage = TRUE;
						}
					}
				}

				if (bLogMessage == TRUE)
				{
					// something changed, update the SID $09 display because EVAP from one ECU
					// could affect another ECU's display
					for ( EcuIndex = 0, EcuMask=1; EcuIndex < gUserNumEcus; EcuIndex++, EcuMask<<=1 )
					{
						DoneFlags = 0xFFFFFFFF;   // by default all items completed
						ret_code = EvaluateSid9Ipt ( EcuIndex, &DoneFlags, 5, FALSE );
						if ( (ret_code & FAIL) == PASS )
						{
							Sid9IptDoneFlags |= EcuMask;
							if ( (ret_code & ERRORS) == ERRORS )
								fSubTestStatus |= SUB_TEST_FAIL;
						}

						if ( EcuIndex == CurrentEcuIndex )
						{
							UpdateSid9IptDisplay ( EcuIndex, DoneFlags, &Test11_5_Sid9Ipt[EcuIndex] );
						}
					}

					LogLastTransaction();
				}
			}
		}



		if (bDisplayCARBTimers == TRUE)
		{
			//-------------------------------------------
			// request RPM - SID 1 PID $0C
			//-------------------------------------------
			if (bRunTimeSupport == FALSE)
			{
				SidReq.SID    = 1;
				SidReq.NumIds = 1;
				SidReq.Ids[0] = 0x0c;

				ret_code = SidRequest( &SidReq, SID_REQ_NO_PERIODIC_DISABLE );

				bLogMessage = FALSE;

				if (ret_code == FAIL)
				{
					Log( FAILURE, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
					     "SID $1 PID $0C request\n");
					DumpTransactionBuffer();
				}
				else
				{
					if (ret_code != PASS)
					{
						/* cover the case where a response was early/late */
						fSubTestStatus |= SUB_TEST_FAIL;
						bLogMessage = TRUE;
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
						bLogMessage = TRUE;
					}

					if (bLogMessage == TRUE)
					{
						LogLastTransaction();
					}
				}
			}

			//-------------------------------------------
			// request Speed - SID 1 PID $0D
			//-------------------------------------------
			SidReq.SID    = 1;
			SidReq.NumIds = 1;
			SidReq.Ids[0] = 0x0d;

			ret_code = SidRequest( &SidReq, SID_REQ_NO_PERIODIC_DISABLE );

			bLogMessage = FALSE;

			if (ret_code == FAIL)
			{
				Log( FAILURE, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
				     "SID $1 PID $0D request\n");
				DumpTransactionBuffer();
			}
			else
			{
				if (ret_code != PASS)
				{
					/* cover the case where a response was early/late */
					fSubTestStatus |= SUB_TEST_FAIL;
					bLogMessage = TRUE;
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
					bLogMessage = TRUE;
				}

				if (bLogMessage == TRUE)
				{
					LogLastTransaction();
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

				ret_code = SidRequest( &SidReq, SID_REQ_NO_PERIODIC_DISABLE );

				bLogMessage = FALSE;

				if (ret_code == FAIL)
				{
					Log( FAILURE, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
					     "SID $1 PID $1F request\n");
					DumpTransactionBuffer();
				}
				else
				{
					if (ret_code != PASS)
					{
						/* cover the case where a response was early/late */
						fSubTestStatus |= SUB_TEST_FAIL;
						bLogMessage = TRUE;
					}

					for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
					{
						if (gOBDResponse[EcuIndex].Sid1PidSize > 0)
						{
							pSid1 = (SID1 *)&gOBDResponse[EcuIndex].Sid1Pid[0];

							if (pSid1->PID == 0x1f)
							{
								usRunTime = pSid1->Data[0];
								usRunTime = usRunTime << 8 | pSid1->Data[1];    // 1 cnt = 1 sec
								break;
							}
						}
					}

					if (EcuIndex >= gUserNumEcus)
					{
						Log( FAILURE, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
						     "SID $1 PID $1F missing response\n");
						fSubTestStatus |= SUB_TEST_FAIL;
						bLogMessage = TRUE;
					}

					if (bLogMessage == TRUE)
					{
						LogLastTransaction();
					}
				}
			}
			else
			{
				usRunTime = (unsigned short)(GetTickCount () / 1000) - usTestStartTime;
			}


			SetFieldDec (TOTAL_DRIVE_TIMER, usRunTime);

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
						usTempTime = usRunTime;

						if ( (bRunTimeSupport == TRUE) ? (usRunTime > 0) : (RPM > 450) )
						{

							if (SpeedKPH <= 1)
							{
								TestState = 1;
								usIdleTime = usRunTime;
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
						if ( (bRunTimeSupport == TRUE) ? (usRunTime > 0) : (RPM > 450) )
						{
							// check 600 seconds cumulative time
							if ( ((bDone & CUMULATIVE_TIME) != CUMULATIVE_TIME) && (usRunTime >= 600) )
							{
								bDone |= CUMULATIVE_TIME;
								SaveTransactionStart(); // treat as a seperate transaction to avoid duplication
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "Idle Time = %d;  Speed Time = %d;  Run Time = %d  *RUN TIME DONE*\n",
								     usIdleTime, usAtSpeedTime, usRunTime);
								LogLastTransaction();
							}

							if ( (SpeedKPH <= 1) && (usIdleTime < 30) )
							{
								TestState = 1;
								usIdleTime = 0;
								bLoop = TRUE;
							}
							else if ( (SpeedKPH >= 40) && (usAtSpeedTime < 300) )
							{
								TestState = 2;
								bLoop = TRUE;
							}
							else if ( bDone == (IDLE_TIME | ATSPEED_TIME | CUMULATIVE_TIME) )
							{
								TestState = 3;
								bLoop = TRUE;
							}
							else
							{
								usTempTime = usRunTime;
							}
						}
						break;

					case 1:     // 30 seconds continuous time at idle
						if ((SpeedKPH <= 1) && ( (bRunTimeSupport == FALSE) ? (RPM > 450) : 1) )
						{
							// check idle time
							usIdleTime = min(usIdleTime + usRunTime - usTempTime, 30);
							usTempTime = usRunTime;

							SetFieldDec (IDLE_TIMER, usIdleTime);

							if (usIdleTime >= 30)
							{
								bDone |= IDLE_TIME;
								TestState = 0;
								bLoop = TRUE;
								SaveTransactionStart(); // treat as a seperate transaction to avoid duplication
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "Idle Time = %d;  Speed Time = %d;  Run Time = %d  *IDLE DONE*\n",
								     usIdleTime, usAtSpeedTime, usRunTime);
								LogLastTransaction();
							}

							// check 600 seconds cumulative time
							if ( ((bDone & CUMULATIVE_TIME) != CUMULATIVE_TIME) && (usRunTime >= 600) )
							{
								bDone |= CUMULATIVE_TIME;
								SaveTransactionStart(); // treat as a seperate transaction to avoid duplication
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "Idle Time = %d;  Speed Time = %d;  Run Time = %d  *RUN TIME DONE*\n",
								     usIdleTime, usAtSpeedTime, usRunTime);
								LogLastTransaction();
							}
						}
						else
						{
							TestState = 0;
							bLoop = TRUE;
						}
						break;

					case 2:     // 300 seconds cumulative time at SpeedKPH >= 40 KPH
						if (SpeedKPH >= 40 && ( (bRunTimeSupport == FALSE) ? (RPM > 450) : 1) )
						{
							// check at speed time
							usAtSpeedTime = min(usAtSpeedTime + usRunTime - usTempTime, 300);
							usTempTime = usRunTime;

							SetFieldDec (SPEED_25_MPH_TIMER, usAtSpeedTime);

							if (usAtSpeedTime >= 300)
							{
								bDone |= ATSPEED_TIME;
								TestState = 0;
								bLoop = TRUE;
								SaveTransactionStart(); // treat as a seperate transaction to avoid duplication
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "Idle Time = %d;  Speed Time = %d;  Run Time = %d  *AT SPEED DONE*\n",
								     usIdleTime, usAtSpeedTime, usRunTime);
								LogLastTransaction();
							}

							// check 600 seconds cumulative time
							if ( ((bDone & CUMULATIVE_TIME) != CUMULATIVE_TIME) && (usRunTime >= 600) )
							{
								bDone |= CUMULATIVE_TIME;
								SaveTransactionStart(); // treat as a seperate transaction to avoid duplication
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "Idle Time = %d;  Speed Time = %d;  Run Time = %d  *RUN TIME DONE*\n",
								     usIdleTime, usAtSpeedTime, usRunTime);
								LogLastTransaction();
							}
						}
						else
						{
							TestState = 0;
							bLoop = TRUE;
						}
						break;

					default:     // 'idle' and 'at speed' times done
						break;
				} // end switch (TestState)
			} // end while (bLoop)
		} // end if (bDisplayCARBTimers == TRUE)



		//-------------------------------------------
		// Check for DTCs
		//-------------------------------------------
		if (iTimeToCheckDTCs <= 0)
		{
			// initially clear the DTC flag
			fSubTestStatus &= (~SUB_TEST_DTC);

			ret_code = IsDTCPending((SID_REQ_NO_PERIODIC_DISABLE|SID_REQ_ALLOW_NO_RESPONSE));
			if ( ret_code == PASS )
			{
				// log and flag the pending DTCs
				LogLastTransaction();
				fSubTestStatus |= SUB_TEST_DTC;
			}
			else if ((ret_code) == ERRORS )
			{
				// log the error
				LogLastTransaction();
			}

			ret_code = IsDTCStored((SID_REQ_NO_PERIODIC_DISABLE|SID_REQ_ALLOW_NO_RESPONSE));
			if ( ret_code == PASS )
			{
				// log and flag the stored DTCs
				LogLastTransaction();
				fSubTestStatus |= SUB_TEST_DTC;
			}
			else if ((ret_code) == ERRORS )
			{
				// log the error
				LogLastTransaction();
			}

#ifdef _DEBUG
			iTimeToCheckDTCs = 2;
#else
			iTimeToCheckDTCs = 30;
#endif
		}


		// update on-screen test status
		if (gOBDTestSubsectionFailed)
		{
			// some function logged a FAILURE
			fSubTestStatus |= SUB_TEST_FAIL;
		}

		if (fSubTestStatus != fLastTestStatus)
		{
			if ((fSubTestStatus & SUB_TEST_FAIL) == SUB_TEST_FAIL)
			{
				setrgb (HIGHLIGHTED_TEXT);
				SetFieldText (IPT_TESTSTATUS_INDEX, "FAILURE");
				setrgb (NORMAL_TEXT);
			}
			else
			{
				SetFieldText (IPT_TESTSTATUS_INDEX, "Normal");
			}

			if ((fSubTestStatus & SUB_TEST_DTC) == SUB_TEST_DTC)
			{
				setrgb (HIGHLIGHTED_TEXT);
				SetFieldText (IPT_TESTSTATUS_INDEX + 1, "DTC Detected");
				setrgb (NORMAL_TEXT);
			}
			else
			{
				SetFieldText (IPT_TESTSTATUS_INDEX + 1, "             ");
			}

			fLastTestStatus = fSubTestStatus;
		}

		//-------------------------------------------
		// Check if test is complete
		//-------------------------------------------
		for (EcuIndex=0, EcuMask=1; EcuIndex < gUserNumEcus; EcuIndex++, EcuMask<<=1)
		{
			if ( (EcuDone & EcuMask) == 0)
			{
				if ( (IMReadyDoneFlags & Sid9IptDoneFlags & EcuMask) != 0)
				{
					EcuDone |= EcuMask;
					SetFieldText (ECU_STATUS_INDEX+EcuIndex, "Done");
				}
			}
		}

		if (EcuDone == EcuDoneMask)
		{
			if ( (fSubTestStatus & SUB_TEST_FAIL) == SUB_TEST_FAIL )
			{
				mReturn =  FAIL;
			}
			else
			{
				mReturn =  PASS;
				break;
			}
		}

		//-------------------------------------------
		// Check for num or ESC key, delay 1 second
		//-------------------------------------------
		do
		{
			if (_kbhit () != 0)
			{
				char c = _getch ();
				if (c == 27)                    // ESC key
				{
					gSuspendScreenOutput = FALSE;
					gSuspendLogOutput = FALSE;
					Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "Test 11 aborted by user\n\n");
					mReturn = ABORT;
				}

				if ((c == 'F') || (c == 'f'))   // "FAIL" key
				{
					gSuspendScreenOutput = FALSE;
					gSuspendLogOutput = FALSE;
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "Test 11 failed by user\n\n");
					mReturn = FAIL;
				}

				if (('1' <= c) && (c <= '8'))   // new ECU index
				{
					EcuIndex = c - '1';         // zero-based index
					if (EcuIndex < gUserNumEcus && EcuIndex != CurrentEcuIndex)
					{
						DoneFlags = 0xFFFFFFFF;   // by default all items completed
						EvaluateSid9Ipt ( EcuIndex, &DoneFlags, 5, FALSE );

						SelectECU ( EcuIndex, CurrentEcuIndex );
						DisplayEcuData ( EcuIndex, DoneFlags );
						CurrentEcuIndex = EcuIndex;
					}
				}
			}

			tDelayTimeStamp = GetTickCount ();

			Sleep ( min (1000 - (tDelayTimeStamp - t1SecTimer), 50) );

		} while (tDelayTimeStamp - t1SecTimer < 1000);

		t1SecTimer = tDelayTimeStamp;
		iTimeToCheckDTCs--;

		if ( mReturn != PASS )
		{
			break;
		}
	} // end for (;;)


	gSuspendScreenOutput = FALSE;
	gSuspendLogOutput = FALSE;
	PrintEcuData( IMReadyDoneFlags, Sid9IptDoneFlags, EcuDone, bSid9Ipt );

	if (bDisplayCARBTimers == TRUE)
	{
		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Idle Time = %d;  Speed Time = %d;  Run Time = %d\n",
		     usIdleTime, usAtSpeedTime, usRunTime);
	}

	if ( mReturn == ABORT )
	{
		ABORT_RETURN;
	}
	return mReturn;
}




/******************************************************************************
** PrintEcuData
******************************************************************************/
void PrintEcuData ( unsigned int  IMReadyDoneFlags,
                    unsigned int  Sid9IptDoneFlags,
                    unsigned int  EcuDone,
                    unsigned int  bSid9Ipt )
{
	unsigned int EcuIndex;
	unsigned int EcuMask;
	unsigned int index;
	unsigned int pindex;

	Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
	     "On exit from Test 11.2 the following were the conditions:\n");

	for ( EcuIndex = 0, EcuMask = 1; EcuIndex < gUserNumEcus; EcuIndex++, EcuMask <<= 1 )
	{
		Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
		     "ECU ID: %02X    Status: %s\n",
		     GetEcuId(EcuIndex),
		     (EcuDone & EcuMask) ? "Done" : "    " );


		// write IM Status
		pindex = 10;
		Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
		     "%s\n", _string_elements11_INF856[pindex++] );
		for ( index = 0; index < 11; index++, pindex++ )
		{
			if ( gOBDDieselFlag == TRUE )
			{
				Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
				     "%-25s  %s\n",
				     _string_elements11_INFB36[pindex].szLabel,
				     szIM_Status[Test11IMStatus[EcuIndex][index]] );
			}
			else
			{
				Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
				     "%-25s  %s\n",
				     _string_elements11_INF856[pindex].szLabel,
				     szIM_Status[Test11IMStatus[EcuIndex][index]] );
			}
		}

		if ( bSid9Ipt == 0 )
		{
			Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
			     "No IUMPR data because SID $9 IPT (INF $8 or INF $B) not supported.\n" );
		}
		else
		{
			// write rate based counter
			Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
			     "%-25s  Initial     Current\n", _string_elements11_INF856[pindex++] );
			for ( index = 0; index < 2; index++, pindex++ )
			{
				if ( gOBDDieselFlag == TRUE )
				{
					Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
					     "%-25s  %-8d    %-8d\n",
					     _string_elements11_INFB36[pindex].szLabel,
					     Test10_10_Sid9Ipt[EcuIndex].IPT[index],
					     Test11CurrentDisplayData[EcuIndex].IPT[index] );
				}
				else
				{
					Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
					     "%-25s  %-8d    %-8d\n",
					     _string_elements11_INF856[pindex].szLabel,
					     Test10_10_Sid9Ipt[EcuIndex].IPT[index],
					     Test11CurrentDisplayData[EcuIndex].IPT[index] );
				}
			}

			pindex += 6;
			Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
			     "%-25s  N    D      N    D\n", " ");
			for ( ; index < (unsigned)(Test11CurrentDisplayData[EcuIndex].NODI); index+=2, pindex++ )
			{
				if ( gOBDDieselFlag == TRUE )
				{
					Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
					     "%-25s  %-4d %-4d   %-4d %-4d\n",
					     _string_elements11_INFB36[pindex].szLabel,
					     Test10_10_Sid9Ipt[EcuIndex].IPT[index],
					     Test10_10_Sid9Ipt[EcuIndex].IPT[index+1],
					     Test11CurrentDisplayData[EcuIndex].IPT[index],
					     Test11CurrentDisplayData[EcuIndex].IPT[index+1] );
				}
				else
				{
					Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
					     "%-25s  %-4d %-4d   %-4d %-4d\n",
					     _string_elements11_INF856[pindex].szLabel,
					     Test10_10_Sid9Ipt[EcuIndex].IPT[index],
					     Test10_10_Sid9Ipt[EcuIndex].IPT[index+1],
					     Test11CurrentDisplayData[EcuIndex].IPT[index],
					     Test11CurrentDisplayData[EcuIndex].IPT[index+1] );
				}
			}
		}
	}
}
 