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
** 04/26/04      Commented out logic to check SID9 INF8.  It is not specified to
**               be cleared via mode $04 in ISO15031-5.
** 05/01/04      Altered logic to flag non-support of INFOTYPE $08 as
**               failure.  OEM must present phase-in plan to CARB if
**               unsupported.
** 05/11/04      Added logic to verify J1699 Spec 11.5 test case #5.17.1
**               Verify ECU support of INFOTYPE $04.
** 05/11/04      Added logic to verify J1699 Spec 11.5 test case #5.17.1
**               Verify ECU support of INFOTYPE $06.
********************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "j2534.h"
#include "j1699.h"

/* Function Prototypes */
STATUS VerifyVINFormat (unsigned long  EcuIndex);
STATUS VerifyECUNameFormat (unsigned long  EcuIndex);
STATUS VerifyInfDAndFFormat (unsigned long  EcuIndex);
STATUS VerifyInf12Data (unsigned long  EcuIndex, unsigned long IGNCNTR);
STATUS VerifyInf14Data (unsigned long  EcuIndex);
int    VerifySid9PidSupportData (void);

/*
*******************************************************************************
** VerifyVehicleInformationSupportAndData -
** Function to verify SID9 vehicle info support and data
*******************************************************************************
*/
//*****************************************************************************
//
//
//
//*****************************************************************************
//	DATE        Modification
//	07/16/03   SF#760692:   Mode 9 model year modifier.
//	                        Eleminated check for gVIN[9] for determination
//	                        of vehicle model year with respect to INFOTYPE
//	                        2, 4, or 6 determination.
//*****************************************************************************
STATUS VerifyVehicleInformationSupportAndData (void)
{
	unsigned long  EcuIndex;     // ECU Index
	unsigned long  EcuLoopIndex; // nested ECU Index
	unsigned long  IdIndex;      // INF ID index
	SID_REQ        SidReq;
	SID9          *pSid9;
	unsigned long  SidIndex;     // multi-part message index

	unsigned long  Inf3NumItems[OBD_MAX_ECUS];
	unsigned long  Inf5NumItems[OBD_MAX_ECUS];
	unsigned long  Inf7NumItems[OBD_MAX_ECUS];
	unsigned long  Inf9NumItems[OBD_MAX_ECUS];
	unsigned long  InfCNumItems[OBD_MAX_ECUS];
	unsigned long  InfENumItems[OBD_MAX_ECUS];

	unsigned long  Inf2NumResponses = 0;         // number of INF $02 responses
	unsigned char  fInf4Responded[OBD_MAX_ECUS]; // per ECU INF $4 response indicator
	unsigned char  Inf6NumResponses = 0;         // number of INF $06 responses
	unsigned char  Inf8NumResponses = 0;         // number of INF $08 responses
	unsigned char  fInfAResponded[OBD_MAX_ECUS]; // per ECU INF $A response indicator
	unsigned char  InfBNumResponses = 0;         // number of INF $0B responses
	unsigned char  InfDNumResponses = 0;         // number of INF $0D responses

	unsigned long  INF2SupportCount = 0;         // number of ECUs supporting INF $2
	unsigned char  INF2Supported[OBD_MAX_ECUS];  // per ECU support INF $2 indicator
	unsigned long  INF4SupportCount = 0;         // number of ECUs supporting INF $4
	unsigned long  INF6SupportCount = 0;         // number of ECUs supporting INF $6
	unsigned long  INF8SupportCount = 0;         // number of ECUs supporting INF $8
	unsigned long  INFASupportCount = 0;         // number of ECUs supporting INF $A
	unsigned long  INFBSupportCount = 0;         // number of ECUs supporting INF $B
	unsigned long  INFDSupportCount = 0;         // number of ECUs supporting INF $D
	unsigned long  INF12SupportCount = 0;        // number of ECUs supporting INF $D
	unsigned long  INF14SupportCount = 0;        // number of ECUs supporting INF $D

	BOOL bTestFailed = FALSE;

	unsigned long ulInit_FailureCount = 0;

	unsigned long IGNCNTR;


	ulInit_FailureCount = GetFailureCount();

	/* Request SID 9 support data */
	if (RequestSID9SupportData () != PASS)
	{
		return FAIL;
	}


	/*
	** Determine number of ECUs supporting required INFs
	*/
	for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
	{
		/* initialize the array values for this EcuIndex before use */
		Inf3NumItems[EcuIndex] = 0;
		Inf5NumItems[EcuIndex] = 0;
		Inf7NumItems[EcuIndex] = 0;
		Inf9NumItems[EcuIndex] = 0;
		InfCNumItems[EcuIndex] = 0;
		InfENumItems[EcuIndex] = 0;
		fInf4Responded[EcuIndex] = FALSE;
		fInfAResponded[EcuIndex] = FALSE;
		INF2Supported[EcuIndex] = FALSE;


		if (IsSid9InfSupported (EcuIndex, 0x02) == TRUE)
		{
			INF2SupportCount++;
			INF2Supported[EcuIndex] = TRUE;
		}

		if (IsSid9InfSupported (EcuIndex, 0x04) == TRUE)
		{
			INF4SupportCount++;
		}

		if (IsSid9InfSupported (EcuIndex, 0x06) == TRUE)
		{
			INF6SupportCount++;
		}

		if (IsSid9InfSupported (EcuIndex, 0x08) == TRUE)
		{
			// Spark Ignition IPT support
			INF8SupportCount++;

			// if ECU does not only supports CCM requirements (SID 1 PID 1 Data B bit 2==1)
			if ( (Sid1Pid1[EcuIndex].Data[1] & 0x03) != 0x00 ||
			     (Sid1Pid1[EcuIndex].Data[2] & 0xFF) != 0x00 )
			{
				// Spark Ignition operator selection
				if ( gOBDDieselFlag == FALSE )
				{
					// Compression Ignition SID1 PID1 DATA_B Bit_3
					if ( (Sid1Pid1[EcuIndex].Data[1] & 0x08) != 0 )
					{
						if ( gModelYear >= 2010 )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $1 PID $1 DATA_B Bit_3 does not match Operator Selected Ignition Type (Required for MY 2010 and later vehicles)\n", GetEcuId(EcuIndex) );
							bTestFailed = TRUE;
						}
						else
						{
							Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $1 PID $1 DATA_B Bit_3 does not match Operator Selected Ignition Type (Required for MY 2010 and later vehicles)\n", GetEcuId(EcuIndex) );
						}
					}
				}
				// Compression Ignition operator selection
				else
				{
					if ( gModelYear >= 2010 )
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  SID $9 IPT (INF8) support does not match Operator Selected Ignition Type (Required for MY 2010 and later vehicles)\n", GetEcuId(EcuIndex) );
						bTestFailed = TRUE;
					}
					else
					{
						Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  SID $9 IPT (INF8) support does not match Operator Selected Ignition Type (Required for MY 2010 and later vehicles)\n", GetEcuId(EcuIndex) );
					}

					// Spark Ignition SID1 PID1 DATA_B Bit_3
					if ( (Sid1Pid1[EcuIndex].Data[1] & 0x08) == 0 )
					{
						if ( gModelYear >= 2010 )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $1 PID $1 DATA_B Bit_3 does not match Operator Selected Ignition Type (Required for MY 2010 and later vehicles)\n", GetEcuId(EcuIndex) );
							bTestFailed = TRUE;
						}
						else
						{
							Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $1 PID $1 DATA_B Bit_3 does not match Operator Selected Ignition Type (Required for MY 2010 and later vehicles)\n", GetEcuId(EcuIndex) );
						}
					}
				}
			} // end if ECU does not only supports CCM requirements (SID 1 PID 1 Data B bit 2==1)
		} // end if (IsSid9InfSupported (EcuIndex, 0x08) == TRUE)

		if (IsSid9InfSupported (EcuIndex, 0x0A) == TRUE)
		{
			INFASupportCount++;
		}
		else
		{
			// Verify that INF $A is supported for MY 2010 and later
			if ( gModelYear >= 2010 &&
			     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  SID $9 INF $A not supported! (Required for MY 2010 and later vehicles)\n", GetEcuId(EcuIndex) );
				bTestFailed = TRUE;
			}
		}

		if (IsSid9InfSupported (EcuIndex, 0x0B) == TRUE)
		{
			// Compression Ignition IPT support
			INFBSupportCount++;

			// if ECU does not only supports CCM requirements (SID 1 PID 1 Data B bit 2==1)
			if ( (Sid1Pid1[EcuIndex].Data[1] & 0x03) != 0x00 ||
			     (Sid1Pid1[EcuIndex].Data[2] & 0xFF) != 0x00 )
			{
				// Compression Ignition operator selection
				if ( gOBDDieselFlag == TRUE )
				{
					// Compression Ignition SID1 PID1 DATA_B Bit_3
					if ( (Sid1Pid1[EcuIndex].Data[1] & 0x08) == 0 )
					{
						if ( gModelYear >= 2010 )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $1 PID $1 DATA_B Bit_3 does not match Operator Selected Ignition Type (Required for MY 2010 and later vehicles)\n", GetEcuId(EcuIndex) );
							bTestFailed = TRUE;
						}
						else
						{
							Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $1 PID $1 DATA_B Bit_3 does not match Operator Selected Ignition Type (Required for MY 2010 and later vehicles)\n", GetEcuId(EcuIndex) );
						}
					}
				}
				// Spark Ignition operator selection
				else
				{
					if ( gModelYear >= 2010 )
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  SID $9 IPT (INFB) support does not match Operator Selected Ignition Type (Required for MY 2010 and later vehicles)\n", GetEcuId(EcuIndex) );
						bTestFailed = TRUE;
					}
					else
					{
						Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  SID $9 IPT (INFB) support does not match Operator Selected Ignition Type (Required for MY 2010 and later vehicles)\n", GetEcuId(EcuIndex) );
					}

					// Compression Ignition SID1 PID1 DATA_B Bit_3
					if ( (Sid1Pid1[EcuIndex].Data[1] & 0x08) != 0 )
					{
						if ( gModelYear >= 2010 )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID1 PID1 DATA_B Bit_3 does not match Operator Selected Ignition Type (Required for MY 2010 and later vehicles)\n", GetEcuId(EcuIndex) );
							bTestFailed = TRUE;
						}
						else
						{
							Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID1 PID1 DATA_B Bit_3 does not match Operator Selected Ignition Type (Required for MY 2010 and later vehicles)\n", GetEcuId(EcuIndex) );
						}
					}
				} // end gOBDDieselFlag == FALSE
			} // end if ECU does not only supports CCM requirements (SID 1 PID 1 Data B bit 2==1)
		} // end if (IsSid9InfSupported (EcuIndex, 0x0B) == TRUE)

		if (IsSid9InfSupported (EcuIndex, 0x0D) == TRUE)
		{
			INFDSupportCount++;
		}

		if (IsSid9InfSupported (EcuIndex, 0x12) == TRUE)
		{
			INF12SupportCount++;
		}

		if (IsSid9InfSupported (EcuIndex, 0x14) == TRUE)
		{
			INF14SupportCount++;
		}
	} // end for ( EcuIndex


	// Verify that one and only one ECU supports INF $2 (VIN) for US OBD II and HD EOBD
	if ( INF2SupportCount == 0 )
	{
		if ( gUserInput.eComplianceType == US_OBDII ||
		     gUserInput.eComplianceType == HD_OBD   ||
		     gUserInput.eComplianceType == HD_EOBD )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $9 INF $2 (VIN) not supported by any ECUs\n" );
		}
		else
		{
			Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $9 INF $2 (VIN) not supported by any ECUs\n" );
		}
	}
	else if ( INF2SupportCount > 1 )
	{
		for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
		{
			if ( INF2Supported[EcuIndex] == TRUE )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "SID $9 INF $2 (VIN) supported by multiple ECUs ( ECU %X )\n", GetEcuId(EcuIndex) );
			}
		}
		bTestFailed = TRUE;
	}

	// Verify that INF $4 (CALID) is supported by the expected number of ECUs (from user prompt)
	if ( INF4SupportCount != gUserNumEcus )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 INF $4 (CALID) not supported by expected number of ECUs ( %d support, %d expected)!\n",
		     INF4SupportCount, gUserNumEcus );
		bTestFailed = TRUE;
	}

	// Verify that INF $6 (CVN) is supported by at least the number of reprogrammable ECUs (from user prompt)
	if ( (INF6SupportCount < gUserNumEcusReprgm) &&
	     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 INF $6 (CVN) supported by less than the expected number of ECUs ( %d support, %d expected)!\n",
		     INF6SupportCount, gUserNumEcus );
		bTestFailed = TRUE;
	}

	// Verify that INF $8 and INF $B are not both supported
	if ( INF8SupportCount != 0 && INFBSupportCount != 0 )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Both SID $9 INF $8 and INF $B are supported!\n");
		bTestFailed = TRUE;
	}

	if ( INF8SupportCount == 0 && INFBSupportCount == 0 )
	{
		// Verify that INF $8 or INF $B (IPT) is supported by at least one ECU for MY 2007 and later - OBD II
		if ( gModelYear >= 2007 &&
		     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Neither SID $9 INF $8 nor INF $B is supported! (Support for one required for MY 2007 and later vehicles)\n");
			bTestFailed = TRUE;
		}
		// Verify that INF $8 or INF $B (IPT) is supported by at least one ECU - EOBD with IUMPR or HD_EOB
		else if ( gUserInput.eComplianceType == EOBD_WITH_IUMPR || gUserInput.eComplianceType == HD_EOBD )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Neither SID $9 INF $8 nor INF $B is supported!\n");
			bTestFailed = TRUE;
		}
	}

	// Verify that INF $A is supported for MY 2010 and later
	if ( gModelYear >= 2010 &&
	     INFASupportCount != gUserNumEcus &&
	     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 INF $A (ECU_NAME) is not supported by all ECUs! (Required for MY 2010 and later vehicles)\n");
		bTestFailed = TRUE;
	}

	if ( (INFDSupportCount == 0 || INFDSupportCount > 1) &&
	     gModelYear >= 2013 &&
	     gUserInput.eComplianceType == HD_OBD )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 INF $D (ESN) is not supported by one and only one ECU! (Required for MY 2013 and later Heavy Duty vehicles)\n");
		bTestFailed = TRUE;
	}

	// Verify that INF $12 is supported for MY 2014 and later PHEV OBDII vehicles
	if ( INF12SupportCount == 0 &&
	     gUserInput.eComplianceType == US_OBDII &&
	     gModelYear >= 2014 &&
	     gUserInput.ePwrTrnType == PHEV )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 INF $12 (FEOCNTR) is not supported by any ECU! (Required for MY 2014 and later PHEV vehicles)\n");
		bTestFailed = TRUE;
	}

	// Verify that INF $14 is supported for MY 2017 and later OBDII vehicles
	if ( INF14SupportCount == 0 &&
	     gUserInput.eComplianceType == US_OBDII &&
	     gOBDDieselFlag == FALSE &&
	     gModelYear >= 2017 )
	{
		Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 INF $14 (EVAP_DIST) is not supported by any ECU! (Required for vehicles that meet the LEV III / Tier 3 EVAP requirements)\n");
	}


	/*
	** For each INF group
	** Verify that all SID 9 INF data is valid
	** Request user supplied expected OBD ECU responses
	*/
	for ( IdIndex = 0x01; IdIndex <= 0x14; IdIndex++ )
	{
		/* If INF is supported by any ECU, request it */
		if ( IsSid9InfSupported (-1, IdIndex) == TRUE  )
		{
			SidReq.SID    = 9;
			SidReq.NumIds = 1;
			SidReq.Ids[0] = (unsigned char)IdIndex;

			if ( SidRequest( &SidReq, SID_REQ_NORMAL ) == FAIL)
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				      "SID $9 INF $%X request\n", IdIndex );

				if	( IdIndex != 0x04 ) // IF not CALID, Don't check
				{
					continue;
				}
			}

			for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
			{
				/* If INF is not supported, skip to next ECU */
				if ( IsSid9InfSupported (EcuIndex, IdIndex) == FALSE )
					continue;

				/* INF's 1, 3, 5 & 7 should not be supported by any ECU if protocol is ISO15765 */
				if ( gOBDList[gOBDListIndex].Protocol == ISO15765 )
				{
					if ( IdIndex ==  0x01 || IdIndex ==  0x03 || IdIndex ==  0x05 || IdIndex ==  0x07 ||
					     IdIndex ==  0x09 || IdIndex ==  0x0C || IdIndex ==  0x0E )
					{
						Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  SID $9 INF $%X is supported (Should not be supported by ISO5765 vehicles)\n",
						     GetEcuId(EcuIndex),
						     IdIndex );
						continue;
					}
				}


				/* Check the data to see if it is valid */
				pSid9 = (SID9 *)&gOBDResponse[EcuIndex].Sid9Inf[0];

				if ( gOBDResponse[EcuIndex].Sid9InfSize == 0 )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					          "ECU %X  No SID $9 INF $%X data\n", GetEcuId(EcuIndex), IdIndex );
					continue;
				}

				/* Check various INF values for validity */
				switch(pSid9->INF)
				{
					case INF_TYPE_VIN_COUNT:
					{
						if ( (gOBDList[gOBDListIndex].Protocol != ISO15765) &&
						     (pSid9->NumItems != 0x05) )
						{
							bTestFailed = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $9 INF $1 (VIN Count) NumItems = %d (should be 5)\n",
							     GetEcuId(EcuIndex),
							     pSid9->NumItems );
						}
					}
					break;

					case INF_TYPE_VIN:
					{
						// Copy the VIN into the global array
						if (gOBDList[gOBDListIndex].Protocol == ISO15765)
						{
							memcpy (&gVIN[0], &pSid9[0].Data[0], 17);

							/// Check that there are no pad bytes included in message
							if (pSid9[0].Data[20] != 0x00)
							{
								bTestFailed = TRUE;
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  SID $9 INF $2 format error! (Must be 17 chars and no pad bytes)\n",
								     GetEcuId(EcuIndex) );
							}

							// SID9 INF2 NODI must equal $01
							if ( pSid9->NumItems != 0x01 )
							{
								bTestFailed = TRUE;
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  SID $9 INF $2 NODI = %d (Must be 1)\n",
								     GetEcuId(EcuIndex),
								     pSid9->NumItems );
							}
						}
						else
						{
							/* INF $2 (VIN) should not be supported if INF $1 (VIN msg count) not supported for non-15765 protocols */
							if ( IsSid9InfSupported ( EcuIndex, 0x01 ) == FALSE )
							{
								bTestFailed = TRUE;
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  SID $9 INF $2 (VIN) is supported but not INF $1 (VIN msg count)\n",
								     GetEcuId(EcuIndex));
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
									memcpy(&gVIN[SidIndex*4 - 3], &pSid9[SidIndex].Data[0], 4);
								}
							}
						}

						// Check for INF2 (VIN) support from multiple controllers
						if ( (++Inf2NumResponses != 0x01) &&
						     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
						{
							bTestFailed = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $9 INF $2 response from multiple controllers!\n" );
						}

						/* Isolated VIN verification logic */
						if (VerifyVINFormat (EcuIndex) != PASS)
						{
							bTestFailed = TRUE;
						}
					}
					break;

					case INF_TYPE_CALID_COUNT:
					{
						/* Response should be a multiple of four if not ISO15765 */
						if ( (gOBDList[gOBDListIndex].Protocol != ISO15765) &&
						     (pSid9->NumItems & 0x03) )
						{
							bTestFailed = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $9 INF $3 (CALID Count) NumItems = %d (should be a multiple of 4)\n",
							     GetEcuId(EcuIndex),
							     pSid9->NumItems );
						}

						Inf3NumItems[EcuIndex] = pSid9->NumItems;
					}
					break;

					case INF_TYPE_CALID:
					{
						fInf4Responded[EcuIndex] = TRUE;

						if ( VerifyCALIDFormat ( EcuIndex, Inf3NumItems[EcuIndex] ) != PASS)
						{
							bTestFailed = TRUE;
						}
					}
					break;

					case INF_TYPE_CVN_COUNT:
					{
						Inf5NumItems[EcuIndex] = pSid9->NumItems;
					}
					break;

					case INF_TYPE_CVN:
					{
						Inf6NumResponses++;

						if ( VerifyCVNFormat ( EcuIndex, Inf5NumItems[EcuIndex] ) != PASS )
						{
							bTestFailed = TRUE;
						}
					}
					break;

					case INF_TYPE_IPT_COUNT:
					{
						/* non-ISO15765 should report 0x08, 0x9, or 0xA */
						if ( (gOBDList[gOBDListIndex].Protocol != ISO15765) &&
						     (pSid9->NumItems != 0x08) &&
						     (pSid9->NumItems != 0x09) &&
						     (pSid9->NumItems != 0x0A)
						   )
						{
							bTestFailed = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $9 INF $7 (IPT Count) is invalid   Message Count = %d\n",
							     GetEcuId(EcuIndex),
							     pSid9->NumItems);
						}

						Inf7NumItems[EcuIndex] = pSid9->NumItems;
					}
					break;

					case INF_TYPE_IPT:
					{
						Inf8NumResponses++;

						if ( VerifyINF8Data (EcuIndex) != PASS )
						{
							bTestFailed = TRUE;
						}

						IGNCNTR = (gOBDResponse[EcuIndex].Sid9Inf[IPT_IGNCNTR_INDEX] * 256) + gOBDResponse[EcuIndex].Sid9Inf[IPT_IGNCNTR_INDEX+1];

						/* Response should match INF7 if not ISO15765 */
						if ((gOBDList[gOBDListIndex].Protocol != ISO15765) &&
						    (gOBDResponse[EcuIndex].Sid9Inf[gOBDResponse[EcuIndex].Sid9InfSize - 5] != Inf7NumItems[EcuIndex]))
						{
							bTestFailed = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $9 INF $8 (IPT) Message Count = %d (should match INF7)\n",
							     GetEcuId(EcuIndex),
							     gOBDResponse[EcuIndex].Sid9Inf[gOBDResponse[EcuIndex].Sid9InfSize - 5] );
						}
					}
					break;

					case INF_TYPE_ECUNAME_COUNT:
					{
						/* non-ISO15765 should report 0x05 messages */
						if ( (gOBDList[gOBDListIndex].Protocol != ISO15765) &&
						     (pSid9->NumItems != 0x05) )
						{
							bTestFailed = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $9 INF $9 (IPT Count) is invalid   Message Count = %d (should be 0x05)\n",
							     GetEcuId(EcuIndex),
							     pSid9->NumItems);
						}

						Inf9NumItems[EcuIndex] = pSid9->NumItems;
					}
					break;

					case INF_TYPE_ECUNAME:
					{
						fInfAResponded[EcuIndex] = TRUE;

						if ( VerifyECUNameFormat (EcuIndex) != PASS )
						{
							bTestFailed = TRUE;
						}

						/* Response should match INF9 if not ISO15765 */
						if ((gOBDList[gOBDListIndex].Protocol != ISO15765) &&
							(gOBDResponse[EcuIndex].Sid9Inf[gOBDResponse[EcuIndex].Sid9InfSize - 5] != Inf9NumItems[EcuIndex]))
						{
							bTestFailed = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $9 INF $A (ECU Name) Message Count = %d (should match INF9)\n",
							     GetEcuId(EcuIndex),
							     gOBDResponse[EcuIndex].Sid9Inf[gOBDResponse[EcuIndex].Sid9InfSize - 5] );
						}

						// Check for duplicate ECU Names
						for ( EcuLoopIndex = 0; EcuLoopIndex < EcuIndex; EcuLoopIndex++ )
						{
							if (gOBDResponse[EcuIndex].Sid9InfSize != 0 &&
							    gOBDResponse[EcuLoopIndex].Sid9InfSize != 0 &&
							    memicmp(&gOBDResponse[EcuIndex].Sid9Inf[0],
							            &gOBDResponse[EcuLoopIndex].Sid9Inf[0],
							            gOBDResponse[EcuLoopIndex].Sid9InfSize) == 0 )
							{
								bTestFailed = TRUE;
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X and ECU %X  SID $9 INF $A, Duplicate ECU names\n",
								     GetEcuId(EcuIndex),
								     GetEcuId(EcuLoopIndex) );
							}
						}
					}
					break;

					case INF_TYPE_IPD:
					{
						InfBNumResponses++;

						if ( VerifyINFBData (EcuIndex) != PASS )
						{
							bTestFailed = TRUE;
						}

						IGNCNTR = (gOBDResponse[EcuIndex].Sid9Inf[IPT_IGNCNTR_INDEX] * 256) + gOBDResponse[EcuIndex].Sid9Inf[IPT_IGNCNTR_INDEX+1];

						/* Response should match INF7 if not ISO15765 */
						if ((gOBDList[gOBDListIndex].Protocol != ISO15765) &&
						    (gOBDResponse[EcuIndex].Sid9Inf[gOBDResponse[EcuIndex].Sid9InfSize - 5] != Inf7NumItems[EcuIndex]))
						{
							bTestFailed = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $9 INF $B (IPT) Message Count = %d (should match INF7)\n",
							     GetEcuId(EcuIndex),
							     gOBDResponse[EcuIndex].Sid9Inf[gOBDResponse[EcuIndex].Sid9InfSize - 5] );
						}
					}
					break;

					case INF_TYPE_ESN_COUNT:	/* INFOTYPE $0C Engine Serial Number Message Count */
					{
						/* non-ISO15765 should report 0x05 messages */
						if ( (gOBDList[gOBDListIndex].Protocol != ISO15765) &&
						     (pSid9->NumItems != 0x05) )
						{
							bTestFailed = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $9 INF $C (ESN Count) is invalid   Message Count = %d (should be 0x05)\n",
							     GetEcuId(EcuIndex),
							     pSid9->NumItems);
						}
						InfCNumItems[EcuIndex] = pSid9->NumItems;
					}
					break;

					case INF_TYPE_ESN:	/* INFOTYPE $0D Engine Serial Number */
					{
						if ( VerifyInfDAndFFormat (EcuIndex) != PASS )
						{
							bTestFailed = TRUE;
						}
					}
					break;

					case INF_TYPE_EROTAN_COUNT:	/* INFOTYPE $0E Exhaust Regulation or Type Approval Number Message Count */
					{
						/* non-ISO15765 should report 0x05 messages */
						if ( (gOBDList[gOBDListIndex].Protocol != ISO15765) &&
						     (pSid9->NumItems != 0x05) )
						{
							bTestFailed = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $9 INF $E (EROTAN Count) is invalid   Message Count = %d (should be 0x05)\n",
							     GetEcuId(EcuIndex),
							     pSid9->NumItems);
						}
						InfENumItems[EcuIndex] = pSid9->NumItems;
					}
					break;

					case INF_TYPE_EROTAN:	/* INFOTYPE $0F Exhaust Regulation or Type Approval */
					{
						if ( VerifyInfDAndFFormat (EcuIndex) != PASS )
						{
							bTestFailed = TRUE;
						}
					}
					break;

					case INF_TYPE_FEOCNTR:
					{
						if ( VerifyInf12Data (EcuIndex, IGNCNTR) != PASS )
						{
							bTestFailed = TRUE;
						}
					}
					break;

					case INF_TYPE_EVAP_DIST:
					{
						if ( VerifyInf14Data (EcuIndex) != PASS )
						{
							bTestFailed = TRUE;
						}
					}
					break;

					default:
					{
						/* Non-OBD INF type */
					}
					break;
				} /* end switch(pSid9->INF) */
			} /* end for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ ) */
		} /* end if ( IsSid9InfSupported (-1, IdIndex) == TRUE ) */

	} /* end for ( IdIndex = 0x01; IdIndex <= 0x0B; IdIndex++ ) */


	for ( EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++ )
	{
		/* Verify that every ECU responded to INF $04 request */
		if ( fInf4Responded[EcuIndex] == FALSE )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  Did not report SID $9 INF $4\n", GetEcuId(EcuIndex) );
			bTestFailed = TRUE;
		}

		// Verify that INF $A is supported for MY 2010 and later
		if ( gModelYear >= 2010 &&
		     fInfAResponded[EcuIndex] == FALSE &&
		     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  Did not report SID $9 INF $A! (Required for MY 2010 and later vehicles)\n",
			     GetEcuId(EcuIndex) );
			bTestFailed = TRUE;
		}

		// Check if INF $13 is supported, this is a failure
		if ( IsSid9InfSupported (EcuIndex, 0x13) == TRUE )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  Supports INF $13 (Must not support INF $13)\n",
			     GetEcuId(EcuIndex) );
			bTestFailed = TRUE;
		}

		// Check if INFOTYPES higher than $14 are supported, this is a failure
		for ( IdIndex = 0x15; IdIndex <= 0x100; IdIndex++ )
		{
			/* If INF is supported, this is a failure */
			if ( IsSid9InfSupported (EcuIndex, IdIndex) == TRUE )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  Supports INF $%X (Must not support INF higher than $14)\n",
				     GetEcuId(EcuIndex),
				     IdIndex );
				bTestFailed = TRUE;
			}
		 }
	}

	// Verify that INF $2 is supported by the correct number of ECUs
	if ( Inf2NumResponses == 0)
	{
		if ( gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $9 INF $2 must be reported by one ECU\n");
			bTestFailed = TRUE;
		}
		else
		{
			Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $9 INF $2 not reported by any ECUs\n");
		}
	}

	// Verify that INF $6 is supported by the correct number of ECUs
	if (
	     (Inf6NumResponses < gUserNumEcusReprgm || Inf6NumResponses > gUserNumEcus ) &&
	     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD)
	   )
	{
		Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $9 INF $6 not reported by expected number of ECUs\n");
	}

	if ( gOBDDieselFlag == FALSE && Inf8NumResponses == 0)
	{
		if ( gModelYear >= 2005 &&
		     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
		{
			/* If M/Y 2005 or greater and OBD II, INFO type 8 must be supported */
			/* If not supported then OEM will present Infotype phase in plan */
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "No SID $9 INF $8 response (Required for Spark Ignition vehicles MY 2005 and later)\n");
			bTestFailed = TRUE;
		}
		else if ( gUserInput.eComplianceType == EOBD_WITH_IUMPR || gUserInput.eComplianceType == HD_EOBD )
		{
			/* If EOBD with IUMPR, INFO type 8 must be supported */
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "No SID $9 INF $8 response (Required for Spark Ignition vehicles)\n");
			bTestFailed = TRUE;
		}
	}

	if ( gOBDDieselFlag == TRUE && InfBNumResponses == 0)
	{
		if ( gModelYear >= 2010 &&
		     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
		{
			/* If M/Y 2010 or greater, INFO type B must be supported */
			/* If not supported then OEM will present Infotype phase in plan */
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "No SID $9 INF $B response (Required for Compression Ignition vehicles MY 2010 and later)\n");
			bTestFailed = TRUE;
		}
		else if ( gUserInput.eComplianceType == EOBD_WITH_IUMPR || gUserInput.eComplianceType == HD_EOBD )
		{
			/* If EOBD with IUMPR, INFO type 8 must be supported */
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "No SID $9 INF $B response (Required for Compression Ignition vehicles)\n");
			bTestFailed = TRUE;
		}
	}


	/* Try group support if ISO15765 */
	if (gOBDList[gOBDListIndex].Protocol == ISO15765)
	{
		if (VerifyGroupVehicleInformationSupport() == FAIL)
		{
			bTestFailed = TRUE;
		}
	}

	// If there where any errors in the responses, fail
	if ( (bTestFailed == TRUE) || (ulInit_FailureCount != GetFailureCount()) )
	{
		/* There could have been early/late responses that weren't treated as FAIL */
		return(FAIL);
	}
	else
	{
		return(PASS);
	}

}

/******************************************************************************
**
**	Function:	RequestSID9SupportData
**
**	Purpose:	Purpose of this function is to Request SID 9 support data
**
*******************************************************************************
**
**	DATE        MODIFICATION
**	10/22/03    Isolated Request SID 9 support data
**	05/11/04    Added logic, per J1699 ver 11.5 TC 5.17.3, request next
**	            unsupported INFOTYPE-support INFOTYPE and verify ECU did
**	            not drop out.
**	06/16/04    Added logic to account for upper $E0 limit and validate
**	            support.
**	07/15/04    Correct logic error associated with the evaluation of $E0
**	            support.
**
*******************************************************************************
*/
STATUS RequestSID9SupportData (void)
{
	unsigned long EcuIndex;
	unsigned long IdIndex;
	unsigned long ulInfSupport;  /* used to determine $E0 support indication */

	SID_REQ SidReq;

	for (IdIndex = 0x00; IdIndex < 0x100; IdIndex += 0x20)
	{
		SidReq.SID = 9;
		SidReq.NumIds = 1;
		SidReq.Ids[0] = (unsigned char)IdIndex;
		if (SidRequest(&SidReq, SID_REQ_NORMAL) == FAIL)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $9 INF-Supported INF $%X request\n", IdIndex );
			return FAIL;
		}

		/* Check if we need to request the next group */
		for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
		{
			if (gOBDResponse[EcuIndex].Sid9InfSupport[IdIndex >> 5].IDBits[3] & 0x01)
			{
				break;
			}
		}
		if (EcuIndex >= gUserNumEcus)
		{
			break;
		}
	}

	/* Flag error if ECU indicates no support */
	if (VerifySid9PidSupportData() == FAIL)
	{
		return (FAIL);
	}

	/* Enhance logic to verify support information if request is at upper limit of $E0 */
	if ( IdIndex == 0xE0 )
	{
		/* Init variable to no-support */
		ulInfSupport = 0;

		/* For each ECU */
		for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
		{
			/* If MID is supported, keep looking */
			if ( ( gOBDResponse[EcuIndex].Sid9InfSupport[IdIndex >> 5].IDBits[0]		||
			       gOBDResponse[EcuIndex].Sid9InfSupport[IdIndex >> 5].IDBits[1]		||
			       gOBDResponse[EcuIndex].Sid9InfSupport[IdIndex >> 5].IDBits[2]		||
			     ( gOBDResponse[EcuIndex].Sid9InfSupport[IdIndex >> 5].IDBits[3] & 0xFE ) ) != 0x00)
			{
				/* Flag as support indicated! */
				ulInfSupport = 1;
			}
		}

		/* If no ECU indicated support, flag as error. */
		if ( ulInfSupport == 0 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "No SID $9 INF $E0 support indicated!\n");
			return FAIL;
		}
	}
	else
	{
		/*
		** Request next unsupported INFOTYPE-support INFOTYPE
		*/
		SidReq.SID = 9;
		SidReq.NumIds = 1;
		SidReq.Ids[0] = (unsigned char)(IdIndex += 0x20);

		if ( SidRequest(&SidReq, SID_REQ_NORMAL|SID_REQ_IGNORE_NO_RESPONSE) != FAIL )
		{
			if (gOBDList[gOBDListIndex].Protocol == ISO15765)
			{
				for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
				{
					if (gOBDResponse[EcuIndex].Sid9InfSize != 0)
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  Unexpected response to unsupported SID $9 INF $%X!\n", GetEcuId(EcuIndex), IdIndex );
					}
				}
				return FAIL;
			}
		}
	}

	/*
	** Per J1699 rev 11.5 TC# 5.17.3 - Verify ECU did not drop out.
	*/
	if (VerifyLinkActive() != PASS)
	{
		return FAIL;
	}

	return PASS;
}


/******************************************************************************
**
**	Function:   VerifyVINFormat
**
**	Purpose:    Purpose of this function is to verify the VIN format
**	            for correct format.  In the event the format fails
**	            defined criteria, an error is returned.
**
*******************************************************************************
**
**	DATE        MODIFICATION
**	10/22/03    Isolated VIN verification logic
**	06/17/04    Update VIN character validation as documented in J1699 version
**	            11.6, table 79.
**
******************************************************************************/
STATUS VerifyVINFormat (unsigned long EcuIndex)
{
	unsigned long VinIndex;
	STATUS        RetCode = PASS;

	// Print VIN string to log file
	 Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "VIN = %s\n", gVIN);

	/* Check all VIN characters for validity */
	for (VinIndex = 0; VinIndex < 17; VinIndex++)
	{
		if ((gVIN[VinIndex] <  '0') || (gVIN[VinIndex] >  'Z') ||
		    (gVIN[VinIndex] == 'I') || (gVIN[VinIndex] == 'O') ||
		    (gVIN[VinIndex] == 'Q') || (gVIN[VinIndex] == ':') ||
		    (gVIN[VinIndex] == ';') || (gVIN[VinIndex] == '<') ||
		    (gVIN[VinIndex] == '>') || (gVIN[VinIndex] == '=') ||
		    (gVIN[VinIndex] == '?') || (gVIN[VinIndex] == '@'))
		{
			break;
		}
	}

	if (VinIndex != 17)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  Invalid VIN information\n", GetEcuId(EcuIndex));
		RetCode = FAIL;
	}


	/* Check that VIN model year character matches user input model year */
	if (
	    (( gVIN[9] == '1' && gModelYear != 2001 ) ||
	     ( gVIN[9] == '2' && gModelYear != 2002 ) ||
	     ( gVIN[9] == '3' && gModelYear != 2003 ) ||
	     ( gVIN[9] == '4' && gModelYear != 2004 ) ||
	     ( gVIN[9] == '5' && gModelYear != 2005 ) ||
	     ( gVIN[9] == '6' && gModelYear != 2006 ) ||
	     ( gVIN[9] == '7' && gModelYear != 2007 ) ||
	     ( gVIN[9] == '8' && gModelYear != 2008 ) ||
	     ( gVIN[9] == '9' && gModelYear != 2009 ) ||
	     ( gVIN[9] == 'A' && gModelYear != 2010 ) ||
	     ( gVIN[9] == 'B' && gModelYear != 1981 && gModelYear != 2011 ) ||
	     ( gVIN[9] == 'C' && gModelYear != 1982 && gModelYear != 2012 ) ||
	     ( gVIN[9] == 'D' && gModelYear != 1983 && gModelYear != 2013 ) ||
	     ( gVIN[9] == 'E' && gModelYear != 1984 && gModelYear != 2014 ) ||
	     ( gVIN[9] == 'F' && gModelYear != 1985 && gModelYear != 2015 ) ||
	     ( gVIN[9] == 'G' && gModelYear != 1986 && gModelYear != 2016 ) ||
	     ( gVIN[9] == 'H' && gModelYear != 1987 && gModelYear != 2017 ) ||
	     ( gVIN[9] == 'J' && gModelYear != 1988 && gModelYear != 2018 ) ||
	     ( gVIN[9] == 'K' && gModelYear != 1989 && gModelYear != 2019 ) ||
	     ( gVIN[9] == 'L' && gModelYear != 1990 && gModelYear != 2020 ) ||
	     ( gVIN[9] == 'M' && gModelYear != 1991 && gModelYear != 2021 ) ||
	     ( gVIN[9] == 'N' && gModelYear != 1992 && gModelYear != 2022 ) ||
	     ( gVIN[9] == 'P' && gModelYear != 1993 && gModelYear != 2023 ) ||
	     ( gVIN[9] == 'R' && gModelYear != 1994 && gModelYear != 2024 ) ||
	     ( gVIN[9] == 'S' && gModelYear != 1995 && gModelYear != 2025 ) ||
	     ( gVIN[9] == 'T' && gModelYear != 1996 ) ||
	     ( gVIN[9] == 'V' && gModelYear != 1997 ) ||
	     ( gVIN[9] == 'W' && gModelYear != 1998 ) ||
	     ( gVIN[9] == 'X' && gModelYear != 1999 ) ||
	     ( gVIN[9] == 'Y' && gModelYear != 2000 ) ) &&
	     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD)
	   )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  VIN year character (%c) does not match user entry (%d)\n", GetEcuId(EcuIndex), gVIN[9], gModelYear );
		RetCode = FAIL;
	}

	else if ( gVIN[9] == 'Z' )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  Invalid VIN year character (%c) not currently defined)\n", GetEcuId(EcuIndex), gVIN[9] );
		RetCode = FAIL;
	}

	else if (
	          (gModelYear < 1981 || gModelYear > 2025) &&
	          (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD)
	        )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  Model Year %d does not currently have a VIN character defined)\n", GetEcuId(EcuIndex), gModelYear );
		RetCode = FAIL;
	}


	return RetCode;
}

//*****************************************************************************
//
//	Function:   VerifyCALIDFormat
//
//	Purpose:    Purpose of this function is to verify the CALID format
//	            for correct format.  In the event the format fails
//	            defined criteria, an error is returned.
//
//*****************************************************************************
//
//	DATE        MODIFICATION
//	11/01/03    Isolated CALID verification logic
//
//*****************************************************************************
STATUS VerifyCALIDFormat ( unsigned long EcuIndex, unsigned long Inf3NumItems )
{
	unsigned long  SidIndex;
	unsigned long  Sid9Limit;
	unsigned long  Inf4NumItems;
	unsigned long  ItemIndex;
	unsigned long  ByteIndex;
	char           buffer[20];
	SID9          *pSid9;
	STATUS         RetCode = PASS;
	BOOL bCalidEnd = FALSE;

	if (gOBDResponse[EcuIndex].Sid9InfSize == 0)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  No SID $9 INF $4 data\n", GetEcuId(EcuIndex) );
		return FAIL;
	}

	pSid9 = (SID9 *)&gOBDResponse[EcuIndex].Sid9Inf[0];

	if (gOBDList[gOBDListIndex].Protocol == ISO15765)
	{
		Inf4NumItems = pSid9->NumItems;

		// SID9 INF4 NODI must equal number of CALIDs
		if ( (pSid9->NumItems) * 16 != (gOBDResponse[EcuIndex].Sid9InfSize - 2) )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $4 NODI = %d mis-matched with Data Size\n",
			     GetEcuId(EcuIndex),
			     pSid9->NumItems );
			return FAIL;
		}

		for (ItemIndex = 0; ItemIndex < Inf4NumItems; ItemIndex++)
		{
			bCalidEnd = FALSE;
			memcpy (buffer, &(pSid9->Data[ItemIndex * 16]), 16);

			for (ByteIndex = 0; ByteIndex < 16; ByteIndex++)
			{
				if ( bCalidEnd == FALSE )
				{
					if ( (buffer[ByteIndex] < 0x20) || (buffer[ByteIndex] > 0x7E) )
					{
						if ( buffer[ByteIndex] != 0x00 )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  CALID contains one or more invalid characters\n", GetEcuId(EcuIndex) );
							RetCode = FAIL;
							buffer[ByteIndex] = 0;
							break;
						}
						else if (ByteIndex == 0)
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  CALID can not start with pad byte\n", GetEcuId(EcuIndex) );
							RetCode = FAIL;
							break;
						}
						else
						{
							bCalidEnd = TRUE;
						}
					}
				}
				else
				{
					if (buffer[ByteIndex] != 0)
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  CALID not zero padded on right\n", GetEcuId(EcuIndex) );
						RetCode = FAIL;
						buffer[ByteIndex] = 0;
						break;
					}
				}
			} // end for (ByteIndex...

			buffer[16] = 0;
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  CALID: %s\n", GetEcuId(EcuIndex), buffer );
		}

		/* if ISO15765, response size should be 1 for MY 2009 */
		if ( gModelYear >= 2009 &&
		     pSid9[0].NumItems != 1 )
		{
			Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $4 (CALID) NumItems = %d (should be 1 for MY 2009 and later, unless EO approved)\n",
			     GetEcuId(EcuIndex),
			     pSid9[0].NumItems );
		}
	}
	else
	{
		// size of each CALIDs messsage checked in SidSaveResponseData.c

		Sid9Limit = gOBDResponse[EcuIndex].Sid9InfSize / sizeof (SID9);

		for (SidIndex = 0; SidIndex < Sid9Limit; SidIndex += 4)
		{
			bCalidEnd = FALSE;
			for (ItemIndex = 0, ByteIndex = 0; ItemIndex < 4; ItemIndex++, pSid9++)
			{
				buffer[ByteIndex++] = pSid9->Data[0];
				buffer[ByteIndex++] = pSid9->Data[1];
				buffer[ByteIndex++] = pSid9->Data[2];
				buffer[ByteIndex++] = pSid9->Data[3];
			}

			for (ByteIndex = 0; ByteIndex < 16; ByteIndex++)
			{
				if ( bCalidEnd == FALSE )
				{
					if ( (buffer[ByteIndex] < 0x20) || (buffer[ByteIndex] > 0x7E) )
					{
						if ( buffer[ByteIndex] != 0x00 )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  CALID contains one or more invalid characters\n", GetEcuId(EcuIndex) );
							RetCode = FAIL;
							buffer[ByteIndex] = 0;
							break;
						}
						else if (ByteIndex == 0)
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  CALID can not start with pad byte\n", GetEcuId(EcuIndex) );
							RetCode = FAIL;
							break;
						}
						else
						{
							bCalidEnd = TRUE;
						}
					}
				}
				else
				{
					if (buffer[ByteIndex] != 0)
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  CALID not zero padded on right\n", GetEcuId(EcuIndex) );
						RetCode = FAIL;
						buffer[ByteIndex] = 0;
						break;
					}
				}
			} // end for (ByteIndex...

			buffer[ByteIndex] = 0;
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  CALID: %s\n", GetEcuId (EcuIndex), buffer );
		}

		/*
		 * reload SID9 pointer for next check, it should be pointing at the last item
		 */
		pSid9--;

		/* Response should match INF $3
		 * pSid9[Inf3NumItems[EcuIndex]-1].NumItems is actually
		 * the message count (1-XX) of the last 4-byte block in the message
		 * (ie if Inf3NumItems[EcuIndex] == 4 (1 16 byte CALID is made up of 4 4-byte messages)
		 * the last block of the message should have a message count of 4
		 */
		if ( pSid9->NumItems != Inf3NumItems )
		{
			Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $4 (CALID) NumItems = %d (should match INF $3 CALID Count %d)\n",
			     GetEcuId(EcuIndex),
			     pSid9->NumItems,
			     Inf3NumItems);
		}

		/* Response size should be 1 (16-byte CALID made up of 4 4-byte messages) for MY 2009 */
		if ( gModelYear >= 2009 &&
		     pSid9->NumItems != 4 )
		{
			Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $4 (CALID) NumItems = %d (should be 4 (for 1 16-byte CALID) for MY 2009 and later, unless EO approved)\n",
			     GetEcuId(EcuIndex),
			     pSid9->NumItems );
		}
	}

	return RetCode;
}


//*****************************************************************************
//
//	Function:   VerifyCVNFormat
//
//	Purpose:    Purpose of this function is to verify the CVN format
//	            for correct format.  In the event the format fails
//	            defined criteria, an error is returned.
//
//*****************************************************************************
STATUS VerifyCVNFormat ( unsigned long EcuIndex, unsigned long Inf5NumItems )
{
	unsigned long  SidIndex;
	unsigned long  Sid9Limit;
	unsigned long  Inf6NumItems;
	unsigned long  ItemIndex;
	unsigned long  ByteIndex;
	char           buffer[26];
	SID9          *pSid9;

	if ( gOBDResponse[EcuIndex].Sid9InfSize == 0 )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  No SID $9 INF $6 (CVN) data\n", GetEcuId(EcuIndex) );
		return PASS;
	}

	pSid9 = (SID9 *)&gOBDResponse[EcuIndex].Sid9Inf[0];

	if ( gOBDList[gOBDListIndex].Protocol == ISO15765 )
	{
		Inf6NumItems = pSid9->NumItems;

		// SID9 INF6 NODI must equal number of CVNs
		if ( (pSid9->NumItems) * 4 != (gOBDResponse[EcuIndex].Sid9InfSize - 2) )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $6 NODI = %d mis-matched with Data Size\n",
			     GetEcuId(EcuIndex),
			     pSid9->NumItems );
			return FAIL;
		}

		for ( ItemIndex = 0, ByteIndex = 0; ItemIndex < Inf6NumItems; ItemIndex++, ByteIndex += 4 )
		{
			sprintf ( buffer,
			          "%02X %02X %02X %02X",
			          pSid9->Data[ByteIndex],
			          pSid9->Data[ByteIndex+1],
			          pSid9->Data[ByteIndex+2],
			          pSid9->Data[ByteIndex+3] );

			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  CVN: %s\n", GetEcuId(EcuIndex), buffer );
		}

		/* If ISO15765, response size should be 1 for MY 2009 */
		if ( gModelYear >= 2009 &&
		     pSid9[0].NumItems != 1)
		{
			Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $6 (CVN) NumItems = %d (should be 1 for MY 2009 and later, unless EO approved)\n",
			     GetEcuId(EcuIndex),
			     pSid9[0].NumItems );
		}
	}
	else
	{
		// size of each CVN checked in SidSaveResponseData.c

		Sid9Limit = gOBDResponse[EcuIndex].Sid9InfSize / sizeof (SID9);

		for ( SidIndex = 0; SidIndex < Sid9Limit; SidIndex += 2 )
		{
			for ( ItemIndex = 0; ItemIndex < 2; ItemIndex++, pSid9++ )
			{
				sprintf ( buffer,
				          "%02X %02X %02X %02X",
				          pSid9->Data[0],
				          pSid9->Data[1],
				          pSid9->Data[2],
				          pSid9->Data[3] );

				Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  CVN: %s\n", GetEcuId (EcuIndex), buffer );
			}
		}

		/* reload SID9 pointer for next check */
		pSid9--;

		if ( pSid9->NumItems != Inf5NumItems )
		{
			Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $6 (CVN) NumItems = %d (should match INF $5 CVN Count %d)\n",
			     GetEcuId(EcuIndex),
			     pSid9->NumItems,
			     Inf5NumItems );
		}

		/* Response size should be 1 for MY 2009 */
		if ( gModelYear >= 2009 &&
		     pSid9->NumItems != 1)
		{
			Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $6 (CVN) NumItems = %d (should be 1 for MY 2009 and later, unless EO approved)\n",
			     GetEcuId(EcuIndex),
			     pSid9->NumItems );
		}
	}

	return PASS;
}

/******************************************************************************
**
**	Function:   VerifyECUNameFormat
**
**	Purpose:    Purpose of this function is to verify the ECU Name format
**	            for correct format.  In the event the format fails
**	            defined criteria, an error is returned.
**
*******************************************************************************
**
**	DATE        MODIFICATION
**	07/07/07    Isolated ECU Name verification logic
**
******************************************************************************/
#define TABLE_SIZE  37
#define ACRONYM_STRING_SIZE 45
#define TEXTNAME_STRING_SIZE 16
const char szECUAcronym[TABLE_SIZE][ACRONYM_STRING_SIZE] =
	{ "ABS ABS1 ABS2 ABS3 ABS4 ABS5 ABS6 ABS7 ABS8",
	  "AFCM AFC1 AFC2 AFC3 AFC4 AFC5 AFC6 AFC7 AFC8",
	  "AHCM AHC1 AHC2 AHC3 AHC4 AHC5 AHC6 AHC7 AHC8",
	  "APCM APC1 APC2 APC3 APC4 APC5 APC6 APC7 APC8",
	  "AWDC AWD1 AWD2 AWD3 AWD4 AWD5 AWD6 AWD7 AWD8",
	  "BCCM BCC1 BCC2 BCC3 BCC4 BCC5 BCC6 BCC7 BCC8",
	  "BECM BEC1 BEC2 BEC3 BEC4 BEC5 BEC6 BEC7 BEC8",
	  "BCM BCM1 BCM2 BCM3 BCM4 BCM5 BCM6 BCM7 BCM8",
	  "BSCM BSC1 BSC2 BSC3 BSC4 BSC5 BSC6 BSC7 BSC8",
	  "CATM CAT1 CAT2 CAT3 CAT4 CAT5 CAT6 CAT7 CAT8",
	  "CMPM CMP1 CMP2 CMP3 CMP4 CMP5 CMP6 CMP7 CMP8",
	  "CHCM CHC1 CHC2 CHC3 CHC4 CHC5 CHC6 CHC7 CHC8",
	  "CRCM CRC1 CRC2 CRC3 CRC4 CRC5 CRC6 CRC7 CRC8",
	  "CTCM CTC1 CTC2 CTC3 CTC4 CTC5 CTC6 CTC7 CTC8",
	  "DCDC DCD1 DCD2 DCD3 DCD4 DCD5 DCD6 DCD7 DCD8",
	  "DMCM DMC1 DMC2 DMC3 DMC4 DMC5 DMC6 DMC7 DMC8",
	  "EACC EAC1 EAC2 EAC3 EAC4 EAC5 EAC6 EAC7 EAC8",
	  "EACM EAM1 EAM2 EAM3 EAM4 EAM5 EAM6 EAM7 EAM8",
	  "EBBC EBB1 EBB2 EBB3 EBB4 EBB5 EBB6 EBB7 EBB8",
	  "ECCI ECC1 ECC2 ECC3 ECC4 ECC5 ECC6 ECC7 ECC8",
	  "ECM ECM1 ECM2 ECM3 ECM4 ECM5 ECM6 ECM7 ECM8",
	  "FACM FAC1 FAC2 FAC3 FAC4 FAC5 FAC6 FAC7 FAC8",
	  "FICM FIC1 FIC2 FIC3 FIC4 FIC5 FIC6 FIC7 FIC8",
	  "FPCM FPC1 FPC2 FPC3 FPC4 FPC5 FPC6 FPC7 FPC8",
	  "4WDC 4WD1 4WD2 4WD3 4WD4 4WD5 4WD6 4WD7 4WD8",
	  "GPCM GPC1 GPC2 GPC3 GPC4 GPC5 GPC6 GPC7 GPC8",
	  "GSM GSM1 GSM2 GSM3 GSM4 GSM5 GSM6 GSM7 GSM8",
	  "HVAC HVA1 HVA2 HVA3 HVA4 HVA5 HVA6 HVA7 HVA8",
	  "HPCM HPC1 HPC2 HPC3 HPC4 HPC5 HPC6 HPC7 HPC8",
	  "IPC IPC1 IPC2 IPC3 IPC4 IPC5 IPC6 IPC7 IPC8",
	  "PCM PCM1 PCM2 PCM3 PCM4 PCM5 PCM6 PCM7 PCM8",
	  "RDCM RDC1 RDC2 RDC3 RDC4 RDC5 RDC6 RDC7 RDC8",
	  "SGCM SGC1 SGC2 SGC3 SGC4 SGC5 SGC6 SGC7 SGC8",
	  "TACM TAC1 TAC2 TAC3 TAC4 TAC5 TAC6 TAC7 TAC8",
	  "TCCM TCC1 TCC2 TCC3 TCC4 TCC5 TCC6 TCC7 TCC8",
	  "TCM TCM1 TCM2 TCM3 TCM4 TCM5 TCM6 TCM7 TCM8",
	  "VTMC VTM1 VTM2 VTM3 VTM4 VTM5 VTM6 VTM7 VTM8"};

const char szECUTextName[TABLE_SIZE][TEXTNAME_STRING_SIZE] =
	{ "AntiLockBrake",
	  "AltFuelCtrl",
	  "AuxHeatCtrl",
	  "AirPumpCtrl",
	  "AllWhlDrvCtrl",
	  "B+ChargerCtrl",
	  "B+EnergyCtrl",
	  "BodyControl",
	  "BrakeSystem",
	  "CatHeaterCtrl",
	  "CamPosCtrl",
	  "ChassisCtrl",
	  "CruiseControl",
	  "CoolTempCtrl",
	  "DCDCConvCtrl",
	  "DriveMotorCtrl",
	  "ElecACCompCtrl",
	  "ExhaustAftCtrl",
	  "ElecBrkBstCtrl",
	  "EmisCritInfo",
	  "EngineControl",
	  "FuelAddCtrl",
	  "FuelInjCtrl",
	  "FuelPumpCtrl",
	  "4WhlDrvClCtrl",
	  "GlowPlugCtrl",
	  "GearShiftCtrl",
	  "HVACCtrl",
	  "HybridPtCtrl",
	  "InstPanelClust",
	  "PowertrainCtrl",
	  "ReductantCtrl",
	  "Start/GenCtrl",
	  "ThrotActCtrl",
	  "TransfCaseCtrl",
	  "TransmisCtrl",
	  "ThermalMgmt" };

STATUS VerifyECUNameFormat ( unsigned long  EcuIndex )
{
	SID9           *pSid9;
	unsigned int    ByteIndex;
	unsigned int    MsgIndex;			// multi-part message index
	char            buffer[21];
	char           *pString;
	char            AcronymBuffer[ACRONYM_STRING_SIZE];
	char            AcronymNumber = 0;
	char            TextNameNumber = 0;
	unsigned int    CompSize;
	unsigned int    TableIndex;
	unsigned int    AcronymTableIndex;
	unsigned char   bTestFailed = FALSE;
	unsigned char   bMatchFound = FALSE;

	if (gOBDResponse[EcuIndex].Sid9InfSize == 0)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  No SID $9 INF $A data\n", GetEcuId(EcuIndex) );
		return FAIL;
	}

	pSid9 = (SID9 *)&gOBDResponse[EcuIndex].Sid9Inf[0];

	// copy to a protocol independent buffer
	if (gOBDList[gOBDListIndex].Protocol == ISO15765)
	{
		// ECU Name Data must contain 20 bytes of data
		if ( gOBDResponse[EcuIndex].Sid9InfSize - 0x02 != 20 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $A (ECU NAME) Data Size Error = %d (Must be 20 bytes!)\n",
			     GetEcuId(EcuIndex),
			     (gOBDResponse[EcuIndex].Sid9InfSize - 0x02) );
			return FAIL;
		}

		// SID9 INFA NODI must equal $01
		if ( pSid9->NumItems != 0x01 )
		{
			bTestFailed = TRUE;
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $A NODI = %d (Must be 1)\n",
			     GetEcuId(EcuIndex),
			     pSid9->NumItems );
		}

		memcpy (buffer, &(pSid9->Data[0]), 20);
	}
	else
	{
		/* For non-CAN, calculate the number of responses.  Expected is
		** 5 (20 bytes) records.  Each response from controller held in
		** data structure SID9.
		*/
		if ( gOBDResponse[EcuIndex].Sid9InfSize / sizeof(SID9) != 0x05 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $A (ECU Name) Data Size Error (Must be 20 bytes!)\n",
			     GetEcuId(EcuIndex) );
			return (FAIL);
		}

		// SID9 INF8 message count must equal $05
		if ( gOBDResponse[EcuIndex].Sid9Inf[gOBDResponse[EcuIndex].Sid9InfSize - 5] != 0x05 ) // check to message number byte
		{
			bTestFailed = TRUE;
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $A Message Count = %d (Must be 5 ($05))\n",
			     GetEcuId(EcuIndex),
			     gOBDResponse[EcuIndex].Sid9Inf[gOBDResponse[EcuIndex].Sid9InfSize - 5] );
		}

		// save ECU Name data to buffer
		for ( MsgIndex = 0; MsgIndex < (gOBDResponse[EcuIndex].Sid9InfSize / sizeof(SID9)); MsgIndex++ )
		{
			memcpy (&(buffer[MsgIndex * 4]), &(pSid9[MsgIndex].Data[0]), 4);
		}
	}


	// Check all ECU Acronym characters for validity
	for ( ByteIndex = 0; ByteIndex < 4; ByteIndex++ )
	{
		if ( buffer[ByteIndex] == 0x00 )
		{
			break;
		}
		else if ( !( buffer[ByteIndex] >= '1' && buffer[ByteIndex] <= '9' ) &&
		          !( buffer[ByteIndex] >= 'A' && buffer[ByteIndex] <= 'Z' ) )
		{
			if ( gModelYear >= 2010 )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  Invalid ECU acronym/number character (byte %d)\n",
				     GetEcuId(EcuIndex),
				     ByteIndex );
				bTestFailed = TRUE;
			}
			else
			{
				Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  Invalid ECU acronym/number character (byte %d)\n",
				     GetEcuId(EcuIndex),
				     ByteIndex );
			}
		}
	}

	// Check that ECU Acronym is approved
	for ( TableIndex = 0; TableIndex < TABLE_SIZE; TableIndex++ )
	{
		memcpy ( AcronymBuffer, &(szECUAcronym[TableIndex]), ACRONYM_STRING_SIZE );

		// get pointer to first string
		pString = strtok ( (char*)&(AcronymBuffer), " " );
		do
		{
			// if found match
			if ( strncmp ( pString, &buffer[0], ByteIndex ) == 0 )
			{
				bMatchFound = TRUE;
				break;
			}

			// get pointer to next string
			pString = strtok ('\0', " " );
		} while ( pString && bMatchFound != TRUE );

		if ( bMatchFound == TRUE )
		{
			break;
		}
	}

	if ( TableIndex == TABLE_SIZE )
	{
		if ( gModelYear >= 2010 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  Not an approved ECU acronym/number\n", GetEcuId(EcuIndex) );
			bTestFailed = TRUE;
		}
		else
		{
			Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  Not an approved ECU acronym/number\n", GetEcuId(EcuIndex) );
		}
	}

	// if last character was a number
	if ( buffer[ByteIndex-1] >= '1' && buffer[ByteIndex-1] <= '9' )
	{
		// save the number for Acronym/Text Name verification
		AcronymNumber = buffer[ByteIndex-1];
	}

	// save index for Acronym/Text Name verification
	AcronymTableIndex = TableIndex;

	// Check for zero padding of ECU Acronym
	for ( ; ByteIndex < 4; ByteIndex++ )
	{
		if ( buffer[ByteIndex] != 0x00 )
		{
			if ( gModelYear >= 2010 )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  ECU acronym/number not zero padded on right\n", GetEcuId(EcuIndex) );
				bTestFailed = TRUE;
			}
			else
			{
				Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  ECU acronym/number not zero padded on right\n", GetEcuId(EcuIndex) );
			}
		}
		else
		{
			buffer[ByteIndex] = ' ';    // replace zero with space for printing to log file
		}
	}

	// Check for invalid delimiter
	if ( buffer[ByteIndex++] != '-' )
	{
		if ( gModelYear >= 2010 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  Invalid ECU Name delimiter character (byte %d)\n",
			     GetEcuId(EcuIndex),
			     ByteIndex );
			bTestFailed = TRUE;
		}
		else
		{
			Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  Invalid ECU Name delimiter character (byte %d)\n",
			     GetEcuId(EcuIndex),
			     ByteIndex );
		}
	}

	// Check all ECU Text Name characters for validity
	for ( ; ByteIndex < 20; ByteIndex++ )
	{
		if ( buffer[ByteIndex] == 0x00 )
		{
			break;
		}

		if ( !( buffer[ByteIndex] >= '1' && buffer[ByteIndex] <= '9' ) &&
		     !( buffer[ByteIndex] >= 'A' && buffer[ByteIndex] <= 'Z' ) &&
		     !( buffer[ByteIndex] >= 'a' && buffer[ByteIndex] <= 'z' ) &&
		     buffer[ByteIndex] != '+' && buffer[ByteIndex] != '/' )
		{
			if ( gModelYear >= 2010 )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  Invalid ECU text name character (byte %d)\n",
				     GetEcuId(EcuIndex),
				     ByteIndex );
				bTestFailed = TRUE;
			}
			else
			{
				Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  Invalid ECU text name character (byte %d)\n",
				     GetEcuId(EcuIndex),
				     ByteIndex );
			}
		}
	}

	// if last character was a number, don't use it in ECU Text Name search
	if ( buffer[ByteIndex-1] >= '1' && buffer[ByteIndex-1] <= '9' )
	{
		// comparison size is buffer size less the Acronym, the Delimiter and the Text Name number
		CompSize = ByteIndex - 6;
		// save the number for Acronym/Text Name verification
		TextNameNumber = buffer[ByteIndex-1];
	}
	else
	{
		// comparison size is buffer size less the Acronym and the Delimiter
		CompSize = ByteIndex - 5;
	}

	// Check that ECU Text Name is approved
	for ( TableIndex = 0; TableIndex < TABLE_SIZE; TableIndex++ )
	{
		// if found match
		if ( strncmp ( &buffer[5], szECUTextName[TableIndex], CompSize ) == 0 )
		{
			break;
		}
	}

	if ( TableIndex == TABLE_SIZE )
	{
		if ( gModelYear >= 2010 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  Not an approved ECU text name\n", GetEcuId(EcuIndex) );
			bTestFailed = TRUE;
		}
		else
		{
			Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  Not an approved ECU text name\n", GetEcuId(EcuIndex) );
		}
	}

	// Check for zero padding of ECU Text Name
	for ( ; ByteIndex < 20; ByteIndex++ )
	{
		if ( buffer[ByteIndex] != 0 )
		{
			if ( gModelYear >= 2010 )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  ECU text name not zero padded on right\n", GetEcuId(EcuIndex) );
				bTestFailed = TRUE;
			}
			else
			{
				Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  ECU text name not zero padded on right\n", GetEcuId(EcuIndex) );
			}
		}
	}

	// Check for ECU Acronym and Text Name match
	if ( AcronymTableIndex != TableIndex ||
	     AcronymNumber != TextNameNumber )
	{
		if ( gModelYear >= 2010 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  ECU acronym and text name do not match\n", GetEcuId(EcuIndex) );
			bTestFailed = TRUE;
		}
		else
		{
			Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  ECU acronym and text name do not match\n", GetEcuId(EcuIndex) );
		}
	}


	// insert NULL terminator at end of string
	buffer[20] = 0;

	// Print ECU Name to Log file
	Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "ECU %X  NAME: %s\n", GetEcuId(EcuIndex), buffer );


	if ( bTestFailed == TRUE )
	{
		return FAIL;
	}
	else
	{
		return PASS;
	}
}


/******************************************************************************
**
**	Function:   VerifyINF8Data
**
**	Purpose:    Purpose of this function is to verify the INF 8 data
**	            for correct format.  In the event the format fails
**	            defined criteria, an error is returned.
**
*******************************************************************************
**
**	DATE        MODIFICATION
**	07/07/07    Isolated INF 8 verification logic
**
******************************************************************************/
STATUS VerifyINF8Data ( unsigned long  EcuIndex )
{
	STATUS eResult=PASS; // function result

	if ( gOBDDieselFlag == TRUE && gModelYear >= 2010 &&
	     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
	{
		// Compression Ignition vehicles MY 2010 and later should not support INF8
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  SID $9 INF $8 response! (Not allowed for MY 2010 and later compression ignition vehicles)\n",
		     GetEcuId(EcuIndex) );
		return (FAIL);
	}

	// Check to see if IUMPR is supported for Compliance Type EOBD_NO_IUMPR, IOBD_NO_IUMPR, HD_IOBD_NO_IUMPR or OBDBr_NO_IUMPR
	if ( gUserInput.eComplianceType == EOBD_NO_IUMPR    ||
	     gUserInput.eComplianceType == IOBD_NO_IUMPR    ||
	     gUserInput.eComplianceType == HD_IOBD_NO_IUMPR ||
	     gUserInput.eComplianceType == OBDBr_NO_IUMPR )
	{
		Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  SID $9 INF $8 response! (Conflicts with Compliance Type selected)\n",
		     GetEcuId(EcuIndex) );
	}

	if (gOBDList[gOBDListIndex].Protocol == ISO15765)
	{
		// If MY 2019 and later spark ignition vehicle
		if ( gModelYear >= 2019 &&
		     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
		{
			// In-Use Performance Data must contain 56 bytes of data
			if ( gOBDResponse[EcuIndex].Sid9InfSize - 0x02 != 56 )
			{
				if ( gModelYear >= 2019 &&	gModelYear <= 2020 )
				{
					Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  SID $9 INF $8 (IPT) Data Size = %d (Must be 56 bytes!)\n",
					     GetEcuId(EcuIndex),
					     (gOBDResponse[EcuIndex].Sid9InfSize - 0x02) );
				}
				else
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  SID $9 INF $8 (IPT) Data Size = %d (Must be 56 bytes!)\n",
					     GetEcuId(EcuIndex),
					     (gOBDResponse[EcuIndex].Sid9InfSize - 0x02) );
					eResult = ERRORS;
				}
			}

			// SID9 INF8 NODI must equal 28 ($1C)
			if ( gOBDResponse[EcuIndex].Sid9Inf[1] != 0x1C )
			{
				if ( gModelYear >= 2019 &&	gModelYear <= 2020 )
				{
					Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  SID $9 INF $8 NODI = %d (Must be 28 ($14))\n",
					     GetEcuId(EcuIndex),
					     gOBDResponse[EcuIndex].Sid9Inf[1] );
				}
				else
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  SID $9 INF $8 NODI = %d (Must be 28 ($14))\n",
					     GetEcuId(EcuIndex),
					     gOBDResponse[EcuIndex].Sid9Inf[1] );
					eResult = ERRORS;
				}
			}
		}
		
		// If MY 2010 and later spark ignition vehicle
		else if ( gModelYear >= 2010 &&
		     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
		{
			// In-Use Performance Data must contain 40 bytes of data
			if ( gOBDResponse[EcuIndex].Sid9InfSize - 0x02 != 40 &&
			     gOBDResponse[EcuIndex].Sid9InfSize - 0x02 != 56 )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  SID $9 INF $8 (IPT) Data Size = %d (Must be 40 or 56 bytes!)\n",
				     GetEcuId(EcuIndex),
				     (gOBDResponse[EcuIndex].Sid9InfSize - 0x02) );
				eResult = ERRORS;
			}

			// SID9 INF8 NODI must equal $14
			if ( gOBDResponse[EcuIndex].Sid9Inf[1] != 0x14 &&
			     gOBDResponse[EcuIndex].Sid9Inf[1] != 0x1C )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  SID $9 INF $8 NODI = %d (Must be 20 ($14) or 28 ($1C))\n",
				     GetEcuId(EcuIndex),
				     gOBDResponse[EcuIndex].Sid9Inf[1] );
				eResult = ERRORS;
			}
		}
		else
		{
			// In-Use Performance Data must contain 32 or 40 bytes of data
			if ( gOBDResponse[EcuIndex].Sid9InfSize - 0x02 != 32 &&
			     gOBDResponse[EcuIndex].Sid9InfSize - 0x02 != 40 )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  SID $9 INF $8 (IPT) Data Size = %d (Must be 32 or 40 bytes!)\n",
				     GetEcuId(EcuIndex),
				     (gOBDResponse[EcuIndex].Sid9InfSize - 0x02) );
				return (FAIL);
			}

			// SID9 INF8 NODI must equal $10 or $14
			if ( gOBDResponse[EcuIndex].Sid9Inf[1] != 0x10 &&
			     gOBDResponse[EcuIndex].Sid9Inf[1] != 0x14 )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  SID $9 INF $8 NODI = %d (Must be 16 ($10) or 20 ($14))\n",
				     GetEcuId(EcuIndex),
				     gOBDResponse[EcuIndex].Sid9Inf[1] );
				return (FAIL);
			}
		}
	}
	else
	{
		/* For non-CAN, calculate the number of responses.  Expected is
		** 8 (32 bytes) or 10 (40 bytes) records.  Each response from controller held in
		** data structure SID9.
		*/
		if ( gOBDResponse[EcuIndex].Sid9InfSize / sizeof(SID9) != 0x08 &&
		     gOBDResponse[EcuIndex].Sid9InfSize / sizeof(SID9) != 0x0A )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $8 (IPT) Data Size Error (Must be 32 or 40 bytes!)\n",
			     GetEcuId(EcuIndex) );
			return (FAIL);
		}

		// SID9 INF8 message count must equal $08 or $10
		if ( gOBDResponse[EcuIndex].Sid9Inf[gOBDResponse[EcuIndex].Sid9InfSize - 5] != 0x08 && // check to message number byte
			 gOBDResponse[EcuIndex].Sid9Inf[gOBDResponse[EcuIndex].Sid9InfSize - 5] != 0x0A )  // check to message number byte
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $8 Message Count = %d (Must be 8 ($08) or 10 ($0A))\n",
			     GetEcuId(EcuIndex),
			     gOBDResponse[EcuIndex].Sid9Inf[gOBDResponse[EcuIndex].Sid9InfSize - 5] );
			return (FAIL);
		}
	}

	return(eResult);
}


/******************************************************************************
**
**	Function:   VerifyINFBData
**
**	Purpose:    Purpose of this function is to verify the INF B data
**	            for correct format.  In the event the format fails
**	            defined criteria, an error is returned.
**
*******************************************************************************
**
**	DATE        MODIFICATION
**	07/07/07    Isolated INF B verification logic
**
******************************************************************************/
STATUS VerifyINFBData ( unsigned long  EcuIndex )
{
	if ( gOBDDieselFlag == FALSE && gModelYear >= 2010 &&
	     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
	{
		// Spark Ignition vehicles MY 2010 and later should not support INFB
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  SID $9 INF $B response! (Not allowed for MY 2010 and later spark ignition vehicles)\n",
		     GetEcuId(EcuIndex) );
		return (FAIL);
	}

	// Check to see if IUMPR is supported for Compliance Type EOBD_NO_IUMPR, IOBD_NO_IUMPR, HD_IOBD_NO_IUMPR or OBDBr_NO_IUMPR
	if ( gUserInput.eComplianceType == EOBD_NO_IUMPR    ||
	     gUserInput.eComplianceType == IOBD_NO_IUMPR    ||
	     gUserInput.eComplianceType == HD_IOBD_NO_IUMPR ||
	     gUserInput.eComplianceType == OBDBr_NO_IUMPR )
	{
		Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  SID $9 INF $B response! (Conflicts with Compliance Type selected)\n",
		     GetEcuId(EcuIndex) );
	}

	/* 32 or 36 bytes of data returned */
	if (gOBDList[gOBDListIndex].Protocol == ISO15765)
	{
		// In-Use Performance Data must contain 32 or 36 bytes of data
		if ( gOBDResponse[EcuIndex].Sid9InfSize - 0x02 != 32 &&
		     gOBDResponse[EcuIndex].Sid9InfSize - 0x02 != 36 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $B (IPT) Data Size Error = %d (Must be 32 or 36 bytes!)\n",
			     GetEcuId(EcuIndex),
			     (gOBDResponse[EcuIndex].Sid9InfSize - 0x02) );
			return (FAIL);
		}

		if ( gOBDResponse[EcuIndex].Sid9InfSize - 0x02 == 32 &&
		     gOBDResponse[EcuIndex].Sid9Inf[1] != 0x10 )
		{
			// SID9 INFB NODI must equal 16 if the data size is 32
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $B NODI = %d (Must be 16)\n",
			     GetEcuId(EcuIndex),
			     gOBDResponse[EcuIndex].Sid9Inf[1] );
			return (FAIL);
		}

		if ( gOBDResponse[EcuIndex].Sid9InfSize - 0x02 == 36 &&
		     gOBDResponse[EcuIndex].Sid9Inf[1] != 0x12 )
		{
			// SID9 INFB NODI must equal 18 if the data size is 36
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $B NODI = %d (Must be 18)\n",
			     GetEcuId(EcuIndex),
			     gOBDResponse[EcuIndex].Sid9Inf[1] );
			return (FAIL);
		}
	}
	else
	{
		/* For non-CAN, calculate the number of responses.  Expected is
		** 8 (32 bytes) or 9 (36 bytes) records.  Each response from controller held in
		** data structure SID9.
		*/
		if ( gOBDResponse[EcuIndex].Sid9InfSize / sizeof(SID9) != 0x08 &&
		     gOBDResponse[EcuIndex].Sid9InfSize / sizeof(SID9) != 0x09 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $B (IPT) Data Size Error (Must be 32 or 36 bytes!)\n",
			     GetEcuId(EcuIndex) );
			return (FAIL);
		}
		// SID9 INFB message count must equal $08 or $09
		if ( gOBDResponse[EcuIndex].Sid9Inf[gOBDResponse[EcuIndex].Sid9InfSize - 5] != 0x08 && // check to message number byte
		     gOBDResponse[EcuIndex].Sid9Inf[gOBDResponse[EcuIndex].Sid9InfSize - 5] != 0x09 )  // check to message number byte
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $B Message Count = %d (Must be 8 ($08) or 9 ($09))\n",
			     GetEcuId(EcuIndex),
			     gOBDResponse[EcuIndex].Sid9Inf[gOBDResponse[EcuIndex].Sid9InfSize - 5] );
			return (FAIL);
		}
	}

	return PASS;
}


/******************************************************************************
**
**	Function:   VerifyInfDAndFFormat
**
**	Purpose:    Purpose of this function is to verify the ESN/EROTAN format
**	            for correct format.  In the event the format fails
**	            defined criteria, an error is returned.
**
*******************************************************************************/
STATUS VerifyInfDAndFFormat (unsigned long EcuIndex)
{
	unsigned int StartOffset = 0;
	unsigned int MsgIndex = 0; // multi-part message index
	unsigned long ByteIndex = 0;
	char rgcBuffer[21];
	STATUS RetCode = PASS;
	SID9 *pSid9;

	if (gOBDResponse[EcuIndex].Sid9InfSize == 0)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  No SID $9 INF $D data\n", GetEcuId(EcuIndex) );
		return FAIL;
	}

	pSid9 = (SID9 *)&gOBDResponse[EcuIndex].Sid9Inf[0];

	// copy to a protocol independent buffer
	if (gOBDList[gOBDListIndex].Protocol == ISO15765)
	{
		// ESN Data must contain 17 bytes of data
		if ( gOBDResponse[EcuIndex].Sid9InfSize - 0x02 != 17 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $%X Data Size Error = %d (Must be 17 bytes!)\n",
			     GetEcuId(EcuIndex),
			     pSid9->INF,
			     (gOBDResponse[EcuIndex].Sid9InfSize - 0x02) );
			return FAIL;
		}

		// SID9 INFC NODI must equal $01
		if ( pSid9->NumItems != 0x01 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $%X NODI = %d (Must be 1)\n",
			     GetEcuId(EcuIndex),
			     pSid9->INF,
			     pSid9->NumItems );
			RetCode = FAIL;
		}

		memcpy (rgcBuffer, &(pSid9->Data[0]), 20);
	}
	else
	{
		/* For non-CAN, calculate the number of responses.  Expected is
		** 5 (20 bytes) records.  Each response from controller held in
		** data structure SID9.
		*/
		if ( gOBDResponse[EcuIndex].Sid9InfSize / sizeof(SID9) != 0x05 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $%X Data Size Error (Must be 20 bytes!)\n",
			     GetEcuId(EcuIndex),
			     pSid9->INF );
			return (FAIL);
		}

		// SID9 INFC message count must equal $05
		if ( gOBDResponse[EcuIndex].Sid9Inf[gOBDResponse[EcuIndex].Sid9InfSize - 5] != 0x05 ) // check to message number byte
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $%X Message Count = %d (Must be 5)\n",
			     GetEcuId(EcuIndex),
			     pSid9->INF,
			     gOBDResponse[EcuIndex].Sid9Inf[gOBDResponse[EcuIndex].Sid9InfSize - 5] );
			RetCode = FAIL;
		}

		// save ESN data to buffer
		for ( MsgIndex = 0; MsgIndex < (gOBDResponse[EcuIndex].Sid9InfSize / sizeof(SID9)); MsgIndex++ )
		{
			if (MsgIndex == 0)
			{
				/* get rid of the three leading NULLs in the first message */
				rgcBuffer[0] = pSid9[0].Data[3];
			}
			else
			{
				/* extract the data from the remaining messages */
				memcpy (&(rgcBuffer[1 + ((MsgIndex - 1) * 4)]), &(pSid9[MsgIndex].Data[0]), 4);
			}
		}
	}

	/* strip of the pad bytes */
	for (ByteIndex = 0; ByteIndex < 17; ByteIndex++)
	{
		if (rgcBuffer[ByteIndex] != 0x00)
		{
			StartOffset = ByteIndex;
			break;
		}
	}

	if (ByteIndex == 17)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  SID $9 INF $%X contains all pad bytes\n",
		     GetEcuId(EcuIndex),
		     pSid9->INF );
		RetCode = FAIL;
	}
	else
	{
		/* Check all ESN characters for validity */
		for (; ByteIndex < 17; ByteIndex++)
		{
			if ((rgcBuffer[ByteIndex] <  '0') || (rgcBuffer[ByteIndex] >  'Z') ||
			    (rgcBuffer[ByteIndex] == 'I') || (rgcBuffer[ByteIndex] == 'O') ||
			    (rgcBuffer[ByteIndex] == 'Q') || (rgcBuffer[ByteIndex] == ':') ||
			    (rgcBuffer[ByteIndex] == ';') || (rgcBuffer[ByteIndex] == '<') ||
			    (rgcBuffer[ByteIndex] == '>') || (rgcBuffer[ByteIndex] == '=') ||
			    (rgcBuffer[ByteIndex] == '?') || (rgcBuffer[ByteIndex] == '@'))
			{
				break;
			}
		}

		// NULL terminate string
		rgcBuffer[ByteIndex] = 0x00;

		if (ByteIndex != 17)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  Invalid or missing SID $9 INF $%X data\n",
			     GetEcuId(EcuIndex),
			     pSid9->INF );
			RetCode = FAIL;
		}
	}

	// Print ESN/EROTAN string to log file
	Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT, "ECU %X  %s = %s\n",
	     GetEcuId(EcuIndex),
	     pSid9->INF == 0x0D ? "ESN" : "EROTAN",
	     &rgcBuffer[StartOffset]);

	return RetCode;
}


/******************************************************************************
**
**	Function:   VerifyInf12Format
**
**	Purpose:    Purpose of this function is to verify the FEOCNTR format
**	            for correct format.  In the event the format fails
**	            defined criteria, an error is returned.
**
*******************************************************************************/
STATUS VerifyInf12Format (unsigned long EcuIndex)
{
	unsigned int StartOffset = 0;
	unsigned int MsgIndex = 0; // multi-part message index
	unsigned long ByteIndex = 0;
	char rgcBuffer[21];
	STATUS RetCode = PASS;
	SID9 *pSid9;

	if (gOBDResponse[EcuIndex].Sid9InfSize == 0)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  No SID $9 INF $12 data\n", GetEcuId(EcuIndex) );
		return FAIL;
	}

	pSid9 = (SID9 *)&gOBDResponse[EcuIndex].Sid9Inf[0];

	// copy to a protocol independent buffer
	if (gOBDList[gOBDListIndex].Protocol == ISO15765)
	{
		// FEOCNTR Data must contain 5 bytes of data
		if ( gOBDResponse[EcuIndex].Sid9InfSize - 0x02 != 2 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $%X Data Size Error = %d (Must be 2 bytes!)\n",
			     GetEcuId(EcuIndex),
			     pSid9->INF,
			     (gOBDResponse[EcuIndex].Sid9InfSize - 0x02) );
			return FAIL;
		}

		// SID9 INF12 NODI must equal $01
		if ( pSid9->NumItems != 0x01 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $%X NODI = %d (Must be 1)\n",
			     GetEcuId(EcuIndex),
			     pSid9->INF,
			     pSid9->NumItems );
			RetCode = FAIL;
		}

		memcpy (rgcBuffer, &(pSid9->Data[0]), 20);
	}

	return RetCode;
}


/******************************************************************************
**
**	Function:   VerifyInf12Data
**
**	Purpose:    Purpose of this function is to verify the FEOCNTR data
**	            In the event the data fails defined criteria,
**             an error is returned.
**
*******************************************************************************/
STATUS VerifyInf12Data (unsigned long EcuIndex, unsigned long IGNCNTR)
{
	STATUS RetCode = PASS;
	SID9 *pSid9;
	unsigned long FEOCNTR;

	if (gOBDResponse[EcuIndex].Sid9InfSize == 0)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  No SID $9 INF $12 data\n", GetEcuId(EcuIndex) );
		return FAIL;
	}

	pSid9 = (SID9 *)&gOBDResponse[EcuIndex].Sid9Inf[0];

	// copy to a protocol independent buffer
	if (gOBDList[gOBDListIndex].Protocol == ISO15765)
	{
		// FEOCNTR Data must contain 2 bytes of data
		if ( gOBDResponse[EcuIndex].Sid9InfSize - 0x02 != 2 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $%X Data Size Error = %d (Must be 2 bytes!)\n",
			     GetEcuId(EcuIndex),
			     pSid9->INF,
			     (gOBDResponse[EcuIndex].Sid9InfSize - 0x02) );
			return FAIL;
		}

		// SID9 INF12 NODI must equal $01
		if ( pSid9->NumItems != 0x01 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $%X NODI = %d (Must be 1)\n",
			     GetEcuId(EcuIndex),
			     pSid9->INF,
			     pSid9->NumItems );
			RetCode = FAIL;
		}

		FEOCNTR = (gOBDResponse[EcuIndex].Sid9Inf[2] * 256) + gOBDResponse[EcuIndex].Sid9Inf[3];

		// log FEOCNTR
		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  SID $9 INF $12 FEOCNTR = %d\n",
		     GetEcuId (EcuIndex),
		     FEOCNTR );

		if ( FEOCNTR > IGNCNTR )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  FEOCNTR value cannot be greater than IGNCNTR\n",
			     GetEcuId(EcuIndex) );
			RetCode = FAIL;
		}

	}

	return RetCode;
}


/******************************************************************************
**
**	Function:   VerifyInf14Data
**
**	Purpose:    Purpose of this function is to verify the EVAP_DIST data
**	            In the event the data fails defined criteria,
**             an error is returned.
**
*******************************************************************************/
STATUS VerifyInf14Data (unsigned long EcuIndex)
{
	STATUS RetCode = PASS;
	SID9 *pSid9;
	unsigned long EVAP_DIST;

	if (gOBDResponse[EcuIndex].Sid9InfSize == 0)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  No SID $9 INF $14 data\n", GetEcuId(EcuIndex) );
		return FAIL;
	}

	pSid9 = (SID9 *)&gOBDResponse[EcuIndex].Sid9Inf[0];

	// copy to a protocol independent buffer
	if (gOBDList[gOBDListIndex].Protocol == ISO15765)
	{
		// EVAP_DIST Data must contain 2 bytes of data
		if ( gOBDResponse[EcuIndex].Sid9InfSize - 0x02 != 2 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $%X Data Size Error = %d (Must be 2 bytes!)\n",
			     GetEcuId(EcuIndex),
			     pSid9->INF,
			     (gOBDResponse[EcuIndex].Sid9InfSize - 0x02) );
			return FAIL;
		}

		// SID9 INF14 NODI must equal $01
		if ( pSid9->NumItems != 0x01 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $%X NODI = %d (Must be 1)\n",
			     GetEcuId(EcuIndex),
			     pSid9->INF,
			     pSid9->NumItems );
			RetCode = FAIL;
		}

		EVAP_DIST = (gOBDResponse[EcuIndex].Sid9Inf[2] * 256) + gOBDResponse[EcuIndex].Sid9Inf[3];

		// log FEOCNTR
		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  SID $9 INF $%X EVAP_DIST = %d\n",
		     GetEcuId (EcuIndex),
			 pSid9->INF,
		     EVAP_DIST );

		// if tests 11.7
		if ( TestPhase == eTestPerformanceCounters && TestSubsection == 7 && EVAP_DIST == 0xFFFF )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $%X should not return a value of $FFFF after evap monitor runs\n",
			     GetEcuId(EcuIndex),
			     pSid9->INF );
			RetCode = FAIL;
		}
		// if tests 5.17
		else if ( TestPhase == eTestNoDTC && TestSubsection == 17 && EVAP_DIST != 0xFFFF )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF $%X should return a value of $FFFF after a Service $04 code clear\n",
			     GetEcuId(EcuIndex),
			     pSid9->INF );
			RetCode = FAIL;
		}
	}

	return RetCode;
}


//*****************************************************************************
//	Function:   VerifySid9PidSupportData
//
//	Purpose:    Verify each controller supports at a minimum one PID.
//	            Any ECU that responds that does not support at least
//	            one PID is flagged as an error.
//
//*****************************************************************************
int VerifySid9PidSupportData (void)
{
	int             bReturn = PASS;
	int             bEcuResult;
	unsigned long   EcuIndex;
	unsigned long   Index;

	/* For each ECU */
	for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
	{
		bEcuResult = FAIL;
		for (Index = 0; Index < gOBDResponse[EcuIndex].Sid9InfSupportSize; Index++)
		{
			/* If INF is supported, keep looking */
			if ( ( gOBDResponse[EcuIndex].Sid9InfSupport[Index].IDBits[0] ||
			       gOBDResponse[EcuIndex].Sid9InfSupport[Index].IDBits[1] ||
			       gOBDResponse[EcuIndex].Sid9InfSupport[Index].IDBits[2] ||
			     ( gOBDResponse[EcuIndex].Sid9InfSupport[Index].IDBits[3] & 0xFE ) ) != 0x00)
			{
				bEcuResult = PASS;
			}
		}

		if ((bEcuResult == FAIL) && (gOBDResponse[EcuIndex].Sid9InfSupportSize > 0))
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $9 INF-Supported INFs indicate no INFs supported", GetEcuId(EcuIndex) );
			bReturn = FAIL;
		}
	}

	return bReturn;
}

//*****************************************************************************
//
//	Function:   IsSid9InfSuported
//
//	Purpose:    Determine if SID 9 INF is supported on specific ECU.
//              Need to have called RequestSID9SupportData() previously.
//              If EcuIndex < 0 then check all ECUs.
//
//*****************************************************************************
//
//	DATE        MODIFICATION
//	02/10/05    Created common function for this logic.
//
//*****************************************************************************
unsigned int IsSid9InfSupported (unsigned int EcuIndex, unsigned int InfIndex)
{
	unsigned int index0;
	unsigned int index1;
	unsigned int index2;
	unsigned int mask;

	if (InfIndex == 0)
		return TRUE;            // all modules must support SID 09 INF 00

	InfIndex--;

	index1 =  InfIndex >> 5;
	index2 = (InfIndex >> 3) & 0x03;
	mask   = 0x80 >> (InfIndex & 0x07);

	if ((signed int)EcuIndex < 0)
	{
		for (index0 = 0; index0 < gUserNumEcus; index0++)
		{
			if (gOBDResponse[index0].Sid9InfSupport[index1].IDBits[index2] & mask)
				return TRUE;
		}
	}
	else
	{
		if (gOBDResponse[EcuIndex].Sid9InfSupport[index1].IDBits[index2] & mask)
			return TRUE;
	}

	return FALSE;
}
