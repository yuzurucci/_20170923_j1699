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
** This computer program is based upon SAE Technical Report J1699,
** which is provided "AS IS"
**
********************************************************************************
*/

/*
** Typedefs and function pointers for J2534 API
*/
typedef long (CALLBACK* PTCONNECT)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long *);
typedef long (CALLBACK* PTDISCONNECT)(unsigned long);
typedef long (CALLBACK* PTREADMSGS)(unsigned long, void *, unsigned long *, unsigned long);
typedef long (CALLBACK* PTWRITEMSGS)(unsigned long, void *, unsigned long *, unsigned long);
typedef long (CALLBACK* PTSTARTPERIODICMSG)(unsigned long, void *, unsigned long *, unsigned long);
typedef long (CALLBACK* PTSTOPPERIODICMSG)(unsigned long, unsigned long);
typedef long (CALLBACK* PTSTARTMSGFILTER)(unsigned long, unsigned long, void *, void *, void *, unsigned long *);
typedef long (CALLBACK* PTSTOPMSGFILTER)(unsigned long, unsigned long);
typedef long (CALLBACK* PTSETPROGRAMMINGVOLTAGE)(unsigned long, unsigned long, unsigned long);
typedef long (CALLBACK* PTREADVERSION)(unsigned long, char *, char *, char *);
typedef long (CALLBACK* PTGETLASTERROR)(char *);
typedef long (CALLBACK* PTIOCTL)(unsigned long, unsigned long, void *, void *);

/*
** 01/04/06 : Update to J2534 API to 4.04
*/
typedef long (CALLBACK* PTOPEN)(void *, unsigned long *);
typedef long (CALLBACK* PTCLOSE)(unsigned long);

/*
** 06/24/04: Enhancement required to satisfy Prompt 2 for test cases 5.0 & 5.17.
*/
extern unsigned long gUserNumEcusReprgm;

extern unsigned long gRespTimeOutofRange;
extern unsigned long gRespTimeTooSoon;
extern unsigned long gRespTimeTooLate;
extern unsigned long gDetermineProtocol;

extern PTCONNECT PassThruConnect;
extern PTDISCONNECT PassThruDisconnect;
extern PTREADMSGS PassThruReadMsgs;
extern PTWRITEMSGS PassThruWriteMsgs;
extern PTSTARTPERIODICMSG PassThruStartPeriodicMsg;
extern PTSTOPPERIODICMSG PassThruStopPeriodicMsg;
extern PTSTARTMSGFILTER PassThruStartMsgFilter;
extern PTSTOPMSGFILTER PassThruStopMsgFilter;
extern PTSETPROGRAMMINGVOLTAGE PassThruSetProgrammingVoltage;
extern PTREADVERSION PassThruReadVersion;
extern PTGETLASTERROR PassThruGetLastError;
extern PTIOCTL PassThruIoctl;
extern PTOPEN  PassThruOpen;
extern PTCLOSE PassThruClose;


/* maximum number of J2534 devices allowed */
#define MAX_J2534_DEVICES        50

/* Battery voltage limits */
/* Below 11VDC, OBD monitors are not required to run */
#define OBD_BATTERY_MINIMUM      11000

/* Above 18VDC, J1978 scan tools are not required to run */
#define OBD_BATTERY_MAXIMUM      18000

/* Tester present message rate */
/* 4.9 seconds between tester present messages */
#define OBD_TESTER_PRESENT_RATE  2000

/* Time delay to allow for code clearing */
#define CLEAR_CODES_DELAY_MSEC   2000

/* Maximum number of OBD ECUs and protocols */
#define OBD_MAX_ECUS             8
#define OBD_MAX_US_PROTOCOLS     7
#define OBD_MAX_EU250K_PROTOCOLS 7
#define OBD_MAX_EU_PROTOCOLS     9
#define OBD_MAX_PROTOCOLS        OBD_MAX_EU_PROTOCOLS  /* the largest number of protocols possible */

/* OBD response indicator bit */
#define OBD_RESPONSE_BIT         0x40

/* Maximum number of bytes in ISO15765 frame */
#define ISO15765_MAX_BYTES_PER_FRAME 7

/* earliest allowable model year */
#define MIN_MODEL_YEAR           1981

/* Latest allowable model year */
#define MAX_MODEL_YEAR           2025

/* maximum length for a log filename */
#define MAX_LOGFILENAME          80
#define MAX_USERINPUT            255

/* maximum number of OBD types for SID $01 PID $1C */
#define MAX_OBD_TYPES            43

/* logging related maximums */
#define MAX_LOG_STRING_SIZE      2048   /* max. size of a single log entry */
#define MAX_MESSAGE_LOG_SIZE     640    /* max. number of message bytes in a single log entry */
#define DESIRED_DUMP_SIZE        8      /* desired number of transactions to be dumped from ring buffer */
#define MAX_TRANSACTION_COUNT    (DESIRED_DUMP_SIZE + 1)  /* max. number of transactions in ring buffer */
#define MAX_RING_BUFFER_SIZE     16384  /* max size of transaction ring buffer */


/* Function return value definitions (sometimes treated as bit map, DO NOT CHANGE VALUES!) */
typedef enum {PASS=0x00, FAIL=0x02, ABORT=0x04, ERRORS=0x08, RETRY=0x10, CTRL_C=0x20, EXIT=0x40} STATUS;

/* NAK Response ID and Codes */
#define NAK                      0x7F
#define NAK_GENERAL_REJECT       0x10
#define NAK_NOT_SUPPORTED        0x11
#define NAK_INVALID_FORMAT       0x12
#define NAK_REPEAT_REQUEST       0x21
#define NAK_SEQUENCE_ERROR       0x22
#define NAK_RESPONSE_PENDING     0x78

/* Tester node address */
#define TESTER_NODE_ADDRESS      0xF1

/* User prompt type definitions */
typedef enum { NO_PROMPT=0, ENTER_PROMPT=1, YES_NO_PROMPT=2, YES_NO_ALL_PROMPT=3, QUIT_CONTINUE_PROMPT=4,
               CUSTOM_PROMPT=5, COMMENT_PROMPT=6 } PROMPTTYPE;

/* User log entry type definitions */
typedef enum { USER_ERROR=1, FAILURE=2, WARNING=3, INFORMATION=4, PROMPT=5,
               SUBSECTION_BEGIN=6, SUBSECTION_PASSED_RESULT=7, SUBSECTION_FAILED_RESULT=8,
               SUBSECTION_INCOMPLETE_RESULT=9, SECTION_PASSED_RESULT=10, SECTION_FAILED_RESULT=11,
               SECTION_INCOMPLETE_RESULT=12, BLANK=13, NETWORK=14, ERROR_FAILURE=15, RESULTS=16,
               COMMENT=17, J2534_FAILURE=20 } LOGTYPE;

/* Screen Output type definitions */
typedef enum { SCREENOUTPUTON=TRUE, SCREENOUTPUTOFF=FALSE } SCREENOUTPUT;

/* Logfile Output type definitions */
typedef enum { LOGOUTPUTON=TRUE, LOGOUTPUTOFF=FALSE } LOGOUTPUT;

/* engine type definitions */
typedef enum { ENGINE_UNKNOWN=0, GASOLINE=1, DIESEL=2 } ENGINETYPE;

/* Powertrain type definitions */
typedef enum { PWRTRN_UNKNOWN=0, CONV=1, SS=2, HEV=3, PHEV=4 } PWRTRNTYPE;

/* vehicle type definitions */
typedef enum { VEHICLE_UNKNOWN=0, LD=1, MD=2, HD=3 } VEHICLETYPE;

/* Compliance test type definitions */
typedef enum { UNKNOWN=0, US_OBDII=1, EOBD_WITH_IUMPR=2, EOBD_NO_IUMPR=3, HD_OBD=4, HD_EOBD=5 , IOBD_NO_IUMPR=6, HD_IOBD_NO_IUMPR=7, OBDBr_NO_IUMPR=8} COMPLIANCETYPE;
typedef enum { USOBD=1, EOBD=2, EOBD_250K=3 } SCANTABLETYPE;


typedef enum {eTestNone=0, eTestNoDTC=5, eTestPendingDTC, eTestConfirmedDTC, eTestFaultRepaired,
              eTestNoFault3DriveCycle, eTestInUseCounters, eTestPerformanceCounters} TEST_PHASE;


// List of message defines - ORDER MUST MATCH g_rgpcDisplayStrings in j1699.c
#define DSPSTR_PRMPT_MODEL_YEAR         0
#define DSPSTR_PRMPT_OBD_ECU            1
#define DSPSTR_PRMPT_RPGM_ECU           2
#define DSPSTR_PRMPT_ENG_TYPE           3
#define DSPSTR_PRMPT_PWRTRN_TYPE        4
#define DSPSTR_STMT_COMPLIANCE_TYPE     5
#define DSPSTR_PRMPT_VEH_TYPE           6
#define DSPSTR_PRMPT_ISO15765_TYPE      7
#define DSPSTR_RSLT_TOT_USR_ERROR       8
#define DSPSTR_RSLT_TOT_J2534_FAIL      9
#define DSPSTR_RSLT_TOT_WARN            10
#define DSPSTR_RSLT_TOT_FAIL            11
#define DSPSTR_RSLT_TOT_COMMENTS        12
#define DSPSTR_TOTAL                    13




// Drive Cycle completion flags
#define IDLE_TIME           0x01
#define ATSPEED_TIME        0x02
#define CUMULATIVE_TIME     0x04
#define FEO_TIME            0x08



/* SID 9 InfoTypes */
#define INF_TYPE_VIN_COUNT       0x01
#define INF_TYPE_VIN             0x02
#define INF_TYPE_CALID_COUNT     0x03
#define INF_TYPE_CALID           0x04
#define INF_TYPE_CVN_COUNT       0x05
#define INF_TYPE_CVN             0x06
#define INF_TYPE_IPT_COUNT       0x07
#define INF_TYPE_IPT             0x08
#define INF_TYPE_ECUNAME_COUNT   0x09
#define INF_TYPE_ECUNAME         0x0A
#define INF_TYPE_IPD             0x0B
#define INF_TYPE_ESN_COUNT       0x0C
#define INF_TYPE_ESN             0x0D
#define INF_TYPE_EROTAN_COUNT    0x0E
#define INF_TYPE_EROTAN          0x0F
#define INF_TYPE_FEOCNTR         0x12
#define INF_TYPE_EVAP_DIST       0x14

#define INF_TYPE_IPT_NODI        28      // number of IPT data items

#define IPT_OBDCOND_INDEX       0   //INF8 & INFB
#define IPT_IGNCNTR_INDEX       1   //INF8 & INFB
#define IPT_CATCOMP1_INDEX      2   //INF8
#define IPT_HCCATCOMP_INDEX     2   //INFB
#define IPT_CATCOND1_INDEX      3   //INF8
#define IPT_HCCATCOND_INDEX     3   //INFB
#define IPT_CATCOMP2_INDEX      4   //INF8
#define IPT_NCATCOMP_INDEX      4   //INFB
#define IPT_CATCOND2_INDEX      5   //INF8
#define IPT_NCATCOND_INDEX      5   //INFB
#define IPT_O2SCOMP1_INDEX      6   //INF8
#define IPT_NADSCOMP_INDEX      6   //INFB
#define IPT_O2SCOND1_INDEX      7   //INF8
#define IPT_NADSCOND_INDEX      7   //INFB
#define IPT_O2SCOMP2_INDEX      8   //INF8
#define IPT_PMCOMP_INDEX        8   //INFB
#define IPT_O2SCOND2_INDEX      9   //INF8
#define IPT_PMCOND_INDEX        9   //INFB
#define IPT_INF8_EGRCOMP_INDEX  10  //INF8
#define IPT_EGSCOMP_INDEX       10  //INFB
#define IPT_INF8_EGRCOND_INDEX  11  //INF8
#define IPT_EGSCOND_INDEX       11  //INFB
#define IPT_AIRCOMP_INDEX       12  //INF8
#define IPT_INFB_EGRCOMP_INDEX  12  //INFB
#define IPT_AIRCOND_INDEX       13  //INF8
#define IPT_INFB_EGRCOND_INDEX  13  //INFB
#define IPT_EVAPCOMP_INDEX      14  //INF8
#define IPT_BPCOMP_INDEX        14  //INFB
#define IPT_EVAPCOND_INDEX      15  //INF8
#define IPT_BPCOND_INDEX        15  //INFB
#define IPT_SO2SCOMP1_INDEX     16  //INF8
#define IPT_FUELCOMP_INDEX      16  //INFB
#define IPT_SO2SCOND1_INDEX     17  //INF8
#define IPT_FUELCOND_INDEX      17  //INFB
#define IPT_SO2SCOMP2_INDEX     18  //INF8 NODI 20 & 28
#define IPT_SO2SCOND2_INDEX     19  //INF8 NODI 20 & 28
#define IPT_AFRICOMP1_INDEX     20  //INF8 NODI 28
#define IPT_AFRICOND1_INDEX     21  //INF8 NODI 28
#define IPT_AFRICOMP2_INDEX     22  //INF8 NODI 28
#define IPT_AFRICOND2_INDEX     23  //INF8 NODI 28
#define IPT_PFCOMP1_INDEX       24  //INF8 NODI 28
#define IPT_PFCOND1_INDEX       25  //INF8 NODI 28
#define IPT_PFCOMP2_INDEX       26  //INF8 NODI 28
#define IPT_PFCOND2_INDEX       27  //INF8 NODI 28


/* SidRequest Flags */
#define SID_REQ_NORMAL                      0x00000000
#define SID_REQ_RETURN_AFTER_ALL_RESPONSES  0x00000001
#define SID_REQ_NO_PERIODIC_DISABLE         0x00000002
#define SID_REQ_ALLOW_NO_RESPONSE           0x00000004
        // set = no response returns PASS from SidRequest
        // unset = no response returns FAIL from SidRequest
#define SID_REQ_IGNORE_NO_RESPONSE          0x00000008
        // set = log WARNING on no response in SidRequest
        // unset = don't log WARNING on no response in SidRequest


/* LogMsg Flags */
#define LOG_NORMAL_MSG          0
#define LOG_REQ_MSG             1

/* Macros */
#define ABORT_RETURN   {gOBDTestAborted = TRUE;gOBDTestSectionAborted = TRUE;return(ABORT);}



/*
 * ProtocolTag list - the order of this list must match ProtocolInitData[] in InitProtocolList.c
 */
#define J1850PWM_I              0   /* US OBD-II protocol */
#define J1850VPW_I              1   /* US OBD-II protocol */
#define ISO9141_I               2   /* US OBD-II protocol */
#define ISO14230_FAST_INIT_I    3   /* US OBD-II protocol */
#define ISO14230_I              4   /* US OBD-II protocol */
#define ISO15765_I              5   /* US OBD-II protocol */
#define ISO15765_29_BIT_I       6   /* US OBD-II protocol */
#define ISO15765_250K_11_BIT_I  7   /* EOBD protocol */
#define ISO15765_250K_29_BIT_I  8   /* EOBD protocol */

/* Protocol list structure */
typedef struct
{
	unsigned long Protocol;     // generic protocol (for J2534-1)
	unsigned long ChannelID;
	unsigned long InitFlags;    // J2534-1 ConnectFlags
	unsigned long TesterPresentID;
	unsigned long FilterID;
	unsigned long FlowFilterID[OBD_MAX_ECUS];
	unsigned long HeaderSize;   // size of message header
	unsigned long BaudRate;     // link data rate
	unsigned long ProtocolTag;  // tag that uniquely identifies each protocol 
	char Name[25];              // ASCII string for the protocol name
} PROTOCOL_LIST;

/* OBD message response data structures */
typedef struct
{
	unsigned char PID;
	unsigned char Data[41];
} SID1;

typedef struct
{
	unsigned char PID;
	unsigned char FrameNumber;
	unsigned char Data[41];
} SID2;

typedef struct
{
	unsigned char OBDMID;
	unsigned char SDTID;
	unsigned char UASID;
	unsigned char TVHI;
	unsigned char TVLO;
	unsigned char MINTLHI;
	unsigned char MINTLLO;
	unsigned char MAXTLHI;
	unsigned char MAXTLLO;
} SID6;

typedef struct
{
	unsigned char INF;
	unsigned char NumItems;
	unsigned char Data[4];
} SID9;


typedef struct
{
	unsigned char  INF;         // INF Type
	unsigned char  NODI;        // number data items
	unsigned short IPT[INF_TYPE_IPT_NODI];
	unsigned short Flags;       // application flags. set 1 when valid
} SID9IPT;

typedef struct
{
	unsigned char FirstID;
	unsigned char IDBits[4];
} ID_SUPPORT;

typedef struct
{
	unsigned char FirstID;
	unsigned char FrameNumber;
	unsigned char IDBits[4];
} FF_SUPPORT;

typedef struct
{
	unsigned char   Header[4];

	BOOL            bResponseReceived;  // used to check for multiple responses

	BOOL            LinkActive;

	unsigned char   Sid1PidSupportSize;
	ID_SUPPORT      Sid1PidSupport[8];
	unsigned short  Sid1PidSize;
	unsigned char   Sid1Pid[246];       // 6 * largest PID (81 - 41 bytes)

	unsigned char   Sid2PidSupportSize;
	FF_SUPPORT      Sid2PidSupport[8];
	unsigned short  Sid2PidSize;
	unsigned char   Sid2Pid[2048];

	BOOL            Sid3Supported;
	unsigned short  Sid3Size;           // number of bytes in array Sid3
	unsigned char   Sid3[2048];

	unsigned short  Sid4Size;
	unsigned char   Sid4[8];

	unsigned char   Sid5TidSupportSize;
	ID_SUPPORT      Sid5TidSupport[8];
	unsigned short  Sid5TidSize;
	unsigned char   Sid5Tid[2048];

	unsigned char   Sid6MidSupportSize;
	ID_SUPPORT      Sid6MidSupport[8];
	unsigned short  Sid6MidSize;
	unsigned char   Sid6Mid[2048];

	BOOL            Sid7Supported;
	unsigned short  Sid7Size;           // number of bytes in array Sid7
	unsigned char   Sid7[2048];

	unsigned char   Sid8TidSupportSize;
	ID_SUPPORT      Sid8TidSupport[8];
	unsigned short  Sid8TidSize;
	unsigned char   Sid8Tid[2048];

	unsigned char   Sid9InfSupportSize;
	ID_SUPPORT      Sid9InfSupport[8];
	unsigned short  Sid9InfSize;
	unsigned char   Sid9Inf[2048];

	BOOL            SidASupported;
	unsigned short  SidASize;           // number of bytes in array SidA
	unsigned char   SidA[2048];
} OBD_DATA;

typedef struct
{
	unsigned long   EcuId;                       //	ID of the ECU 
	BOOL            NAKReceived;
	unsigned long   ExtendResponseTimeMsecs;
	unsigned long   ResponsePendingDelay;
	unsigned long   AggregateResponseTimeMsecs;  // Total Accumulated Response Time for this ECU
	unsigned long   AggregateResponses;          // Total number of responses from this ECU
	unsigned long   LongestResponsesTime;        // Longest response time from this ECU
	unsigned long   RespTimeOutofRange;          // Count of response times out of range
	unsigned long   RespTimeTooLate;             // Count of response times too long
	unsigned long   RespTimeTooSoon;             // Count of response times too short
} ECU_TIMING_DATA;

/* Service ID (Mode) request structure */
typedef struct
{
	unsigned char SID;
	unsigned char NumIds;
	unsigned char Ids[8];
} SID_REQ;

typedef struct
{
	unsigned short Size;
	unsigned char  DTC[2048];
} DTC_LIST;


typedef struct
{
	ENGINETYPE     eEngineType;
	PWRTRNTYPE     ePwrTrnType;
	VEHICLETYPE    eVehicleType;
	COMPLIANCETYPE eComplianceType;     /* type of compliance being tested for */
	SCANTABLETYPE  eScanTable;          /* table of permutations used for OBD protocol scans */
	unsigned long  MaxProtocols;        /* maximum number of OBD protocols */
	char szNumDCToSetMIL[10];           /* count of drive cycles (either "two" or "three") with fault to set MIL */
} USER_INPUT;



/* Local function prototypes */
char * substring(char * str, const char * substr); /* returns loc. of 'substr' in 'str' */
void   clear_keyboard_buffer(void);


STATUS TestWithNoDtc(void);
STATUS TestWithPendingDtc(void);
STATUS TestWithConfirmedDtc(void);
STATUS TestWithFaultRepaired(void);
STATUS TestWithNoFaultsAfter3DriveCycles(void);
STATUS TestToVerifyInUseCounters(BOOL *pbReEnterTest);
STATUS TestToVerifyPerformanceCounters(BOOL *pbReEnterTest);
STATUS FindJ2534Interface(void);
STATUS DetermineProtocol(void);
STATUS CheckMILLight(void);
STATUS SidRequest(SID_REQ *, unsigned long);
STATUS SidResetResponseData(PASSTHRU_MSG *);
STATUS SidSaveResponseData(PASSTHRU_MSG *, SID_REQ *, unsigned long *);
STATUS ConnectProtocol(void);
STATUS DisconnectProtocol(void);
void   StopTest(STATUS ExitCode, TEST_PHASE eTestPhase);
void   AbortTest (void);
void   InitProtocolList(void);
STATUS IsDTCPending(unsigned long Flags);
STATUS IsDTCStored(unsigned long Flags);

void   LogMsg(PASSTHRU_MSG *, unsigned long);

char   Log( LOGTYPE LogType, SCREENOUTPUT ScreenOutput, LOGOUTPUT LogOutput, PROMPTTYPE PromptType, const char *LogString, ... );
void   SaveTransactionStart(void);        /* marks the start of a new transaction in the ring buffer */
void   AddToTransactionBuffer (char *pszStringToAdd);  /* adds a string to the transaction ring buffer */
void   LogLastTransaction(void);          /* copies last transaction from ring buffer to log file */
void   DumpTransactionBuffer(void);       /* copies transaction ring buffer to log file */
void   ClearTransactionBuffer(void);      /* clears transaction ring buffer */
unsigned long GetFailureCount (void);     /* returns global failure count */
STATUS VerifyLogFile(FILE *hFileHandle);  /* upon re-entering Dynamic test, verifies previous data */
void   LogStats (void);

STATUS ClearCodes(void);
STATUS VerifyMILData(void);
STATUS VerifyMonitorTestSupportAndResults(void);
STATUS VerifyReverseOrderSupport(void);
STATUS VerifyGroupDiagnosticSupport(void);
STATUS VerifyGroupMonitorTestSupport(void);
STATUS VerifyGroupFreezeFrameSupport(unsigned short *pFreezeFrameDTC);
STATUS VerifyGroupFreezeFrameResponse (unsigned short *pFreezeFrameDTC);
STATUS VerifyGroupControlSupport(void);
STATUS VerifyGroupVehicleInformationSupport(void);
STATUS VerifyDiagnosticSupportAndData( unsigned char OBDEngineDontCare );
STATUS VerifyDiagnosticBurstSupport(void);
STATUS VerifyFreezeFrameSupportAndData(void);
STATUS VerifyDTCStoredData(void);
STATUS VerifyDTCPendingData(void);
STATUS VerifyO2TestResults(void);
STATUS VerifyControlSupportAndData(void);
STATUS VerifyVehicleInformationSupportAndData(void);
STATUS VerifyLinkActive(void);
STATUS VerifyReverseGroupDiagnosticSupport(void);
STATUS VerifyIM_Ready (void);
STATUS VerifyINF8Data ( unsigned long  EcuIndex );
STATUS VerifyINFBData ( unsigned long  EcuIndex );
STATUS VerifyPermanentCodeSupport(void);
STATUS TestToVerifyPermanentCodes(void);
STATUS VerifyReservedServices(void);
STATUS VerifyVehicleState(unsigned char ucEngineRunning, unsigned char ucHybrid);

STATUS VerifyEcuID (unsigned char EcuId[]);
STATUS LogSid9Ipt (void);

STATUS AppendLogFile (void);
STATUS VerifyVINFormat (unsigned long EcuIndex);

STATUS RequestSID1SupportData (void);
unsigned int IsSid1PidSupported (unsigned int EcuIndex, unsigned int PidIndex);
int    VerifySid1PidSupportData(void);

STATUS RequestSID9SupportData (void);
STATUS GetSid9IptData (unsigned int EcuIndex, SID9IPT * pSid9Ipt);
unsigned int IsSid9InfSupported (unsigned int EcuIndex, unsigned int InfIndex);

unsigned int GetEcuId (unsigned int EcuIndex);

STATUS StartPeriodicMsg (void);
STATUS StopPeriodicMsg (BOOL bLogError);

STATUS DetermineVariablePidSize (void);
void   SaveDTCList (int nSID);

BOOL   ReadSid9IptFromLogFile ( const char * szTestSectionStart,
                                const char * szTestSectionEnd,
                                SID9IPT Sid9Ipt[]);

unsigned int IsSid2PidSupported (unsigned int EcuIndex, unsigned int PidIndex);
unsigned int IsSid6MidSupported (unsigned int EcuIndex, unsigned int MidIndex);
unsigned int IsSid8TidSupported (unsigned int EcuIndex, unsigned int TidIndex);

STATUS LookupEcuIndex (PASSTHRU_MSG *RxMsg, unsigned long *pulEcuIndex);

void   LogVersionInformation (SCREENOUTPUT bDisplay, LOGOUTPUT bLog);
void   LogSoftwareVersion (SCREENOUTPUT bDisplay, LOGOUTPUT bLog);
STATUS LogRPM(unsigned short *pusRPM);

STATUS VerifyCALIDFormat ( unsigned long EcuIndex, unsigned long Inf3NumItems );
STATUS VerifyCVNFormat ( unsigned long EcuIndex, unsigned long Inf5NumItems );


/* OBD type definitions */
extern const char *OBD_TYPE[MAX_OBD_TYPES];

/* Global variables */
extern char gLogFileName[MAX_LOGFILENAME];
extern char gUserModelYear[80];                 // string version of model year entered by the user
extern int  gModelYear;                         // integer version of model year entered by the user
extern char gUserMake[80];                      // make of vehicle entered by the user
extern char gUserModel[80];                     // model of vehicle entered by the user
extern unsigned long gUserNumEcus;              // number of ECUs set by the user

extern USER_INPUT gUserInput;                   // structure for user selected compliance test information
extern char *gEngineTypeStrings[3];             // strings that represent the user selected engine type
extern char *gPwrTrnTypeStrings[5];             // strings that represent the user selected powertrain type
extern char *gVehicleTypeStrings[4];            // strings that represent the user selected vehicle type
extern char *gComplianceTestTypeStrings[9];     // strings that represent the user selected compliance test
extern char *g_rgpcDisplayStrings[DSPSTR_TOTAL]; // strings used for display/logging

extern unsigned char gOBDKeywords[2];

extern unsigned char gOBDTestAborted;           // set if any test was aborted,
                                                // clear only before running Static and Dynamic tests

extern unsigned char gOBDTestSectionAborted;    // set if a test section (Static/Dynamic) was aborted,
                                                // clear only before the Static test and before Dynamic tests

extern unsigned char gOBDTestFailed;            // set if any test failed,
                                                // clear only before running Static and Dynamic tests

extern unsigned char gOBDTestSectionFailed;     // set if a test (static/dynamic) failed,
                                                // clear only at the beginning of the Static or Dynamic test

extern unsigned char gOBDTestSubsectionFailed;  // set if a test subsection failed,
                                                // clear before each subsection (ie test 5.10, test 7.4, etc)


extern unsigned char gOBDDetermined;            // set if a OBD protocol found
extern unsigned long gOBDRequestDelay;
extern unsigned long gOBDMaxResponseTimeMsecs;
extern unsigned long gOBDMinResponseTimeMsecs;  // min response time
extern unsigned long gOBDListIndex;             // index of the OBD protocol being used
extern unsigned long gOBDFoundIndex;            // index of OBD protocol found
extern PROTOCOL_LIST gOBDList[OBD_MAX_PROTOCOLS];
extern unsigned long gOBDNumEcus;               // number of responding ECUs
extern unsigned long gOBDNumEcusResp;           // number of responding ECUs
extern unsigned char gOBDEngineRunning;         // set if engine should be running
extern unsigned char gOBDEngineWarm;            // set if engine should be warm
extern unsigned char gOBDDTCPending;            // set if a DTC should be pending
extern unsigned char gOBDDTCStored;             // set if a DTC should be stored
extern unsigned char gOBDDTCHistorical;
extern unsigned char gOBDDTCPermanent;          // set if a DTC should be permanent
extern unsigned char gOBDDieselFlag;            // set by the user if this is a compression ignition (diesel) vehicle
extern unsigned char gOBDHybridFlag;            // set by the user if this is a hybrid vehicle
extern unsigned char gOBDPlugInFlag;
extern unsigned char gOBDDynoCertFlag;
extern unsigned long gOBDProtocolOrder;
extern unsigned long gOBDMonitorCount;
extern unsigned long gOBDAggregateResponseTimeMsecs;
extern unsigned long gOBDAggregateResponses;
extern unsigned long gLastLogTime;
extern unsigned char gIgnoreUnsupported;
extern unsigned char gSuspendScreenOutput;
extern unsigned char gSuspendLogOutput;
extern OBD_DATA gOBDResponse[OBD_MAX_ECUS];
extern OBD_DATA gOBDCompareResponse[OBD_MAX_ECUS];
extern char gVIN[18];
extern FILE *ghLogFile;
extern PASSTHRU_MSG  gTesterPresentMsg;
extern unsigned char gService0ASupported;           // count of ECUs supporting SID $A
extern unsigned int  gSID0ASupECU[OBD_MAX_ECUS];
extern BOOL gVerifyLink;                            // set if in VerifyLinkActive function
extern unsigned char gReverseOrderState[OBD_MAX_ECUS];   // state of reverse order request
#define NOT_REVERSE_ORDER       0 // not reverse order
#define REVERSE_ORDER_REQUESTED 1 // reverse order requested, no response yet
#define REVERSE_ORDER_RESPONSE  2 // first reverse order response received, SupportSize cleared


extern HINSTANCE hDLL;


extern long gSid1VariablePidSize;

char *gBanner;
extern const char gszAPP_REVISION[];

extern unsigned long gOBDNumEcusCan;               /* by Honda */
extern unsigned char gOBDResponseTA[OBD_MAX_ECUS]; /* by Honda */

extern FILE *ghTempLogFile;
extern char gszTempLogFilename[MAX_PATH];

extern DTC_LIST gDTCList[OBD_MAX_ECUS];

extern unsigned long gulDeviceID;       // global storage for J2534-1 Device ID

extern SID1 Sid1Pid1[OBD_MAX_ECUS];     // capture the response from SID01 PID01 response.

extern TEST_PHASE  TestPhase;
extern unsigned char TestSubsection;    // test subsection to be used in conjunction with TestPhase

extern ECU_TIMING_DATA gEcuTimingData[OBD_MAX_ECUS];

/*********************************************/
/* Option to turn off Tester Present Message */
extern BOOL gPeriodicMsgEnabled;
/*********************************************/
