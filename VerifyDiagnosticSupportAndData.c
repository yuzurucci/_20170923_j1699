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
**    DATE      MODIFICATION
**    06/14/04  Removed Mode $01 PID $01 / PID $41 comparison.  Implement
**              PID $01 check logic defined in specification
********************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "j2534.h"
#include "j1699.h"

#define MAX_PIDS 0x100

/*
 * SID $01 PID $01 Bit defines - for tracking support when
 * 'at least one ECU' must have these bits set
 */
#define DATA_B_BIT_0 0x00010000
#define DATA_B_BIT_1 0x00020000
#define DATA_B_BIT_2 0x00040000
#define DATA_C_BIT_0 0x00000100
#define DATA_C_BIT_2 0x00000400
#define DATA_C_BIT_3 0x00000800
#define DATA_C_BIT_5 0x00002000
#define DATA_C_BIT_6 0x00004000
#define DATA_C_BIT_7 0x00008000



STATUS VerifyM01P01 (SID1 *pSid1, unsigned long SidIndex, unsigned long EcuIndex, unsigned long *PidSupported);

STATUS GetPid4FArray (void);
STATUS GetPid50Array (void);

void GetHoursMinsSecs( unsigned long time, unsigned long *hours, unsigned long *mins, unsigned long *secs);

unsigned char Pid4F[OBD_MAX_ECUS][4];
unsigned char Pid50[OBD_MAX_ECUS][4];

SID1 Sid1Pid1[OBD_MAX_ECUS];    // capture the response from SID01 PID01 response.


/*
*******************************************************************************
** VerifyDiagnosticSupportAndData -
** Function to verify SID1 diagnostic support and data
*******************************************************************************
*/
STATUS VerifyDiagnosticSupportAndData( unsigned char OBDEngineDontCare )
{
	unsigned long EcuIndex;
	unsigned long IdIndex;
	SID_REQ SidReq;
	SID1 *pSid1;
	unsigned long SidIndex;
	unsigned long ulInit_FailureCount = 0;
	unsigned long ulTemp_FailureCount = 0;
	long temp_data_long;
	float temp_data_float;
	float temp_EGR_CMD;
	float temp_EGR_ACT;
	unsigned int BSIndex;
	unsigned char fPid13Supported = FALSE;      // set if the current ECU supports SID $1 PID $13
	unsigned char fPid1DSupported = FALSE;      // set if the current ECU supports SID $1 PID $1D
	unsigned char fPid4FSupported = FALSE;      // set if the current ECU supports SID $1 PID $4F
	unsigned char fPid50Supported = FALSE;      // set if the current ECU supports SID $1 PID $50
	unsigned char fReqPidNotSupported = FALSE;  // set if a required PID is not supported
	unsigned long fPidSupported[MAX_PIDS];      // an array of PIDs (TRUE if PID is supported)

	                                            // However, PID $01 is different as we must track
	                                            // support for specific bits. In this case, the ULONG
	                                            // ULONG will be structured so that DATA_A is the
	                                            // most significant byte and DATA_D is the least
	                                            // significant byte (i.e., AABBCCDD)
	unsigned long ulIMBitsForVehicle = 0;       // holds the SID1 PID1 IM bits supported by the vehicle
	unsigned char fPid9FSuccess = FALSE;
	unsigned long hours;
	unsigned long mins;
	unsigned long secs;


	ulInit_FailureCount = GetFailureCount();

	/* Read SID 1 PID support PIDs */
	if (RequestSID1SupportData () != PASS)
	{
		if ( Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT,
		          "Problems verifying PID supported data.\n" ) == 'N' )
		{
			return (FAIL);
		}
	}

	/* Per J1699 rev 11.5 TC# 5.10.5 - Verify ECU did not drop out.	*/
	if (VerifyLinkActive() != PASS)
	{
		if ( Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT,
		          "Problems verifying link active.\n" ) == 'N' )
		{
			return (FAIL);
		}
	}

	/* Determine size of PIDs $06, $07, $08, $09 */
	if (DetermineVariablePidSize () != PASS)
	{
		if ( Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT,
		          "Problems determining the size of variable PIDs.\n" ) == 'N' )
		{
			return (FAIL);
		}
	}

	if (GetPid4FArray() != PASS)
	{
		if ( Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT,
		          "Problems getting PID $4F.\n" ) == 'N' )
		{
			return (FAIL);
		}
	}

	if (GetPid50Array() != PASS)
	{
		if ( Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT,
		          "Problems getting PID $50.\n" ) == 'N' )
		{
			return (FAIL);
		}
	}


	/* For each PID */
	for (IdIndex = 0x01; IdIndex < MAX_PIDS; IdIndex++)
	{
		/* clear PID supported indicator */
		fPidSupported[IdIndex] = FALSE;

		/* skip PID supported PIDs */
		if (IdIndex == 0x20 || IdIndex == 0x40 || IdIndex == 0x60 || IdIndex == 0x80 ||
		    IdIndex == 0xA0 || IdIndex == 0xC0 || IdIndex == 0xE0)
		{
			continue;
		}

		if (IsSid1PidSupported (-1, IdIndex) == FALSE)
		{
			continue;
		}

		SidReq.SID = 1;
		SidReq.NumIds = 1;
		SidReq.Ids[0] = (unsigned char)IdIndex;
		if ( SidRequest(&SidReq, SID_REQ_NORMAL) == FAIL )
		{
			/* There must be a response for ISO15765 protocol */
			if (gOBDList[gOBDListIndex].Protocol == ISO15765)
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "SID $1 PID $%02X request\n", IdIndex );
				continue;
			}
		}

		for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
		{
			if (gOBDResponse[EcuIndex].Sid1PidSize != 0)
			{
				break;
			}
		}

		if (EcuIndex >= gOBDNumEcus)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "No SID $1 PID $%02X data\n", IdIndex );
			continue;
		}

		// get Failure Count to allow for FAILURE checks from this point on
		ulTemp_FailureCount = GetFailureCount();

		/* Verify that all SID 1 PID data is valid */
		for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
		{
			/* Set ECU dependent PID flags */
			fPid13Supported = IsSid1PidSupported (EcuIndex, 0x13);
			fPid1DSupported = IsSid1PidSupported (EcuIndex, 0x1D);
			fPid4FSupported = IsSid1PidSupported (EcuIndex, 0x4F);
			fPid50Supported = IsSid1PidSupported (EcuIndex, 0x50);

			/* If PID is supported, check it */
			if (IsSid1PidSupported (EcuIndex, IdIndex) == TRUE)
			{
				SidIndex = 0;

				/* Check the data to see if it is valid */
				pSid1 = (SID1 *)&gOBDResponse[EcuIndex].Sid1Pid[0];
				if ( gOBDResponse[EcuIndex].Sid1PidSize == 0 )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  PID $%02X supported but no data\n",
					     GetEcuId(EcuIndex),
					     IdIndex );
				}

				else
				{
					/* save indication that PID is supported */
					fPidSupported[IdIndex] = TRUE;

					/* Check various PID values for validity based on vehicle state */
					switch(pSid1[SidIndex].PID)
					{
						case 0x01:
						{
							// Capture the response from PID $01!
							// for use with SID 01 PID 41 and
							// SID $06 MID verification logic
							memcpy( &Sid1Pid1[EcuIndex], &pSid1[SidIndex], sizeof( SID1 ) );

							if ( OBDEngineDontCare == FALSE) // NOT Test 10.2
							{
								if ( VerifyM01P01( pSid1, SidIndex, EcuIndex, &ulIMBitsForVehicle) != PASS )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "SID $1 PID $1 failures detected.\n" );
								}
							}
						}
						break;
						case 0x02:
						{
							temp_data_long = (pSid1[SidIndex].Data[0] * 256) +
							                  pSid1[SidIndex].Data[1];
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  DTCFRZF = $%04X\n",
							     GetEcuId(EcuIndex),
							     temp_data_long );

							if ( OBDEngineDontCare == FALSE && gOBDEngineWarm == FALSE )  // test 5.6 & 5.10
							{
								if ((pSid1[SidIndex].Data[0] != 0x00) ||
								    (pSid1[SidIndex].Data[1] != 0x00))
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  DTCFRZF = $%04X, Freeze frames available\n",
									     GetEcuId(EcuIndex),
									     temp_data_long );
								}
							}
						}
						break;
						case 0x03:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  FUELSYSA = $%02X  FUELSYSB = $%02X\n",
							     GetEcuId(EcuIndex),
							     pSid1[SidIndex].Data[0],
							     pSid1[SidIndex].Data[1] );

							if ( OBDEngineDontCare == FALSE && gOBDEngineRunning == FALSE &&  // if Test 5.6	AND
							     ( (pSid1[SidIndex].Data[0] & 0x02) != 0 ||                   // FUELSYSA Closed Loop OR
							       (pSid1[SidIndex].Data[1] & 0x02) != 0 ) )                  // FUELSYSB Closed Loop
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FUELSYSA Bit 1 and FUELSYSB Bit 1 must both be 0 (not Closed Loop)\n",
								     GetEcuId(EcuIndex) );
							}
						}
						break;
						case 0x04:
						{
							temp_data_float = (float)pSid1[SidIndex].Data[0] * (float)(100.0 / 255.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  LOAD_PCT = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

							if ( OBDEngineDontCare == FALSE )  // NOT Test 10.2
							{
								if ( gOBDEngineRunning == TRUE )  // test 5.10 & 10.13
								{
									if (gOBDHybridFlag != TRUE)
									{
										/* don't range check hybrids at idle, as LOAD_PCT could be up to 100% */
										if ( (temp_data_float == 0 ) || (temp_data_float > 60) )
										{
											/* Load should be 60% or less with engine running */
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  LOAD_PCT = 0 OR LOAD_PCT > 60 %%\n", GetEcuId(EcuIndex) );
										}
									}
								}
								else  // test 5.6
								{
									if ( temp_data_float != 0 )
									{
										/* There should be no load with the engine OFF */
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  LOAD_PCT > 0 %%\n", GetEcuId(EcuIndex) );
									}
								}
							}
						}
						break;
						case 0x05:
						{
							temp_data_long = pSid1[SidIndex].Data[0] - 40;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  ECT = %d C\n", GetEcuId(EcuIndex), temp_data_long);

							if ((temp_data_long < -20) || (temp_data_long > 120))
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  ECT exceeded normal range\n", GetEcuId(EcuIndex) );
							}
						}
						break;
						case 0x06:
						{
							temp_data_float = (float)(pSid1[SidIndex].Data[0]) * (float)(100.0/128.0) - (float)100.0;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SHRTFT1 = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

							if ( OBDEngineDontCare == FALSE && gOBDEngineWarm == TRUE )  // test 10.13
							{
								if (temp_data_float < -50.0 || temp_data_float > 50.0)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  SHRTFT1 out of range\n", GetEcuId(EcuIndex) );
								}
							}

							if (gSid1VariablePidSize == 2)
							{
								temp_data_float = (float)(pSid1[SidIndex].Data[1]) * (float)(100.0/128.0) - (float)100.0;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  SHRTFT3 = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

								if ( OBDEngineDontCare == FALSE && gOBDEngineWarm == TRUE )  // test 10.13
								{
									if (temp_data_float < -50.0 || temp_data_float > 50.0)
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  SHRTFT3 out of range\n", GetEcuId(EcuIndex) );
									}
								}
							}

						}
						break;
						case 0x07:
						{
							temp_data_float = (float)(pSid1[SidIndex].Data[0]) * (float)(100.0/128.0) - (float)100.0;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  LONGFT1 = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

							if ( OBDEngineDontCare == FALSE && gOBDEngineWarm == TRUE )  // test 10.13
							{
								if (temp_data_float < -50.0 || temp_data_float > 50.0)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  LONGFT1 out of range\n", GetEcuId(EcuIndex) );
								}
							}

							if (gSid1VariablePidSize == 2)
							{
								temp_data_float = (float)(pSid1[SidIndex].Data[1]) * (float)(100.0/128.0) - (float)100.0;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  LONGFT3 = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

								if ( OBDEngineDontCare == FALSE && gOBDEngineWarm == TRUE )  // test 10.13
								{
									if (temp_data_float < -50.0 || temp_data_float > 50.0)
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  LONGFT3 out of range\n", GetEcuId(EcuIndex) );
									}
								}
							}

						}
						break;
						case 0x08:
						{
							temp_data_float = (float)(pSid1[SidIndex].Data[0]) * (float)(100.0/128.0) - (float)100.0;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SHRTFT2 = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

							if ( OBDEngineDontCare == FALSE && gOBDEngineWarm == TRUE )  // test 10.13
							{
								if (temp_data_float < -50.0 || temp_data_float > 50.0)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  SHRTFT2 out of range\n", GetEcuId(EcuIndex) );
								}
							}

							if (gSid1VariablePidSize == 2)
							{
								temp_data_float = (float)(pSid1[SidIndex].Data[1]) * (float)(100.0/128.0) - (float)100.0;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  SHRTFT4 = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

								if ( OBDEngineDontCare == FALSE && gOBDEngineWarm == TRUE )  // test 10.13
								{
									if (temp_data_float < -50.0 || temp_data_float > 50.0)
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  SHRTFT4 out of range\n", GetEcuId(EcuIndex) );
									}
								}
							}
						}
						break;
						case 0x09:
						{
							temp_data_float = (float)(pSid1[SidIndex].Data[0]) * (float)(100.0/128.0) - (float)100.0;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  LONGFT2 = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

							if ( OBDEngineDontCare == FALSE && gOBDEngineWarm == TRUE )  // test 10.13
							{
								if (temp_data_float < -50.0 || temp_data_float > 50.0)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  LONGFT2 out of range\n", GetEcuId(EcuIndex) );
								}
							}

							if (gSid1VariablePidSize == 2)
							{
								temp_data_float = (float)(pSid1[SidIndex].Data[1]) * (float)(100.0/128.0) - (float)100.0;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  LONGFT4 = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

								if ( OBDEngineDontCare == FALSE && gOBDEngineWarm == TRUE )  // test 10.13
								{
									if (temp_data_float < -50.0 || temp_data_float > 50.0)
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  LONGFT4 out of range\n", GetEcuId(EcuIndex) );
									}
								}
							}
						}
						break;
						case 0x0A:
						{
							temp_data_long = pSid1[SidIndex].Data[0] * 3;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  FRP = %u kPa\n", GetEcuId(EcuIndex), temp_data_long);

							if ( OBDEngineDontCare == FALSE && gOBDEngineRunning == TRUE )  // test 5.10 & 10.13
							{
								if ( (temp_data_long == 0) && (gOBDHybridFlag != TRUE) )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  FRP must be greater than 0\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;
						case 0x0B:
						{
							temp_data_float = (float)(pSid1[SidIndex].Data[0]);
							if ( fPid4FSupported == TRUE && Pid4F[EcuIndex][3]!=0 )
							{
								temp_data_float = temp_data_float * ((float)(Pid4F[EcuIndex][3]*10)/255);
							}
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  MAP = %.1f kPa\n", GetEcuId(EcuIndex), temp_data_float);

							if ( OBDEngineDontCare == FALSE && gOBDEngineRunning == TRUE )  // test 5.10 & 10.13
							{
								if ( (temp_data_float == 0.0) && (gOBDHybridFlag != TRUE) )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  MAP must be greater than 0\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;
						case 0x0C:
						{
							temp_data_long = (((pSid1[SidIndex].Data[0] * 256) + pSid1[SidIndex].Data[1]) / 4);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  RPM = %d rpm\n", GetEcuId(EcuIndex), temp_data_long);

							if ( OBDEngineDontCare == FALSE )  // NOT Test 10.2
							{
								if ( gOBDEngineRunning == TRUE )  // test 5.10 & 10.13
								{
									/* Per J1699 rev 11.6 - table 41 */
									if ( (temp_data_long > 2000) || (temp_data_long < 300) )
									{
										if ( !((temp_data_long == 0) && (gOBDHybridFlag == TRUE)) )
										{
											/* Idle RPM is outside the reasonable range */
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  RPM exceeded normal range\n", GetEcuId(EcuIndex) );
										}
									}
								}
								else  // test 5.6
								{
									if (temp_data_long != 0)
									{
										/* There should be no RPM with the engine OFF */
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  RPM > 0 rpm\n", GetEcuId(EcuIndex) );
									}
								}
							}
						}
						break;
						case 0x0D:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  VSS = %d km/h\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0]);

							if (pSid1[SidIndex].Data[0] != 0x00)
							{
								/* There should be no vehicle speed when not moving */
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  VSS > 0 km/h\n", GetEcuId(EcuIndex) );
							}
						}
						break;
						case 0x0E:
						{
							temp_data_float = ((float)pSid1[SidIndex].Data[0] / (float)2.0) - (float)64.0;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SPARKADV = %.1f deg\n", GetEcuId(EcuIndex), temp_data_float);

							if ( OBDEngineDontCare == FALSE && gOBDEngineWarm == TRUE )  // test 10.13
							{
								if ((temp_data_float < -35) || (temp_data_float > 55))
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  SPARKADV exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;
						case 0x0F:
						{
							temp_data_long = pSid1[SidIndex].Data[0] - 40;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  IAT = %d C\n", GetEcuId(EcuIndex), temp_data_long);

							if ( (temp_data_long < -20) || (temp_data_long > 120) )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  IAT exceeded normal range\n", GetEcuId(EcuIndex) );
							}
						}
						break;
						case 0x10:
						{
							temp_data_float = (float)((pSid1[SidIndex].Data[0] * 256) +
							                          pSid1[SidIndex].Data[1]);
							if ( fPid50Supported == TRUE && Pid50[EcuIndex][0] != 0 )
							{
								temp_data_float = temp_data_float * ((float)(Pid50[EcuIndex][0] * 10) / 65535);
							}
							else
							{
								temp_data_float = temp_data_float / (float)100.0;
							}
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  MAF = %.2f g/s\n", GetEcuId(EcuIndex), temp_data_float);

							if ( OBDEngineDontCare == FALSE )  // NOT Test 10.2
							{
								if ( gOBDEngineRunning == TRUE )  // test 5.10 & 10.13
								{
									if ( (temp_data_float == 0.0) && (gOBDHybridFlag != TRUE) )
									{
										/* MAF should not be zero with the engine running */
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  MAF = 0 g/s\n", GetEcuId(EcuIndex) );
									}
								}
								else  // test 5.6
								{
									/*
									** J1699 version 11.6 table 23, engine off update.
									*/
									if (temp_data_float > 5.0)
									{
										/* MAF should be zero with the engine OFF */
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  MAF > 5 g/s\n", GetEcuId(EcuIndex) );
									}
								}
							}
						}
						break;
						case 0x11:
						{
							temp_data_float = (float)pSid1[SidIndex].Data[0] * (float)(100.0 / 255.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  TP = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

							if (gOBDDieselFlag == 0)
							{
								// non-diesel
								if (temp_data_float > 40)
								{
									/*
									** Throttle position should be
									** 40% or less when not driving
									*/
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  TP > 40 %%\n", GetEcuId(EcuIndex) );
								}
							}
							else
							{
								// diesel
								if (temp_data_float > 100)
								{
									/*
									** Throttle position should be
									** 100% or less when not driving
									*/
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  TP > 100 %%\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;

						case 0x12:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  AIR_STAT = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
						}
						break;

						/*
						case 0x13:  See 0x1D
						*/

						case 0x14:
						case 0x15:
						case 0x16:
						case 0x17:
						case 0x18:
						case 0x19:
						case 0x1A:
						case 0x1B:
						{
							switch (pSid1[SidIndex].PID)
							{
								case 0x14:
									BSIndex = 0x11;
								break;
								case 0x15:
									BSIndex = 0x12;
								break;
								case 0x16:
									BSIndex = fPid13Supported ? 0x13 : 0x21;
								break;
								case 0x17:
									BSIndex = fPid13Supported ? 0x14 : 0x22;
								break;
								case 0x18:
									BSIndex = fPid13Supported ? 0x21 : 0x31;
								break;
								case 0x19:
									BSIndex = fPid13Supported ? 0x22 : 0x32;
								break;
								case 0x1A:
									BSIndex = fPid13Supported ? 0x23 : 0x41;
								break;
								case 0x1B:
									BSIndex = fPid13Supported ? 0x24 : 0x42;
								break;
							}

							temp_data_float = (float)pSid1[SidIndex].Data[0] * (float).005;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  O2S%x = %.3f V\n", GetEcuId(EcuIndex), BSIndex, temp_data_float);

							temp_data_float = ((float)(pSid1[SidIndex].Data[1]) * (float)(100.0 / 128.0)) - (float)100.0;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SHRTFT%x = %.1f %%\n", GetEcuId(EcuIndex), BSIndex, temp_data_float);
						}
						break;

						case 0x1C:
						{
							/* Make sure value is in the valid range before the lookup */
							if (pSid1[SidIndex].Data[0] > MAX_OBD_TYPES)
							{
								pSid1[SidIndex].Data[0] = 0;
							}

							/* Updated the following supported IDs per J1699 V11.5 */
							/* Vehicle should support OBD-II */
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  OBD_TYPE = %s\n", GetEcuId(EcuIndex), OBD_TYPE[pSid1[SidIndex].Data[0]]);

							if ( gUserInput.eComplianceType == US_OBDII )
							{
								if ( pSid1[SidIndex].Data[0] != 0x01 &&     /* CARB OBD II */
								     pSid1[SidIndex].Data[0] != 0x03 &&     /* OBDI and OBD II */
								     pSid1[SidIndex].Data[0] != 0x07 &&     /* EOBD and OBD II */
								     pSid1[SidIndex].Data[0] != 0x09 &&     /* EOBD, OBD and OBD II */
								     pSid1[SidIndex].Data[0] != 0x0B &&     /* JOBD and OBD II */
								     pSid1[SidIndex].Data[0] != 0x0D &&     /* JOBD, EOBD, and OBD II */
								     pSid1[SidIndex].Data[0] != 0x0F &&
								     pSid1[SidIndex].Data[0] != 0x22 )      /* OBD, OBD II and HD_OBD */
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  Not a Light Duty OBD II ECU\n", GetEcuId(EcuIndex) );
								}
							}
							else if ( gUserInput.eComplianceType == HD_OBD )
							{
								if ( pSid1[SidIndex].Data[0] != 0x13 &&     /* HD OBD-C */
								     pSid1[SidIndex].Data[0] != 0x14 &&     /* HD OBD */
								     pSid1[SidIndex].Data[0] != 0x22 )      /* OBD, OBD II and HD_OBD */
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  Not a Heavy Duty OBD II ECU\n", GetEcuId(EcuIndex) );
								}
							}
							else if ( gUserInput.eComplianceType == EOBD_WITH_IUMPR ||
							          gUserInput.eComplianceType == EOBD_NO_IUMPR )
							{
								if ( pSid1[SidIndex].Data[0] != 0x06 &&     /* EOBD */
								     pSid1[SidIndex].Data[0] != 0x07 &&     /* EOBD and OBD II */
								     pSid1[SidIndex].Data[0] != 0x08 &&     /* EOBD and OBD */
								     pSid1[SidIndex].Data[0] != 0x09 &&     /* EOBD, OBD and OBD II */
								     pSid1[SidIndex].Data[0] != 0x0C &&     /* JOBD and EOBD */
								     pSid1[SidIndex].Data[0] != 0x0D &&     /* JOBD, EOBD, and OBD II */
								     pSid1[SidIndex].Data[0] != 0x0E &&     /* EURO IV B1 */
								     pSid1[SidIndex].Data[0] != 0x0F )      /* EURO V B2 */
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  Not a Light Duty EOBD ECU\n", GetEcuId(EcuIndex) );
								}
							}
							else if ( gUserInput.eComplianceType == HD_EOBD )
							{
								if ( pSid1[SidIndex].Data[0] != 0x17 &&     /* HD EOBD-I */
								     pSid1[SidIndex].Data[0] != 0x18 &&     /* HD EOBD-I N */
								     pSid1[SidIndex].Data[0] != 0x19 &&     /* HD EOBD-II */
								     pSid1[SidIndex].Data[0] != 0x1A &&     /* HD EOBD-II N */
								     pSid1[SidIndex].Data[0] != 0x21 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  Not a Heavy Duty EOBD ECU\n", GetEcuId(EcuIndex) );
								}
							}
							else if ( gUserInput.eComplianceType == IOBD_NO_IUMPR || gUserInput.eComplianceType == HD_IOBD_NO_IUMPR )
							{
								if ( pSid1[SidIndex].Data[0] != 0x20 )      /* IOBD */
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  Not a IOBD II ECU\n", GetEcuId(EcuIndex) );
								}
							}
							else if ( gUserInput.eComplianceType == OBDBr_NO_IUMPR )
							{
								if ( pSid1[SidIndex].Data[0] != 0x1C &&    /* OBDBr-1 */
								     pSid1[SidIndex].Data[0] != 0x1D &&    /* OBDBr-2 */
								     pSid1[SidIndex].Data[0] != 0x23 &&    /* OBDBr-3 */
								     pSid1[SidIndex].Data[0] != 0x2A )     /* OBDBr-D */
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  Not a OBDBr ECU\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;

						case 0x13:
						case 0x1D:
						{
							unsigned short O2Bit;
							unsigned short O2Count;

							/* Identify support for PID 0x13 / 0x1D */
							if ( pSid1[SidIndex].PID == 0x13 )
							{
								fPid13Supported = TRUE;
							}
							else
							{
								fPid1DSupported = TRUE;
							}

							/* Evaluate for dual PID / Spark engine support */
							if ( ( fPid13Supported == TRUE ) &&
							     ( fPid1DSupported == TRUE ) )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  PID 13 & 1D both indicated as supported\n", GetEcuId(EcuIndex) );
							}

							/* Count the number of O2 sensors */
							for (O2Bit = 0x01, O2Count = 0; O2Bit != 0x100; O2Bit = O2Bit << 1)
							{
								if (pSid1[SidIndex].Data[0] & O2Bit)
								{
									O2Count++;
								}
							}
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  %d O2 Sensors\n", GetEcuId(EcuIndex), O2Count);

							/* At least 2 O2 sensors required for spark ignition enges */
							if ( gOBDDieselFlag == FALSE &&
							     O2Count < 2 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  O2S < 2\n", GetEcuId(EcuIndex) );
							}
						}
						break;

						case 0x1E:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  PTO_STAT = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0]);
						}
						break;

						case 0x1F:
						{
							temp_data_long = ((pSid1[SidIndex].Data[0] * 256) +
							                  pSid1[SidIndex].Data[1]);
							GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  RUNTM = %d sec (%d hrs, %d mins, %d secs)\n",
							     GetEcuId(EcuIndex),
							     temp_data_long,
							     hours, mins, secs );

							if ( OBDEngineDontCare == FALSE )  // NOT Test 10.2
							{
								if ( gOBDEngineRunning == TRUE )
								{
									if (gOBDEngineWarm == TRUE)  // test 10.13
									{
										if (temp_data_long <= 300)
										{
											/* Run time should greater than 300 seconds */
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  RUNTM < 300 sec\n", GetEcuId(EcuIndex) , temp_data_long);
										}
									}
									else  // test 5.10
									{
										if ( (temp_data_long == 0) && (gOBDHybridFlag != TRUE) )
										{
											/* Run time should not be zero if engine is running */
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  RUNTM = 0 sec\n", GetEcuId(EcuIndex) );
										}
									}
								}
								else  // test 5.6
								{
									if (temp_data_long != 0)
									{
										/* Run time should be zero if engine is OFF */
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  RUNTM > 0 sec\n", GetEcuId(EcuIndex) );
									}
								}
							}
						}
						break;
						case 0x21:
						{
							temp_data_long = ((pSid1[SidIndex].Data[0] * 256) +
							                  pSid1[SidIndex].Data[1]);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  MIL_DIST = %d km\n", GetEcuId(EcuIndex), temp_data_long);

							if ( OBDEngineDontCare == FALSE )  // NOT Test 10.2
							{
								if ( ( (gOBDEngineWarm == TRUE)     ||            // test 10.12
								       (gOBDEngineRunning == FALSE) ||            // test 5.6
								       ( (gOBDEngineRunning == TRUE)  &&          // test 5.10
								         (gOBDEngineWarm == FALSE)    &&
								         (gOBDResponse[EcuIndex].Sid4Size > 0) &&
								         (gOBDResponse[EcuIndex].Sid4[0] == 0x44) ) ) &&
								     (temp_data_long != 0x00) )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  MIL_DIST > 0 after clearing DTCs\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;
						case 0x22:
						{
							temp_data_float = (float)((pSid1[SidIndex].Data[0] * 256) +
							                          pSid1[SidIndex].Data[1]) * (float)0.079;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  FRP (relative to manifold) = %.1f kPa\n", GetEcuId(EcuIndex)   , temp_data_float);

							if ( (OBDEngineDontCare == FALSE) &&  // NOT Test 10.2
							     (gOBDEngineRunning == TRUE)  &&  // test 5.10 & 10.13
							     (gOBDHybridFlag != TRUE)     &&  // not a hybrid vehicle
							     (temp_data_float == 0.0) )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FRP = 0 kPa\n", GetEcuId(EcuIndex) );
							}
						}
						break;
						case 0x23:
						{
							temp_data_long = ((pSid1[SidIndex].Data[0] * 256) +
							                  pSid1[SidIndex].Data[1]) * 10;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  FRP = %ld kPa\n", GetEcuId(EcuIndex), temp_data_long);

							if ( (OBDEngineDontCare == FALSE) &&  // NOT Test 10.2
							     (gOBDEngineRunning == TRUE)  &&  // test 5.10 & 10.12
							     (gOBDHybridFlag != TRUE)     &&  // not a hybrid vehicle
							     (temp_data_long == 0)  )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FRP = 0 kPa\n", GetEcuId(EcuIndex) );
							}
						}
						break;
						case 0x24:
						case 0x25:
						case 0x26:
						case 0x27:
						case 0x28:
						case 0x29:
						case 0x2A:
						case 0x2B:
						{
							switch (pSid1[SidIndex].PID)
							{
								case 0x24:
									BSIndex = 0x11;
								break;
								case 0x25:
									BSIndex = 0x12;
								break;
								case 0x26:
									BSIndex = fPid13Supported ? 0x13 : 0x21;
								break;
								case 0x27:
									BSIndex = fPid13Supported ? 0x14 : 0x22;
								break;
								case 0x28:
									BSIndex = fPid13Supported ? 0x21 : 0x31;
								break;
								case 0x29:
									BSIndex = fPid13Supported ? 0x22 : 0x32;
								break;
								case 0x2A:
									BSIndex = fPid13Supported ? 0x23 : 0x41;
								break;
								case 0x2B:
									BSIndex = fPid13Supported ? 0x24 : 0x42;
								break;
							}

							temp_data_float = (float)(((unsigned long)(pSid1[SidIndex].Data[0] << 8) |
							                           pSid1[SidIndex].Data[1]));
							if ( fPid4FSupported == FALSE || Pid4F[EcuIndex][0] == 0 )
							{
								temp_data_float = temp_data_float * (float)0.0000305;
							}
							else
							{
								temp_data_float = temp_data_float * ((float)(Pid4F[EcuIndex][0])/65535);
							}
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EQ_RAT%x = %.3f %%\n", GetEcuId(EcuIndex), BSIndex, temp_data_float);

							temp_data_float = (float)(((unsigned long)(pSid1[SidIndex].Data[2] << 8) |
							                           pSid1[SidIndex].Data[3]));
							if ( fPid4FSupported == FALSE || Pid4F[EcuIndex][1] == 0 )
							{
								temp_data_float = temp_data_float * (float)0.000122;
							}
							else
							{
								temp_data_float = temp_data_float * ((float)(Pid4F[EcuIndex][1])/65535);
							}
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  O2S%x = %.3f V\n", GetEcuId(EcuIndex), BSIndex, temp_data_float);
						}
						break;
						case 0x2C:
						{
							temp_data_float = (float)pSid1[SidIndex].Data[0] * (float)(100.0 / 255.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EGR_PCT = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

							if ( gOBDEngineRunning == FALSE )  // test 5.6
							{
								if ( temp_data_float > 10 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  EGR_PCT > 10 %%\n", GetEcuId(EcuIndex) );
								}
							}
							else  // test 5.10 & 10.13
							{
								if ( gOBDDieselFlag == TRUE )  // compression ignition
								{
									if (temp_data_float > 100)
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  EGR_PCT > 100 %%\n", GetEcuId(EcuIndex) );
									}
								}
								else  // spark ignition engine
								{
									if ( gOBDHybridFlag == FALSE && temp_data_float > 10 )
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  EGR_PCT > 10 %%\n", GetEcuId(EcuIndex) );
									}
									else if ( gOBDHybridFlag == TRUE && temp_data_float > 50 )
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  EGR_PCT > 50 %% \n", GetEcuId(EcuIndex) );
									}
								}
							}
						}
						break;

						case 0x2D:
						{
							temp_data_float = ( (float)pSid1[SidIndex].Data[0] * (float)(100.0/128.0) ) - (float)100.0;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EGR_ERR = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
						}
						break;

						case 0x2E:
						{
							temp_data_float = (float)pSid1[SidIndex].Data[0] * (float)(100.0/255.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EVAP_PCT = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
						}
						break;

						case 0x2F:
						{
							temp_data_float = ((float)pSid1[SidIndex].Data[0]) * (float)(100.0 / 255.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  FLI = %.1f\n", GetEcuId(EcuIndex), temp_data_float);

							if (temp_data_float < 1.0 || temp_data_float > 100.0)
							{
								Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FLI should be between 1 and 100 %%\n", GetEcuId(EcuIndex), temp_data_float);
							}
						}
						break;

						case 0x30:
						{
							temp_data_long = pSid1[SidIndex].Data[0];
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  WARM_UPS = %d\n", GetEcuId(EcuIndex), temp_data_long);

							if ( OBDEngineDontCare == FALSE )  // NOT Test 10.2
							{
								if (gOBDEngineWarm == TRUE)  // test 10.13
								{
									if (temp_data_long > 4)
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  WARM_UPS > 4\n", GetEcuId(EcuIndex) );
									}
								}
								else if ( gOBDEngineRunning == FALSE )          // test 5.6
								{
									if ( temp_data_long > 0 )
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  WARM_UPS > 0\n", GetEcuId(EcuIndex) );
									}
								}
								else if ( (gOBDEngineRunning == TRUE)  &&       // test 5.10
								          (gOBDResponse[EcuIndex].Sid4Size > 0) &&
								          (gOBDResponse[EcuIndex].Sid4[0] == 0x44) )
								{
									if ( temp_data_long > 1 )
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  WARM_UPS > 1\n", GetEcuId(EcuIndex) );
									}
								}
							}
						}
						break;
						case 0x31:
						{
							temp_data_long = ((pSid1[SidIndex].Data[0] * 256) +
							                  pSid1[SidIndex].Data[1]);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  CLR_DIST = %d km\n", GetEcuId(EcuIndex), temp_data_long);

							if ( OBDEngineDontCare == FALSE )  // NOT Test 10.2
							{
								if (gOBDEngineWarm == TRUE)                     // test 10.13
								{
									if (temp_data_long >= 50)
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  CLR_DIST >= 50km after CARB drive cycle\n", GetEcuId(EcuIndex) );
									}
								}
								else if ( (gOBDEngineRunning == FALSE) ||       // test 5.6
								          ((gOBDEngineRunning == TRUE) &&       // test 5.10
								           (gOBDResponse[EcuIndex].Sid4Size > 0) &&
								           (gOBDResponse[EcuIndex].Sid4[0] == 0x44)))
								{
									if (temp_data_long != 0)
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  CLR_DIST > 0 km after clearing DTCs\n", GetEcuId(EcuIndex) );
									}
								}
							}
						}
						break;

						case 0x32:
						{
							temp_data_float = (float)( (signed short)( (pSid1[SidIndex].Data[0] * 256) +
							                                           pSid1[SidIndex].Data[1] ) ) * (float).25;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EVAP_VP = %.1f Pa\n", GetEcuId(EcuIndex), temp_data_float);
						}
						break;

						case 0x33:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  BARO = %d kPa\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0]);

							if ((pSid1[SidIndex].Data[0] < 71) || (pSid1[SidIndex].Data[0] > 110))
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  BARO exceeded normal range\n", GetEcuId(EcuIndex) );
							}
						}
						break;

						case 0x34:
						case 0x35:
						case 0x36:
						case 0x37:
						case 0x38:
						case 0x39:
						case 0x3A:
						case 0x3B:
						{
							switch (pSid1[SidIndex].PID)
							{
								case 0x34:
									BSIndex = 0x11;
								break;
								case 0x35:
									BSIndex = 0x12;
								break;
								case 0x36:
									BSIndex = fPid13Supported ? 0x13 : 0x21;
								break;
								case 0x37:
									BSIndex = fPid13Supported ? 0x14 : 0x22;
								break;
								case 0x38:
									BSIndex = fPid13Supported ? 0x21 : 0x31;
								break;
								case 0x39:
									BSIndex = fPid13Supported ? 0x22 : 0x32;
								break;
								case 0x3A:
									BSIndex = fPid13Supported ? 0x23 : 0x41;
								break;
								case 0x3B:
									BSIndex = fPid13Supported ? 0x24 : 0x42;
								break;
							}

							// scale Equivalence Ratio
							temp_data_float = (float)(((unsigned long)(pSid1[SidIndex].Data[0] << 8) |
							                           pSid1[SidIndex].Data[1]));
							if ( fPid4FSupported == FALSE || Pid4F[EcuIndex][0] == 0 )
							{
								temp_data_float = temp_data_float * (float)0.0000305;
							}
							else
							{
								temp_data_float = temp_data_float * ((float)(Pid4F[EcuIndex][0])/65535);
							}
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EQ_RAT%x = %.3f %%\n", GetEcuId(EcuIndex), BSIndex, temp_data_float);

							// offset and scale Oxygen Sensor Current
							temp_data_float = (float)((((signed short)(pSid1[SidIndex].Data[2]) << 8) |
							                            pSid1[SidIndex].Data[3]) - 0x8000);
							if ( fPid4FSupported == FALSE || Pid4F[EcuIndex][2] == 0 )
							{
								temp_data_float = temp_data_float * (float)0.00390625;
							}
							else
							{
								temp_data_float = temp_data_float * ((float)(Pid4F[EcuIndex][2])/32768);
							}
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  O2S%x = %.2f mA\n", GetEcuId(EcuIndex), BSIndex, temp_data_float);
						}
						break;

						case 0x3C:
						case 0x3D:
						case 0x3E:
						case 0x3F:
						{
							switch (pSid1[SidIndex].PID)
							{
								case 0x3C:
									BSIndex = 0x11;
								break;
								case 0x3D:
									BSIndex = 0x21;
								break;
								case 0x3E:
									BSIndex = 0x12;
								break;
								case 0x3F:
									BSIndex = 0x22;
								break;
							}
							temp_data_float = ( (float)( (pSid1[SidIndex].Data[0] * 256) +
							                             pSid1[SidIndex].Data[1] ) * (float).1 ) - (float)(40.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  CATEMP%X = %.1f C\n", GetEcuId(EcuIndex), BSIndex, temp_data_float);
						}
						break;

						case 0x41:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Monitor Status  Byte A = $%02X  B = $%02X  C = $%02X  D = $%02X\n", GetEcuId(EcuIndex),
							     pSid1[SidIndex].Data[0],
							     pSid1[SidIndex].Data[1],
							     pSid1[SidIndex].Data[2],
							     pSid1[SidIndex].Data[3] );

							/* Diesel Ignition bit */
							/* only check if MY 2010 and beyond and ECU does not only supports CCM requirements (SID 1 PID 1 Data B bit 2==1) */
							if ( gModelYear >= 2010 &&
							     ( ((Sid1Pid1[EcuIndex].Data[1]) & 0x03) != 0x00 ||
							       ((Sid1Pid1[EcuIndex].Data[2]) & 0xFF) != 0x00 ) )
							{
								if (
								     (pSid1[SidIndex].Data[1] & 0x08) == 0 &&
								     (gOBDDieselFlag == 1)
								   )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  SID $1 PID $41 Data B bit 3 clear (Spark Ignition) does not match user input (Diesel)\n", GetEcuId(EcuIndex) );
								}
								else if (
								          (pSid1[SidIndex].Data[1] & 0x08) == 0x08 &&
								          (gOBDDieselFlag == 0)
								        )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  SID $1 PID $41 Data B bit 3 set (Compression Ignition) does not match user input (Non Diesel)\n", GetEcuId(EcuIndex) );
								}
							}

							if ( OBDEngineDontCare == FALSE && gOBDEngineRunning == FALSE )  // test 5.6; NOT Test 5.10, 10.2 & 10.13
							{
								/*
								 * supported monitors should not be complete (except O2 on spark ignition)
								 */
								/* DATA_D Bit 0 */
								if (
								     ( (Sid1Pid1[EcuIndex].Data[2] & 0x01) == 0x01 ) &&
								     ( (pSid1[SidIndex].Data[3]    & 0x01) == 0x00 )
								   )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  %s: Supported monitor must indicate 'incomplete'\n",
									     GetEcuId(EcuIndex), gOBDDieselFlag ? "HCCAT" : "CAT" );
								}

								/* DATA_D Bit 1 */
								if (
								     ( (Sid1Pid1[EcuIndex].Data[2] & 0x02) == 0x02 ) &&
								     ( (pSid1[SidIndex].Data[3]    & 0x02) == 0x00 )
								   )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  %s: Supported monitor must indicate 'incomplete'\n",
									     GetEcuId(EcuIndex), gOBDDieselFlag ? "NCAT" : "HCAT" );
								}

								/* DATA_D Bit 2 */
								if (
								     ( (Sid1Pid1[EcuIndex].Data[2] & 0x04) == 0x04 ) &&
								     ( (pSid1[SidIndex].Data[3]    & 0x04) == 0x00 )
								   )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  %s: Supported monitor must indicate 'incomplete'\n",
									     GetEcuId(EcuIndex), gOBDDieselFlag ? "RESERVED2" : "EVAP" );
								}

								/* DATA_D Bit 3 */
								if (
								     ( (Sid1Pid1[EcuIndex].Data[2] & 0x08) == 0x08 ) &&
								     ( (pSid1[SidIndex].Data[3]    & 0x08) == 0x00 )
								   )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  %s: Supported monitor must indicate 'incomplete'\n",
									     GetEcuId(EcuIndex), gOBDDieselFlag ? "BP" : "AIR" );
								}

								/* DATA_D Bit 4 */
								if (
								     ( (Sid1Pid1[EcuIndex].Data[2] & 0x10) == 0x10 ) &&
								     ( (pSid1[SidIndex].Data[3]    & 0x10) == 0x00 )
								   )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  RESERVED4: Supported monitor must indicate 'incomplete'\n",
									     GetEcuId(EcuIndex) );
								}

								/* DATA_D Bit 5 */
								if (
								     ( (Sid1Pid1[EcuIndex].Data[2] & 0x20) == 0x20 ) &&
								     ( (pSid1[SidIndex].Data[3]    & 0x20) == 0x00 )
								   )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  %s: Supported monitor must indicate 'incomplete'\n",
									     GetEcuId(EcuIndex), gOBDDieselFlag ? "EGS" : "O2S" );
								}

								/* DATA_D Bit 6 */
								if (
								     ( (Sid1Pid1[EcuIndex].Data[2] & 0x40 ) == 0x40) &&
								     ( (pSid1[SidIndex].Data[3]    & 0x40 ) == 0x00) && gOBDDieselFlag
								   )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  %s: Supported monitor must indicate 'incomplete'\n",
									     GetEcuId(EcuIndex), gOBDDieselFlag ? "PM" : "HTR" );
								}

								/* DATA_D Bit 7 */
								if (
								     ( (Sid1Pid1[EcuIndex].Data[2] & 0x80) == 0x80 ) &&
								     ( (pSid1[SidIndex].Data[3]    & 0x80) == 0x00 )
								   )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  EGR: Supported monitor must indicate 'incomplete'\n",
									     GetEcuId(EcuIndex) );
								}
							}


							/*
							 * check that set bits match support in PID $01
							 */
							/* DATA_B Bit 0-2 */
							if ( ( (pSid1[SidIndex].Data[1] & 0x01) == 0x01 && (Sid1Pid1[EcuIndex].Data[1] & 0x01) == 0x00 ) ||
							     ( (pSid1[SidIndex].Data[1] & 0x02) == 0x02 && (Sid1Pid1[EcuIndex].Data[1] & 0x02) == 0x00 ) ||
							     ( (pSid1[SidIndex].Data[1] & 0x04) == 0x04 && (Sid1Pid1[EcuIndex].Data[1] & 0x04) == 0x00 ) )
							{
								if ( gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "If monitor enabled in Byte B, it must show supported in PID $01\n" );
								}
								else // EOBD
								{
									Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "If monitor enabled in Byte B, it must show supported in PID $01\n" );
								}
							}

							/* DATA_B Bit 4-6 */
							if ( ( (pSid1[SidIndex].Data[1] & 0x10) == 0x10 && (Sid1Pid1[EcuIndex].Data[1] & 0x01) == 0x00 ) ||
							     ( (pSid1[SidIndex].Data[1] & 0x20) == 0x20 && (Sid1Pid1[EcuIndex].Data[1] & 0x02) == 0x00 ) ||
							     ( (pSid1[SidIndex].Data[1] & 0x40) == 0x40 && (Sid1Pid1[EcuIndex].Data[1] & 0x04) == 0x00 ) )
							{
								if ( gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "If monitor not complete in Byte B, it must show supported in PID $01\n" );
								}
								else // EOBD
								{
									Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "If monitor not complete in Byte B, it must show supported in PID $01\n" );
								}
							}

							/* DATA_C Bit 0-7 */
							if ( ( (pSid1[SidIndex].Data[2] & 0x01) == 0x01 && (Sid1Pid1[EcuIndex].Data[2] & 0x01) == 0x00 ) ||
							     ( (pSid1[SidIndex].Data[2] & 0x02) == 0x02 && (Sid1Pid1[EcuIndex].Data[2] & 0x02) == 0x00 ) ||
							     ( (pSid1[SidIndex].Data[2] & 0x04) == 0x04 && (Sid1Pid1[EcuIndex].Data[2] & 0x04) == 0x00 ) ||
							     ( (pSid1[SidIndex].Data[2] & 0x08) == 0x08 && (Sid1Pid1[EcuIndex].Data[2] & 0x08) == 0x00 ) ||
							     ( (pSid1[SidIndex].Data[2] & 0x10) == 0x10 && (Sid1Pid1[EcuIndex].Data[2] & 0x10) == 0x00 ) ||
							     ( (pSid1[SidIndex].Data[2] & 0x20) == 0x20 && (Sid1Pid1[EcuIndex].Data[2] & 0x20) == 0x00 ) ||
							     ( (pSid1[SidIndex].Data[2] & 0x40) == 0x40 && (Sid1Pid1[EcuIndex].Data[2] & 0x40) == 0x00 ) ||
							     ( (pSid1[SidIndex].Data[2] & 0x80) == 0x80 && (Sid1Pid1[EcuIndex].Data[2] & 0x80) == 0x00 ) )
							{
								if ( gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "If monitor not complete in Byte C, it must show supported in PID $01\n" );
								}
								else // EOBD
								{
									Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "If monitor not complete in Byte C, it must show supported in PID $01\n" );
								}
							}

							/* DATA_D Bit 0-7 */
							if ( ( (pSid1[SidIndex].Data[3] & 0x01) == 0x01 && (Sid1Pid1[EcuIndex].Data[2] & 0x01) == 0x00 ) ||
							     ( (pSid1[SidIndex].Data[3] & 0x02) == 0x02 && (Sid1Pid1[EcuIndex].Data[2] & 0x02) == 0x00 ) ||
							     ( (pSid1[SidIndex].Data[3] & 0x04) == 0x04 && (Sid1Pid1[EcuIndex].Data[2] & 0x04) == 0x00 ) ||
							     ( (pSid1[SidIndex].Data[3] & 0x08) == 0x08 && (Sid1Pid1[EcuIndex].Data[2] & 0x08) == 0x00 ) ||
							     ( (pSid1[SidIndex].Data[3] & 0x10) == 0x10 && (Sid1Pid1[EcuIndex].Data[2] & 0x10) == 0x00 ) ||
							     ( (pSid1[SidIndex].Data[3] & 0x20) == 0x20 && (Sid1Pid1[EcuIndex].Data[2] & 0x20) == 0x00 ) ||
							     ( (pSid1[SidIndex].Data[3] & 0x40) == 0x40 && (Sid1Pid1[EcuIndex].Data[2] & 0x40) == 0x00 ) ||
							     ( (pSid1[SidIndex].Data[3] & 0x80) == 0x80 && (Sid1Pid1[EcuIndex].Data[2] & 0x80) == 0x00 ) )
							{
								if ( gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "If monitor enabled in Byte D, it must show supported in PID $01\n" );
								}
								else // EOBD
								{
									Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "If monitor enabled in Byte D, it must show supported in PID $01\n" );
								}
							}

							/*
							 * unsupported monitors should be complete
							 */
							/* DATA_D Bit 0 */
							if (
							     ( (Sid1Pid1[EcuIndex].Data[2] & 0x01) == 0x00 ) &&
							     ( (pSid1[SidIndex].Data[3]    & 0x01) == 0x01 )
							   )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  %s: Unsupported monitor must indicate 'complete'\n",
								     GetEcuId(EcuIndex), gOBDDieselFlag ? "HCCAT" : "CAT" );
							}

							/* DATA_D Bit 1 */
							if (
							     ( (Sid1Pid1[EcuIndex].Data[2] & 0x02) == 0x00 ) &&
							     ( (pSid1[SidIndex].Data[3]    & 0x02) == 0x02 )
							   )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  %s: Unsupported monitor must indicate 'complete'\n",
								     GetEcuId(EcuIndex), gOBDDieselFlag ? "NCAT" : "HCAT" );
							}

							/* DATA_D Bit 2 */
							if (
							     ( (Sid1Pid1[EcuIndex].Data[2] & 0x04) == 0x00 ) &&
							     ( (pSid1[SidIndex].Data[3]    & 0x04) == 0x04 )
							   )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  %s: Unsupported monitor must indicate 'complete'\n",
								     GetEcuId(EcuIndex), gOBDDieselFlag ? "RESERVED2" : "EVAP" );
							}

							/* DATA_D Bit 3 */
							if (
							     ( (Sid1Pid1[EcuIndex].Data[2] & 0x08) == 0x00 ) &&
							     ( (pSid1[SidIndex].Data[3]    & 0x08) == 0x08 )
							   )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  %s: Unsupported monitor must indicate 'complete'\n",
								     GetEcuId(EcuIndex), gOBDDieselFlag ? "BP" : "AIR" );
							}

							/* DATA_D Bit 4 */
							if (
							     ( (Sid1Pid1[EcuIndex].Data[2] & 0x10) == 0x00 ) &&
							     ( (pSid1[SidIndex].Data[3]    & 0x10) == 0x10 )
							   )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  RESERVED4: Unsupported monitor must indicate 'complete'\n",
								     GetEcuId(EcuIndex) );
							}

							/* DATA_D Bit 5 */
							if (
							     ( (Sid1Pid1[EcuIndex].Data[2] & 0x20) == 0x00 ) &&
							     ( (pSid1[SidIndex].Data[3]    & 0x20) == 0x20 )
							   )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  %s: Unsupported monitor must indicate 'complete'\n",
								     GetEcuId(EcuIndex), gOBDDieselFlag ? "EGS" : "O2S" );
							}

							/* DATA_D Bit 6 */
							if (
							     ( (Sid1Pid1[EcuIndex].Data[2] & 0x40) == 0x00 ) &&
							     ( (pSid1[SidIndex].Data[3]    & 0x40) == 0x40 )
							   )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  %s: Unsupported monitor must indicate 'complete'\n",
								     GetEcuId(EcuIndex), gOBDDieselFlag ? "PM" : "HTR" );
							}

							/* DATA_D Bit 7 */
							if (
							     ( (Sid1Pid1[EcuIndex].Data[2] & 0x80) == 0x00 ) &&
							     ( (pSid1[SidIndex].Data[3]    & 0x80) == 0x80 )
							   )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGR: Unsupported monitor must indicate 'complete'\n",
								     GetEcuId(EcuIndex) );
							}
						}
						break;

						case 0x42:
						{
							temp_data_float = (float)( (pSid1[SidIndex].Data[0] * 256) +
							                           pSid1[SidIndex].Data[1] ) * (float).001;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  VPWR = %.3f V\n", GetEcuId(EcuIndex), temp_data_float);
						}
						break;

						case 0x43:
						{
							temp_data_float = (float)( (pSid1[SidIndex].Data[0] * 256) +
							                           pSid1[SidIndex].Data[1] ) * (float)(100.0 / 255.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  LOAD_ABS = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

							if ( OBDEngineDontCare == FALSE )  // NOT Test 10.2
							{
								if (gOBDEngineRunning == TRUE)  // test 5.10 & 10.13
								{
									if ( (temp_data_float == 0.0) && (gOBDHybridFlag != TRUE) )
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  LOAD_ABS = 0%\n", GetEcuId(EcuIndex) );
									}
								}
								else  // test 5.6
								{
									if (temp_data_float != 0.0)
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  LOAD_ABS > 0%\n", GetEcuId(EcuIndex) );
									}
								}
							}
						}
						break;
						case 0x44:
						{
							temp_data_float = (float)(((unsigned long)(pSid1[SidIndex].Data[0]) << 8) |
							                           pSid1[SidIndex].Data[1]);
							if ( fPid4FSupported == FALSE || Pid4F[EcuIndex][0] == 0 )
							{
								temp_data_float = temp_data_float * (float)0.0000305;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EQ_RAT = %.3f %%\n", GetEcuId(EcuIndex), temp_data_float);

								if ( OBDEngineDontCare == FALSE && gOBDEngineWarm == TRUE )  // test 10.13
								{
									if ( (gOBDDieselFlag == FALSE ) && (gOBDHybridFlag != TRUE) )
									{
										if ( temp_data_float < 0.5 || temp_data_float > 1.5  )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  EQ_RAT exceeded normal range\n", GetEcuId(EcuIndex) );
										}
									}
								}
							}
							else
							{
								temp_data_float = temp_data_float * ((float)(Pid4F[EcuIndex][0])/65535);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EQ_RAT = %.3f %%\n", GetEcuId(EcuIndex), temp_data_float);
							}
						}
						break;
						case 0x45:
						{
							temp_data_float = (float)pSid1[SidIndex].Data[0] * (float)(100.0 / 255.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  TP_R = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

							if ( (gOBDDieselFlag == FALSE) && (gOBDHybridFlag == FALSE) && (temp_data_float > 50) )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TP_R exceeded normal range\n", GetEcuId(EcuIndex) );
							}
						}
						break;
						case 0x46:
						{
							temp_data_long = pSid1[SidIndex].Data[0] - 40;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  AAT = %d C\n", GetEcuId(EcuIndex), temp_data_long);

							if ((temp_data_long < -20) || (temp_data_long > 85))
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  AAT exceeded normal range\n", GetEcuId(EcuIndex) );
							}
						}
						break;
						case 0x47:
						{
							temp_data_float = (float)pSid1[SidIndex].Data[0] * (float)(100.0 / 255.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  TP_B = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

							if ( (gOBDDieselFlag == FALSE) && (gOBDHybridFlag == FALSE) && (temp_data_float > 60) )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TP_B exceeded normal range\n", GetEcuId(EcuIndex) );
							}
						}
						break;
						case 0x48:
						{
							temp_data_float = (float)pSid1[SidIndex].Data[0] * (float)(100.0 / 255.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  TP_C = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

							if ( (gOBDDieselFlag == FALSE) && (gOBDHybridFlag == FALSE) && (temp_data_float > 60) )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TP_C exceeded normal range\n", GetEcuId(EcuIndex) );
							}
						}
						break;
						case 0x49:
						{
							temp_data_float = (float)pSid1[SidIndex].Data[0] * (float)(100.0 / 255.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  APP_D = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

							if (temp_data_float > 40)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  APP_D exceeded normal range\n", GetEcuId(EcuIndex) );
							}
						}
						break;
						case 0x4A:
						{
							temp_data_float = (float)pSid1[SidIndex].Data[0] * (float)(100.0 / 255.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  APP_E = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

							if (temp_data_float > 40)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  APP_E exceeded normal range\n", GetEcuId(EcuIndex) );
							}
						}
						break;
						case 0x4B:
						{
							temp_data_float = (float)pSid1[SidIndex].Data[0] * (float)(100.0 / 255.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  APP_F = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

							if (temp_data_float > 40)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  APP_F exceeded normal range\n", GetEcuId(EcuIndex) );
							}
						}
						break;

						case 0x4C:
						{
							temp_data_float = (float)pSid1[SidIndex].Data[0] * (float)(100.0 / 255.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  TAC_PCT = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
						}
						break;

						case 0x4D:
						{
							temp_data_long = ( pSid1[SidIndex].Data[0] * 256 ) +
							                 pSid1[SidIndex].Data[1];
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  MIL_TIME = %d min\n", GetEcuId(EcuIndex), temp_data_long);

							if (temp_data_long != 0x00)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  MIL_TIME > 0\n", GetEcuId(EcuIndex) );
							}
						}
						break;
						case 0x4E:
						{
							temp_data_long = ( pSid1[SidIndex].Data[0] * 256 ) +
							                 pSid1[SidIndex].Data[1];
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  CLR_TIME = %d min\n", GetEcuId(EcuIndex), temp_data_long);

							if ( OBDEngineDontCare == FALSE )  // NOT Test 10.2
							{
								if ( gOBDEngineWarm == TRUE )                   // test 10.13
								{
									if (temp_data_long < 5 )
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  CLR_TIME < 5 min\n", GetEcuId(EcuIndex) );
									}
								}
								else if ( (gOBDEngineRunning == FALSE) ||       // test 5.6
								          ( (gOBDEngineRunning == TRUE)  &&     // test 5.10
								            (gOBDResponse[EcuIndex].Sid4Size > 0) &&
								            (gOBDResponse[EcuIndex].Sid4[0] == 0x44) ) )
								{
									if (temp_data_long != 0)
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  CLR_TIME > 0 min\n", GetEcuId(EcuIndex) );
									}
								}
							}
						}
						break;

						case 0x4F:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EQ_RAT scaling = %d / 65535\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  O2S Voltage scaling = %d * (1 V / 65535)\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[1] );

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  O2S Current scaling = %d * (1 mA / 32768)\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[2] );

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  MAP scaling = %d * (10 kPa / 255)\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[3] );
						}
						break;

						case 0x50:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  MAF scaling = $%02X * (10 g/s / 65535)\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
						}
						break;

						case 0x51:
						{
							temp_data_long = pSid1[SidIndex].Data[0];
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  FUEL_TYPE = $%02X\n", GetEcuId(EcuIndex), temp_data_long);

							if ( temp_data_long == 0x00 ||
							     (temp_data_long >= 0x0F && temp_data_long <= 0x17) )
							{
								Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FUEL_TYPE must be $01 to $0E or $18 to $1C\n", GetEcuId(EcuIndex) );
							}
							else if ( temp_data_long >= 0x1D )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FUEL_TYPE must be $01 to $0E or $18 to $1C\n", GetEcuId(EcuIndex) );
							}
						}
						break;

						case 0x52:
						{
							temp_data_float = (float)pSid1[SidIndex].Data[0] * (float)(100.0 / 255.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  ALCH_PCT = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
						}
						break;

						case 0x53:
						{
							temp_data_float = ( (pSid1[SidIndex].Data[0] * 256) +
							                    pSid1[SidIndex].Data[1] ) * (float)0.005;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EVAP_VPA = %.3f kPa\n", GetEcuId(EcuIndex), temp_data_float);
						}
						break;

						case 0x54:
						{
							temp_data_long = (long)((signed short)((pSid1[SidIndex].Data[0] * 256) +
							                                       pSid1[SidIndex].Data[1]));
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EVAP_VP = %d Pa\n", GetEcuId(EcuIndex), temp_data_long);
						}
						break;

						case 0x55:
						{
							temp_data_float = (float)( pSid1[SidIndex].Data[0]  - 128 ) * (float)(100.0 / 128.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  STSO2FT1 = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
							if (temp_data_float < -100.0 || temp_data_float > 99.2)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  STSO2FT1 out of range\n", GetEcuId(EcuIndex) );
							}

							if (gSid1VariablePidSize == 2)
							{
								temp_data_float =  (float)( pSid1[SidIndex].Data[1] - 128 ) * (float)(100.0 / 128.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  STSO2FT3 = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
								if (temp_data_float < -100.0 || temp_data_float > 99.2)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  STSO2FT3 out of range\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;

						case 0x56:
						{
							temp_data_float = (float)( pSid1[SidIndex].Data[0] - 128 ) * (float)(100.0 / 128.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  LGSO2FT1 = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
							if (temp_data_float < -100.0 || temp_data_float > 99.2)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  LGSO2FT1 out of range\n", GetEcuId(EcuIndex) );
							}

							if (gSid1VariablePidSize == 2)
							{
								temp_data_float = (float)( pSid1[SidIndex].Data[1] - 128 ) * (float)(100.0 / 128.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  LGSO2FT3 = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
								if (temp_data_float < -100.0 || temp_data_float > 99.2)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  LGSO2FT3 out of range\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;

						case 0x57:
						{
							temp_data_float = (float)( pSid1[SidIndex].Data[0] - 128 ) * (float)(100.0 / 128.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  STSO2FT2 = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
							if (temp_data_float < -100.0 || temp_data_float > 99.2)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  STSO2FT2 out of range\n", GetEcuId(EcuIndex) );
							}

							if (gSid1VariablePidSize == 2)
							{
								temp_data_float = (float)( pSid1[SidIndex].Data[1] - 128 ) * (float)(100.0 / 128.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  STSO2FT4 = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
								if (temp_data_float < -100.0 || temp_data_float > 99.2)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  STSO2FT4 out of range\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;

						case 0x58:
						{
							temp_data_float = (float)( pSid1[SidIndex].Data[0] - 128 ) * (float)(100.0 / 128.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  LGSO2FT2 = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
							if (temp_data_float < -100.0 || temp_data_float > 99.2)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  LGSO2FT2 out of range\n", GetEcuId(EcuIndex) );
							}

							if (gSid1VariablePidSize == 2)
							{
								temp_data_float = (float)( pSid1[SidIndex].Data[1] - 128 ) * (float)(100.0 / 128.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  LGSO2FT4 = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
								if (temp_data_float < -100.0 || temp_data_float > 99.2)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  LGSO2FT4 out of range\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;

						case 0x59:
						{
							temp_data_long = ( (pSid1[SidIndex].Data[0] * 256) +
							                    pSid1[SidIndex].Data[1] ) * 10;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  FRP = %d kPa\n", GetEcuId(EcuIndex), temp_data_long);

							if ( OBDEngineDontCare == FALSE && gOBDEngineRunning == TRUE && gOBDHybridFlag != TRUE &&
							     temp_data_long == 0 )  // test 5.10 & 10.13
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FRP exceeded normal range\n", GetEcuId(EcuIndex) );
							}
						}
						break;

						case 0x5A:
						{
							temp_data_float = (float)pSid1[SidIndex].Data[0] * (float)(100.0 / 255.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  APP_R = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
							if ( temp_data_float > 40 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  APP_R exceeded normal range\n", GetEcuId(EcuIndex) );
							}
						}
						break;

						case 0x5B:
						{
							temp_data_float = (float)pSid1[SidIndex].Data[0] * (float)(100.0 / 255.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  BAT_PWR = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

							if ( OBDEngineDontCare == FALSE && gOBDEngineRunning == TRUE &&
							     temp_data_float == 0 )  // test 5.10 & 10.13
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  BAT_PWR exceeded normal range\n", GetEcuId(EcuIndex) );
							}
						}
						break;

						case 0x5C:
						{
							temp_data_long = pSid1[SidIndex].Data[0] - 40;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EOT = %d C\n", GetEcuId(EcuIndex), temp_data_long);
							if ( temp_data_long  < -20 || temp_data_long > 150 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EOT exceeded normal range\n", GetEcuId(EcuIndex) );
							}
						}
						break;

						case 0x5D:
						{
							temp_data_float = (float)( ( (pSid1[SidIndex].Data[0] * 256) +
							                             pSid1[SidIndex].Data[1] ) - 26880 ) / 128;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  FUEL_TIMING = %.2f\n", GetEcuId(EcuIndex), temp_data_float);
							if ( temp_data_float < -210 || temp_data_float >= 302 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FUEL_TIMING exceeded normal range\n", GetEcuId(EcuIndex) );
							}
						}
						break;

						case 0x5E:
						{
							temp_data_float = ( (pSid1[SidIndex].Data[0] * 256) +
							                    pSid1[SidIndex].Data[1] ) * (float)0.05;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  FUEL_RATE = %.2f L/h\n", GetEcuId(EcuIndex), temp_data_float);

							if ( OBDEngineDontCare == FALSE )  // NOT Test 10.2
							{
								if (gOBDEngineRunning == TRUE)  // test 5.10 & 10.13
								{
									if ( (temp_data_float == 0.0) && (gOBDHybridFlag != TRUE) )
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  FUEL_RATE exceeded normal range\n", GetEcuId(EcuIndex) );
									}
								}
								else // test 5.6
								{
									if ( temp_data_float != 0.0 )
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  FUEL_RATE exceeded normal range\n", GetEcuId(EcuIndex) );
									}
								}
							}
						}
						break;

						case 0x5F:
						{
							temp_data_long = pSid1[SidIndex].Data[0];
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EMIS_SUP = $%02X\n", GetEcuId(EcuIndex), temp_data_long);
							if ( temp_data_long  < 0x0E || temp_data_long > 0x10 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EMIS_SUP must be $0E to $10\n", GetEcuId(EcuIndex) );
							}
						}
						break;

						case 0x61:
						{
							temp_data_long = pSid1[SidIndex].Data[0] - 125;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  TQ_DD = %d %%\n", GetEcuId(EcuIndex), temp_data_long);
						}
						break;

						case 0x62:
						{
							temp_data_long = pSid1[SidIndex].Data[0] - 125;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  TQ_ACT = %d %%\n", GetEcuId(EcuIndex), temp_data_long);

							if ( OBDEngineDontCare == FALSE )  // NOT Test 10.2
							{
								if ( gOBDEngineRunning == TRUE )  // test 5.10 & 10.13
								{
									if ( temp_data_long <= 0 )
									{
										if ( !((temp_data_long == 0) && (gOBDHybridFlag == TRUE)) )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  TQ_ACT exceeded normal range\n", GetEcuId(EcuIndex) );
										}
									}
								}
								else if ( temp_data_long < 0 )  // test 5.6
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  TQ_ACT exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;

						case 0x63:
						{
							temp_data_long = (pSid1[SidIndex].Data[0] * 256) +
							                 pSid1[SidIndex].Data[1];
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  TQ_REF = %d Nm\n", GetEcuId(EcuIndex), temp_data_long);

							if ( OBDEngineDontCare == FALSE && gOBDEngineRunning == TRUE &&
							     temp_data_long <= 0 )  // test 5.10 & 10.13
							{
								if ( !((temp_data_long == 0) && (gOBDHybridFlag == TRUE)) )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  TQ_REF exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;

						case 0x64:
						{
							temp_data_long = pSid1[SidIndex].Data[0] - 125;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  TQ_MAX1 = %d %%\n", GetEcuId(EcuIndex), temp_data_long);

							if ( OBDEngineDontCare == FALSE && gOBDEngineRunning == TRUE )  // test 5.10 & 10.13
							{
								if ( temp_data_long <= 0 )
								{
									if ( !((temp_data_long == 0) && (gOBDHybridFlag == TRUE)) )
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  TQ_MAX1 exceeded normal range\n", GetEcuId(EcuIndex) );
									}
								}
							}

							temp_data_long = pSid1[SidIndex].Data[1] - 125;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  TQ_MAX2 = %d %%\n", GetEcuId(EcuIndex), temp_data_long);

							if ( OBDEngineDontCare == FALSE && gOBDEngineRunning == TRUE )  // test 5.10 & 10.13
							{
								if ( temp_data_long <= 0 )
								{
									if ( !((temp_data_long == 0) && (gOBDHybridFlag == TRUE)) )
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  TQ_MAX2 exceeded normal range\n", GetEcuId(EcuIndex) );
									}
								}
							}

							temp_data_long = pSid1[SidIndex].Data[2] - 125;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  TQ_MAX3 = %d %%\n", GetEcuId(EcuIndex), temp_data_long);

							if ( OBDEngineDontCare == FALSE && gOBDEngineRunning == TRUE )  // test 5.10 & 10.13
							{
								if ( temp_data_long <= 0 )
								{
									if ( !((temp_data_long == 0) && (gOBDHybridFlag == TRUE)) )
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  TQ_MAX3 exceeded normal range\n", GetEcuId(EcuIndex) );
									}
								}
							}

							temp_data_long = pSid1[SidIndex].Data[3] - 125;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  TQ_MAX4 = %d %%\n", GetEcuId(EcuIndex), temp_data_long);

							if ( OBDEngineDontCare == FALSE && gOBDEngineRunning == TRUE )  // test 5.10 & 10.13
							{
								if ( temp_data_long <= 0 )
								{
									if ( !((temp_data_long == 0) && (gOBDHybridFlag == TRUE)) )
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  TQ_MAX4 exceeded normal range\n", GetEcuId(EcuIndex) );
									}
								}
							}

							temp_data_long = pSid1[SidIndex].Data[4] - 125;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  TQ_MAX5 = %d %%\n", GetEcuId(EcuIndex), temp_data_long);

							if ( OBDEngineDontCare == FALSE && gOBDEngineRunning == TRUE )  // test 5.10 & 10.13
							{
								if ( temp_data_long <= 0 )
								{
									if ( !((temp_data_long == 0) && (gOBDHybridFlag == TRUE)) )
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  TQ_MAX5 exceeded normal range\n", GetEcuId(EcuIndex) );
									}
								}
							}
						}
						break;

						case 0x65:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Auxiliary I/O A = $%02X  Auxiliary I/O B = $%02X\n", GetEcuId(EcuIndex),
							     pSid1[SidIndex].Data[0],
							     pSid1[SidIndex].Data[1] );

							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x1F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Auxiliary I/O  No devices supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( (pSid1[SidIndex].Data[0] & 0xE0) != 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Auxiliary I/O  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( gOBDDieselFlag == TRUE && (pSid1[SidIndex].Data[0] & 0x08) == 0 )
							{
								Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Auxiliary I/O A Bit 3 must be set for diesel vehicles\n", GetEcuId(EcuIndex) );
							}

						}
						break;

						case 0x66:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  MAF Sensor support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x03) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  MAF Sensor support  No sensors supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xFC )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  MAF Sensor support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_float = (float)( (pSid1[SidIndex].Data[1] * 256) +
								                           pSid1[SidIndex].Data[2] ) * (float)0.03125;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  MAFA = %.3f g/s\n", GetEcuId(EcuIndex), temp_data_float);

								if ( OBDEngineDontCare == FALSE )  // NOT Test 10.2
								{
									if ( gOBDEngineRunning == TRUE )  // test 5.10 & 10.13
									{
										if ( temp_data_float <= 0 )
										{
											if ( !((temp_data_float == 0.0) && (gOBDHybridFlag == TRUE)) )
											{
												Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
												     "ECU %X  MAFA exceeded normal range\n", GetEcuId(EcuIndex) );
											}
										}
									}
									else  // test 5.6
									{
										if ( temp_data_float > 5 )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  MAFA exceeded normal range\n", GetEcuId(EcuIndex) );
										}
									}
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_float = (float)( (pSid1[SidIndex].Data[3] * 256) +
								                           pSid1[SidIndex].Data[4] ) * (float)0.03125;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  MAFB = %.3f g/s\n", GetEcuId(EcuIndex), temp_data_float);

								if ( OBDEngineDontCare == FALSE )  // NOT Test 10.2
								{
									if ( gOBDEngineRunning == TRUE )  // test 5.10 & 10.13
									{
										if ( temp_data_float <= 0 )
										{
											if ( !((temp_data_float == 0.0) && (gOBDHybridFlag == TRUE)) )
											{
												Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
												     "ECU %X  MAFB exceeded normal range\n", GetEcuId(EcuIndex) );
											}
										}
									}
									else  // test 5.6
									{
										if ( temp_data_float > 5 )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  MAFB exceeded normal range\n", GetEcuId(EcuIndex) );
										}
									}
								}
							}
						}
						break;

						case 0x67:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  ECT Sensor support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x03) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  ECT Sensor support  No sensors supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xFC )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  ECT Sensor support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_long = (pSid1[SidIndex].Data[1] ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  ECT1 = %d C\n", GetEcuId(EcuIndex), temp_data_long);
								if ( temp_data_long < -20 || temp_data_long > 120 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  ECT1 exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_long = (pSid1[SidIndex].Data[2] ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  ECT2 = %d C\n", GetEcuId(EcuIndex), temp_data_long);
								if ( temp_data_long < -20 || temp_data_long > 120 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  ECT2 exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;

						case 0x68:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  IAT Sensor support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x3F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  IAT Sensor support  No sensors supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xC0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  IAT Sensor support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[1] ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  IAT11 = %d C\n", GetEcuId(EcuIndex), temp_data_long);
								if ( temp_data_long < -20 || temp_data_long > 120 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  IAT11 exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[2] ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  IAT12 = %d C\n", GetEcuId(EcuIndex), temp_data_long);
								if ( temp_data_long < -20 || temp_data_long > 120 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  IAT12 exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_long = (pSid1[SidIndex].Data[3] ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  IAT13 = %d C\n", GetEcuId(EcuIndex), temp_data_long);
								if ( temp_data_long < -20 || temp_data_long > 120 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  IAT13 exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[4] ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  IAT21 = %d C\n", GetEcuId(EcuIndex), temp_data_long);
								if ( temp_data_long < -20 || temp_data_long > 120 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  IAT21 exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x10) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[5] ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  IAT22 = %d C\n", GetEcuId(EcuIndex), temp_data_long);
								if ( temp_data_long < -20 || temp_data_long > 120 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  IAT22 exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x20) != 0 )
							{
								temp_data_long =  (pSid1[SidIndex].Data[6] ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  IAT23 = %d C\n", GetEcuId(EcuIndex), temp_data_long);
								if ( temp_data_long < -20 || temp_data_long > 120 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  IAT23 exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;

						case 0x69:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EGR data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x3F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGR data support  No sensors supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xC0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGR data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_float = (float)pSid1[SidIndex].Data[1] * (float)(100.0 / 255.0);
								temp_EGR_CMD = temp_data_float;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGR_A_CMD = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

								if (gOBDEngineRunning == FALSE)  // test 5.6
								{
									if ( temp_data_float > 10 )
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  EGR_A_CMD > 10 %%\n", GetEcuId(EcuIndex) );
									}
								}
								else  // test 5.10 & 10.13
								{
									if ( gOBDDieselFlag == TRUE )  // compression ignition
									{
										if (temp_data_float > 100 )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  EGR_A_CMD > 100 %%\n", GetEcuId(EcuIndex) );
										}
									}
									else  // spark ignition
									{
										if ( gOBDHybridFlag == FALSE && temp_data_float > 10 )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  EGR_A_CMD > 10 %%\n", GetEcuId(EcuIndex) );
										}
										else if ( gOBDHybridFlag == TRUE && temp_data_float > 50 )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  EGR_A_CMD > 50 %%\n", GetEcuId(EcuIndex) );
										}
									}
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_float = (float)pSid1[SidIndex].Data[2] * (float)(100.0 / 255.0);
								temp_EGR_ACT = temp_data_float;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGR_A_ACT = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_float = (float)(pSid1[SidIndex].Data[3] - 128 ) * (float)(100.0 / 128.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGR_A_ERR = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

								// if EGR_CMD and EGR_ACT supported, check for proper EGR_ERR
								if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 && (pSid1[SidIndex].Data[0] & 0x02) != 0 )
								{
									if ( temp_EGR_CMD == 0 )
									{
										if ( temp_EGR_ACT == 0 && temp_data_float != 0 )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  EGR_A_ERR should be 0%% when EGR_A_CMD == 0%% and EGR_A_ACT == 0%%\n", GetEcuId(EcuIndex) );
										}
										else if ( temp_EGR_ACT > 0 && pSid1[SidIndex].Data[3] != 0xFF )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  EGR_A_ERR should be 99.2%% when EGR_A_CMD == 0%% and EGR_A_ACT > 0%%\n", GetEcuId(EcuIndex) );
										}
									}
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_float = (float)pSid1[SidIndex].Data[4] * (float)(100.0 / 255.0);
								temp_EGR_CMD = temp_data_float;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGR_B_CMD = %.3f %%\n", GetEcuId(EcuIndex), temp_data_float);

								if (gOBDEngineRunning == FALSE)  // test 5.6
								{
									if ( temp_data_float > 10 )
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  EGR_B_CMD > 10 %%\n", GetEcuId(EcuIndex) );
									}
								}
								else  // test 5.10 & 10.13
								{
									if ( gOBDDieselFlag == TRUE )  // compression ignition
									{
										if (temp_data_float > 100 )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  EGR_B_CMD > 100 %%\n", GetEcuId(EcuIndex) );
										}
									}
									else  // spark ignition
									{
										if ( gOBDHybridFlag == FALSE && temp_data_float > 10 )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  EGR_B_CMD > 10 %%\n", GetEcuId(EcuIndex) );
										}
										else if ( gOBDHybridFlag == TRUE && temp_data_float > 50 )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  EGR_B_CMD > 50 %%\n", GetEcuId(EcuIndex) );
										}
									}
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x10) != 0 )
							{
								temp_data_float = (float)pSid1[SidIndex].Data[5] * (float)(100.0 / 255.0);
								temp_EGR_ACT = temp_data_float;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGR_B_ACT = %.3f %%\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x20) != 0 )
							{
								temp_data_float = (float)( pSid1[SidIndex].Data[6] - 128) * (float)(100.0 / 128.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGR_B_ERR = %.3f %%\n", GetEcuId(EcuIndex), temp_data_float);

								// if EGR_CMD and EGR_ACT supported, check for proper EGR_ERR
								if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 && (pSid1[SidIndex].Data[0] & 0x10) != 0 )
								{
									if ( temp_EGR_CMD == 0 )
									{
										if ( temp_EGR_ACT == 0 && temp_data_float != 0 )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  EGR_B_ERR should be 0%% when EGR_B_CMD == 0%% and EGR_B_ACT == 0%%\n", GetEcuId(EcuIndex) );
										}
										else if ( temp_EGR_ACT > 0 && pSid1[SidIndex].Data[6] != 0xFF )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  EGR_B_ERR should be 99.2%% when EGR_B_CMD == 0%% and EGR_B_ACT > 0%%\n", GetEcuId(EcuIndex) );
										}
									}
								}
							}
						}
						break;

						case 0x6A:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  IAF data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x0F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  IAF data support  No sensors supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  IAF data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_float = (float)pSid1[SidIndex].Data[1] * (float)(100.0 / 255.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  IAF_A_CMD = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_float = (float)pSid1[SidIndex].Data[2] * (float)(100.0 / 255.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  IAF_A_REL = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_float = (float)pSid1[SidIndex].Data[3] * (float)(100.0 / 255.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  IAF_B_CMD = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_float = (float)pSid1[SidIndex].Data[4] * (float)(100.0 / 255.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  IAF_B_REL = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
							}
						}
						break;

						case 0x6B:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EGRT sensor support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0xFF) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGRT sensor support  No sensors supported\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x11) != 0 )
							{
								if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
								{
									temp_data_long = (pSid1[SidIndex].Data[1]) - 40;
								}
								else
								{
									temp_data_long = ((pSid1[SidIndex].Data[1]) * 4) - 40;
								}

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGRT11 = %d C\n", GetEcuId(EcuIndex), temp_data_long);

								if ( OBDEngineDontCare == FALSE )  // NOT Test 10.2
								{
									if ( gOBDEngineRunning == TRUE )  // test 5.10 & 10.13
									{
										if (
										     ( ((pSid1[SidIndex].Data[0] & 0x01) != 0) && (temp_data_long < -20 || temp_data_long > 215) ) ||
										     ( ((pSid1[SidIndex].Data[0] & 0x10) != 0) && (temp_data_long < -20 || temp_data_long > 500) )
										   )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  EGRT11 exceeded normal range\n", GetEcuId(EcuIndex) );
										}
									}
									else  // test 5.6
									{
										if ( temp_data_long < -20 || temp_data_long > 200 )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  EGRT11 exceeded normal range\n", GetEcuId(EcuIndex) );
										}
									}
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x22) != 0 )
							{
								if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
								{
									temp_data_long = (pSid1[SidIndex].Data[2]) - 40;
								}
								else
								{
									temp_data_long = ((pSid1[SidIndex].Data[2]) * 4) - 40;
								}

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGRT12 = %d C\n", GetEcuId(EcuIndex), temp_data_long);

								if ( OBDEngineDontCare == FALSE )  // NOT Test 10.2
								{
									if ( gOBDEngineRunning == TRUE )  // test 5.10 & 10.13
									{
										if (
										     ( ((pSid1[SidIndex].Data[0] & 0x02) != 0) && (temp_data_long < -20 || temp_data_long > 215) ) ||
										     ( ((pSid1[SidIndex].Data[0] & 0x20) != 0) && (temp_data_long < -20 || temp_data_long > 500) )
										   )
										{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  EGRT12 exceeded normal range\n", GetEcuId(EcuIndex) );
										}
									}
									else  // test 5.6
									{
										if ( temp_data_long < -20 || temp_data_long > 200 )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  EGRT12 exceeded normal range\n", GetEcuId(EcuIndex) );
										}
									}
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x44) != 0 )
							{
								if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
								{
									temp_data_long = (pSid1[SidIndex].Data[3]) - 40;
								}
								else
								{
									temp_data_long = ((pSid1[SidIndex].Data[3]) * 4) - 40;
								}

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGRT21 = %d C\n", GetEcuId(EcuIndex), temp_data_long);

								if ( OBDEngineDontCare == FALSE )  // NOT Test 10.2
								{
									if ( gOBDEngineRunning == TRUE )  // test 5.10 & 10.13
									{
										if (
										     ( ((pSid1[SidIndex].Data[0] & 0x04) != 0) && (temp_data_long < -20 || temp_data_long > 215) ) ||
										     ( ((pSid1[SidIndex].Data[0] & 0x40) != 0) && (temp_data_long < -20 || temp_data_long > 500) )
										   )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  EGRT21 exceeded normal range\n", GetEcuId(EcuIndex) );
										}
									}
									else  // test 5.6
									{
										if ( temp_data_long < -20 || temp_data_long > 200 )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  EGRT21 exceeded normal range\n", GetEcuId(EcuIndex) );
										}
									}
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x88) != 0 )
							{
								if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
								{
									temp_data_long = (pSid1[SidIndex].Data[4]) - 40;
								}
								else
								{
									temp_data_long = ((pSid1[SidIndex].Data[4]) * 4) - 40;
								}

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGRT22 = %d C\n", GetEcuId(EcuIndex), temp_data_long);

								if ( OBDEngineDontCare == FALSE )  // NOT Test 10.2
								{
									if ( gOBDEngineRunning == TRUE )  // test 5.10 & 10.13
									{
										if (
										     ( ((pSid1[SidIndex].Data[0] & 0x08) != 0) && (temp_data_long < -20 || temp_data_long > 215) ) ||
										     ( ((pSid1[SidIndex].Data[0] & 0x80) != 0) && (temp_data_long < -20 || temp_data_long > 500) )
										   )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  EGRT22 exceeded normal range\n", GetEcuId(EcuIndex) );
										}
									}
									else  // test 5.6
									{
										if ( temp_data_long < -20 || temp_data_long > 200 )
										{
											Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
											     "ECU %X  EGRT22 exceeded normal range\n", GetEcuId(EcuIndex) );
										}
									}
								}
							}
						}
						break;

						case 0x6C:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  TAC data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x0F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TAC data support  No sensors supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TAC data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_float = (float)pSid1[SidIndex].Data[1] * (float)(100.0 / 255.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TAC_A_CMD = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_float = (float)pSid1[SidIndex].Data[2] * (float)(100.0 / 255.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TP_A_REL = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

								if ( OBDEngineDontCare == FALSE && gOBDDieselFlag == FALSE && gOBDHybridFlag == FALSE &&
								     temp_data_float > 50 )  // NOT test 10.2
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  TP_A_REL exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_float = (float)pSid1[SidIndex].Data[3] * (float)(100.0 / 255.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TAC_B_CMD = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_float = (float)pSid1[SidIndex].Data[4] * (float)(100.0 / 255.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TP_B_REL = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);

								if ( OBDEngineDontCare == FALSE && gOBDDieselFlag == FALSE && gOBDHybridFlag == FALSE &&
								     temp_data_float > 50 )  // NOT Test 10.2
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  TP_B_REL exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;

						case 0x6D:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  FRP data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x3F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FRP data support  No sensors supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xC0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FRP data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_long = ( ( pSid1[SidIndex].Data[1] * 256 ) +
								                   pSid1[SidIndex].Data[2] ) * 10;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FRP_A_CMD = %d kPa\n", GetEcuId(EcuIndex), temp_data_long);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_long = ( ( pSid1[SidIndex].Data[3] * 256 ) +
								                   pSid1[SidIndex].Data[4] ) * 10;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FRP_A = %d kPa\n", GetEcuId(EcuIndex), temp_data_long);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[5] ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FRT_A = %d C\n", GetEcuId(EcuIndex), temp_data_long);
								if ( temp_data_long < -20 || temp_data_long > 120 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  FRT_A exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_long = ( ( pSid1[SidIndex].Data[6] * 256 ) +
								                   pSid1[SidIndex].Data[7] ) * 10;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FRP_B_CMD = %d kPa\n", GetEcuId(EcuIndex), temp_data_long);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x10) != 0 )
							{
								temp_data_long = ( ( pSid1[SidIndex].Data[8] * 256 ) +
								                   pSid1[SidIndex].Data[9] ) * 10;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FRP_B = %d kPa\n", GetEcuId(EcuIndex), temp_data_long);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x20) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[10] ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FRT_B = %d C\n", GetEcuId(EcuIndex), temp_data_long);
								if ( temp_data_long < -20 || temp_data_long > 120 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  FRT_B exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;

						case 0x6E:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  ICP system data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x0F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  ICP data support  No sensors supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  ICP data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_long = ( ( pSid1[SidIndex].Data[1] * 256 ) +
								                   pSid1[SidIndex].Data[2] ) * 10;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  ICP_A_CMD = %d kPa\n", GetEcuId(EcuIndex), temp_data_long);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_long = ( ( pSid1[SidIndex].Data[3] * 256 ) +
								                   pSid1[SidIndex].Data[4] ) * 10;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  ICP_A = %d kPa\n", GetEcuId(EcuIndex), temp_data_long);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_long = ( ( pSid1[SidIndex].Data[5] * 256 ) +
								                   pSid1[SidIndex].Data[6] ) * 10;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  ICP_B_CMD = %d kPa\n", GetEcuId(EcuIndex), temp_data_long);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_long = ( ( pSid1[SidIndex].Data[7] * 256 ) +
								                   pSid1[SidIndex].Data[8] ) * 10;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  ICP_B = %d kPa\n", GetEcuId(EcuIndex), temp_data_long);
							}
						}
						break;

						case 0x6F:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  TCA sensor support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x03) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TCA sensor support  No sensors supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xFC )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TCA sensor support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_long = pSid1[SidIndex].Data[1];
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TCA_CINP = %d kPa\n", GetEcuId(EcuIndex), temp_data_long);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_long = pSid1[SidIndex].Data[2];
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TCB_CINP = %d kPa\n", GetEcuId(EcuIndex), temp_data_long);
							}
						}
						break;

						case 0x70:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  BP data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x3F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  BP data support  No sensors supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xC0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  BP data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_float = ( ( pSid1[SidIndex].Data[1] * 256 ) +
								                    pSid1[SidIndex].Data[2] ) * (float)0.03125;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  BP_A_CMD = %.3f kPa\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_float = ( ( pSid1[SidIndex].Data[3] * 256 ) +
								                    pSid1[SidIndex].Data[4] ) * (float)0.03125;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  BP_A_ACT = %.3f kPa\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_float = ( ( pSid1[SidIndex].Data[5] * 256 ) +
								                    pSid1[SidIndex].Data[6] ) * (float)0.03125;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  BP_B_CMD = %.3f kPa\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x10) != 0 )
							{
								temp_data_float = ( ( pSid1[SidIndex].Data[7] * 256 ) +
								                    pSid1[SidIndex].Data[8] ) * (float)0.03125;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  BP_B_ACT = %.3f kPa\n", GetEcuId(EcuIndex), temp_data_float);
							}

							/* bit 2 (0x04) BP_A Status and bit 5 (0x20) BP_B Status */
							if ( (pSid1[SidIndex].Data[0] & 0x24) != 0 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  BP control status = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[9] );
								/* check that reserved bits are not set */
								if ( pSid1[SidIndex].Data[9] & 0xF0 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  BP control status  Reserved bits set\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;

						case 0x71:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  VGT data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x3F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  VGT data support  No sensors supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xC0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  VGT data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_float = (float)pSid1[SidIndex].Data[1] * (float)(100.0 / 255.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  VGT_A_CMD = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_float = (float)pSid1[SidIndex].Data[2] * (float)(100.0 / 255.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  VGT_A_ACT = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_float = (float)pSid1[SidIndex].Data[3] * (float)(100.0 / 255.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  VGT_B_CMD = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x10) != 0 )
							{
								temp_data_float = (float)pSid1[SidIndex].Data[4] * (float)(100.0 / 255.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  VGT_B_ACT = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
							}

							/* bit 2 (0x04) VGT_A Status and bit 5 (0x20) VGT_B Status */
							if ( (pSid1[SidIndex].Data[0] & 0x24) != 0 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  VGT control status = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[5] );
								/* check that reserved bits are not set */
								if ( pSid1[SidIndex].Data[5] & 0xF0 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  VGT control status  Reserved bits set\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;

						case 0x72:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  WG data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x0F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  WG data support  No sensors supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  WG data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_float = (float)pSid1[SidIndex].Data[1] * (float)(100.0 / 255.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  WG_A_CMD = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_float = (float)pSid1[SidIndex].Data[2] * (float)(100.0 / 255.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  WG_A_ACT = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_float = (float)pSid1[SidIndex].Data[3] * (float)(100.0 / 255.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  WG_B_CMD = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_float = (float)pSid1[SidIndex].Data[4] * (float)(100.0 / 255.0);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  WG_B_ACT = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
							}
						}
						break;

						case 0x73:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EP data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x03) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EP data support  No sensors supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xFC )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EP data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_float = ( (pSid1[SidIndex].Data[1] * 256) +
								                    pSid1[SidIndex].Data[2] ) * (float)0.01;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EP1 = %.2f kPa\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_float = ( (pSid1[SidIndex].Data[3] * 256) +
								                    pSid1[SidIndex].Data[4] ) * (float)0.01;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EP2 = %.2f kPa\n", GetEcuId(EcuIndex), temp_data_float);
							}
						}
						break;

						case 0x74:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  TC_RPM data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x03) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TC_RPM data support  No data supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xFC )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TC_RPM data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_long = (pSid1[SidIndex].Data[1] * 256) +
								                  pSid1[SidIndex].Data[2];
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TCA_RPM = %d RPM\n", GetEcuId(EcuIndex), temp_data_long);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_long = (pSid1[SidIndex].Data[3] * 256) +
								                  pSid1[SidIndex].Data[4];
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TCB_RPM = %d RPM\n", GetEcuId(EcuIndex), temp_data_long);
							}
						}
						break;

						case 0x75:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  TC A Temp data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x0F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TC A Temp data support  No data supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TC A Temp data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[1] ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TCA_CINT = %d C\n", GetEcuId(EcuIndex), temp_data_long);
								if ( temp_data_long < -20 || temp_data_long > 120 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  TCA_CINT exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[2] ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TCA_COUTT = %d C\n", GetEcuId(EcuIndex), temp_data_long);
								if ( temp_data_long < -20 || temp_data_long > 120 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  TCA_COUTT exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_float = ( ( (pSid1[SidIndex].Data[3] * 256) +
								                      pSid1[SidIndex].Data[4] ) * (float)0.1 ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TCA_TINT = %.1f C\n", GetEcuId(EcuIndex), temp_data_float);
								if ( temp_data_float < -20 || temp_data_float > 1000 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  TCA_TINT exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_float = ( ( (pSid1[SidIndex].Data[5] * 256) +
								                      pSid1[SidIndex].Data[6] ) * (float)0.1 ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TCA_TOUTT = %.1f C\n", GetEcuId(EcuIndex), temp_data_float);
								if ( temp_data_float < -20 || temp_data_float > 1000 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  TCA_TOUTT exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;

						case 0x76:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  TC B Temp data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x0F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TC B Temp data support  No data supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TC B Temp data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[1] ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TCB_CINT = %d C\n", GetEcuId(EcuIndex), temp_data_long);
								if ( temp_data_long < -20 || temp_data_long > 120 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  TCB_CINT exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[2] ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TCB_COUTT = %d C\n", GetEcuId(EcuIndex), temp_data_long);
								if ( temp_data_long < -20 || temp_data_long > 120 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  TCB_COUTT exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_float = ( ( (pSid1[SidIndex].Data[3] * 256) +
								                      pSid1[SidIndex].Data[4] ) * (float)0.1 ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TCB_TINT = %.1f C\n", GetEcuId(EcuIndex), temp_data_float);
								if ( temp_data_float < -20 || temp_data_float > 1000 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  TCB_TINT exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_float = ( ( (pSid1[SidIndex].Data[5] * 256) +
								                      pSid1[SidIndex].Data[6] ) * (float)0.1 ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  TCB_TOUTT = %.1f C\n", GetEcuId(EcuIndex), temp_data_float);
								if ( temp_data_float < -20 || temp_data_float > 1000 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  TCB_TOUTT exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;

						case 0x77:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  CACT data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x0F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  CACT data support  No data supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  CACT data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[1] ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  CACT11 = %d C\n", GetEcuId(EcuIndex), temp_data_long);
								if ( temp_data_long < -20 || temp_data_long > 120 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  CACT11 exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[2] ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  CACT12 = %d C\n", GetEcuId(EcuIndex), temp_data_long);
								if ( temp_data_long < -20 || temp_data_long > 120 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  CACT12 exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[3] ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  CACT21 = %d C\n", GetEcuId(EcuIndex), temp_data_long);
								if ( temp_data_long < -20 || temp_data_long > 120 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  CACT21 exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[4] ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  CACT22 = %d C\n", GetEcuId(EcuIndex), temp_data_long);
								if ( temp_data_long < -20 || temp_data_long > 120 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  CACT22 exceeded normal range\n", GetEcuId(EcuIndex) );
								}
							}
						}
						break;

						case 0x78:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EGT Bank 1 data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x0F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGT Bank 1 data support  No data supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGT Bank 1 data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_float = ( ( (pSid1[SidIndex].Data[1] * 256) +
								                      pSid1[SidIndex].Data[2] ) * (float).1 ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGT11 = %.1f C\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_float = ( ( (pSid1[SidIndex].Data[3] * 256) +
								                      pSid1[SidIndex].Data[4] ) * (float).1 ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGT12 = %.1f C\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_float = ( ( (pSid1[SidIndex].Data[5] * 256) +
								                      pSid1[SidIndex].Data[6] ) * (float).1 ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGT13 = %.1f C\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_float = ( ( (pSid1[SidIndex].Data[7] * 256) +
								                      pSid1[SidIndex].Data[8] ) * (float).1 ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGT14 = %.1f C\n", GetEcuId(EcuIndex), temp_data_float);
							}
						}
						break;

						case 0x79:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EGT Bank 2 data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x0F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGT Bank 2 data support  No data supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGT Bank 2 data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_float = ( ( (pSid1[SidIndex].Data[1] * 256) +
								                      pSid1[SidIndex].Data[2] ) * (float)0.1 ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGT21 = %.1f C\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_float = ( ( (pSid1[SidIndex].Data[3] * 256) +
								                      pSid1[SidIndex].Data[4] ) * (float)0.1 ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGT22 = %.1f C\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_float = ( ( (pSid1[SidIndex].Data[5] * 256) +
								                      pSid1[SidIndex].Data[6] ) * (float)0.1 ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGT23 = %.1f C\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_float = ( ( (pSid1[SidIndex].Data[7] * 256) +
								                      pSid1[SidIndex].Data[8] ) * (float)0.1 ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGT24 = %.1f C\n", GetEcuId(EcuIndex), temp_data_float);
							}
						}
						break;

						case 0x7A:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  DPF Bank 1 data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x07) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DPF Bank 1 data support  No data supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF8 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DPF Bank 1 data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_float = (float)((signed short)( (pSid1[SidIndex].Data[1] * 256) +
								                                          pSid1[SidIndex].Data[2] )) * (float)0.01;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DPF1_DP = %.1f kPa\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_float = ( ( ( pSid1[SidIndex].Data[3] ) * 256 ) +
								                      pSid1[SidIndex].Data[4] ) * (float)0.01;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DPF1_INP = %.1f kPa\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_float = ( ( ( pSid1[SidIndex].Data[5] ) * 256 ) +
								                      pSid1[SidIndex].Data[6] ) * (float)0.01;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DPF1_OUTP = %.1f kPa\n", GetEcuId(EcuIndex), temp_data_float);
							}
						}
						break;

						case 0x7B:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  DPF Bank 2 data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x07) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DPF Bank 2 data support  No data supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF8 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DPF Bank 2 data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_float = (float)((signed short)( (pSid1[SidIndex].Data[1] * 256) +
								                                          pSid1[SidIndex].Data[2] )) * (float)0.01;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DPF2_DP = %.1f kPa\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_float = ( ( ( pSid1[SidIndex].Data[3] ) * 256 ) +
								                      pSid1[SidIndex].Data[4] ) * (float)0.01;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DPF2_INP = %.1f kPa\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_float = ( ( ( pSid1[SidIndex].Data[5] ) * 256 ) +
								                      pSid1[SidIndex].Data[6] ) * (float)0.01;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DPF2_OUTP = %.1f kPa\n", GetEcuId(EcuIndex), temp_data_float);
							}
						}
						break;

						case 0x7C:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  DPF Temp data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x0F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DPF Temp data support  No data supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DPF Temp data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_float = ( ( (pSid1[SidIndex].Data[1] * 256) +
								                      pSid1[SidIndex].Data[2] ) * (float)0.1 ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DPF1_INT = %.1f C\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_float = ( ( (pSid1[SidIndex].Data[3] * 256) +
								                      pSid1[SidIndex].Data[4] ) * (float)0.1 ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DPF1_OUTT = %.1f C\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_float = ( ( (pSid1[SidIndex].Data[5] * 256) +
								                      pSid1[SidIndex].Data[6] ) * (float)0.1 ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DPF2_INT = %.1f C\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_float = ( ( (pSid1[SidIndex].Data[7] * 256) +
								                      pSid1[SidIndex].Data[8] ) * (float)0.1 ) - 40;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DPF2_OUTT = %.1f C\n", GetEcuId(EcuIndex), temp_data_float);
							}
						}
						break;

						case 0x7D:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  NOx NTE control area status = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  NOx NTE control area status  Reserved bits set\n", GetEcuId(EcuIndex) );
							}
						}
						break;

						case 0x7E:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  PM NTE control area status = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  PM NTE control area status  Reserved bits set\n", GetEcuId(EcuIndex) );
							}
						}
						break;

						case 0x7F:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Run Time support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x07) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Run Time data support  No data supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF8 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Run Time support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[1] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[2] * 65536 ) +
								                 ( pSid1[SidIndex].Data[3] * 256 ) +
								                 pSid1[SidIndex].Data[4];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  RUN_TIME = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[5] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[6] * 65536 ) +
								                 ( pSid1[SidIndex].Data[7] * 256 ) +
								                 pSid1[SidIndex].Data[8];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  IDLE_TIME = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[9] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[10] * 65536 ) +
								                 ( pSid1[SidIndex].Data[11] * 256 ) +
								                 pSid1[SidIndex].Data[12];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  PTO_TIME = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}
						}
						break;

						case 0x81:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Run Time for AECD #1-#5 support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x1F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Run Time for AECD #1-#5 support  No data supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xE0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Run Time for AECD #1-#5 support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[1] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[2] * 65536 ) +
								                 ( pSid1[SidIndex].Data[3] * 256 ) +
								                 pSid1[SidIndex].Data[4];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}

								temp_data_long = ( pSid1[SidIndex].Data[5] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[6] * 65536 ) +
								                 ( pSid1[SidIndex].Data[7] * 256 ) +
								                 pSid1[SidIndex].Data[8];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME2 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[9] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[10] * 65536 ) +
								                 ( pSid1[SidIndex].Data[11] * 256 ) +
								                 pSid1[SidIndex].Data[12];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD2_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}

								temp_data_long = ( pSid1[SidIndex].Data[13] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[14] * 65536 ) +
								                 ( pSid1[SidIndex].Data[15] * 256 ) +
								                 pSid1[SidIndex].Data[16];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD2_TIME2 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[17] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[18] * 65536 ) +
								                 ( pSid1[SidIndex].Data[19] * 256 ) +
								                 pSid1[SidIndex].Data[20];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD3_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}

								temp_data_long = ( pSid1[SidIndex].Data[21] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[22] * 65536 ) +
								                 ( pSid1[SidIndex].Data[23] * 256 ) +
								                 pSid1[SidIndex].Data[24];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD3_TIME2 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[25] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[26] * 65536 ) +
								                 ( pSid1[SidIndex].Data[27] * 256 ) +
								                 pSid1[SidIndex].Data[28];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD4_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}

								temp_data_long = ( pSid1[SidIndex].Data[29] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[30] * 65536 ) +
								                 ( pSid1[SidIndex].Data[31] * 256 ) +
								                 pSid1[SidIndex].Data[32];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD4_TIME2 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x10) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[33] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[34] * 65536 ) +
								                 ( pSid1[SidIndex].Data[35] * 256 ) +
								                 pSid1[SidIndex].Data[36];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD5_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}

								temp_data_long = ( pSid1[SidIndex].Data[37] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[38] * 65536 ) +
								                 ( pSid1[SidIndex].Data[39] * 256 ) +
								                 pSid1[SidIndex].Data[40];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD5_TIME2 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}
						}
						break;

						case 0x82:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Run Time for AECD #6-#10 support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x1F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Run Time for AECD #6-#10 support  No data supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xE0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Run Time for AECD #6-#10 support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[1] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[2] * 65536 ) +
								                 ( pSid1[SidIndex].Data[3] * 256 ) +
								                 pSid1[SidIndex].Data[4];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD6_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}

								temp_data_long = ( pSid1[SidIndex].Data[5] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[6] * 65536 ) +
								                 ( pSid1[SidIndex].Data[7] * 256 ) +
								                 pSid1[SidIndex].Data[8];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD6_TIME2 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[9] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[10] * 65536 ) +
								                 ( pSid1[SidIndex].Data[11] * 256 ) +
								                 pSid1[SidIndex].Data[12];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD7_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}

								temp_data_long = ( pSid1[SidIndex].Data[13] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[14] * 65536 ) +
								                 ( pSid1[SidIndex].Data[15] * 256 ) +
								                 pSid1[SidIndex].Data[16];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD7_TIME2 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[17] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[18] * 65536 ) +
								                 ( pSid1[SidIndex].Data[19] * 256 ) +
								                 pSid1[SidIndex].Data[20];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD8_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}

								temp_data_long = ( pSid1[SidIndex].Data[21] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[22] * 65536 ) +
								                 ( pSid1[SidIndex].Data[23] * 256 ) +
								                 pSid1[SidIndex].Data[24];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD8_TIME2 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[25] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[26] * 65536 ) +
								                 ( pSid1[SidIndex].Data[27] * 256 ) +
								                 pSid1[SidIndex].Data[28];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD9_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}

								temp_data_long = ( pSid1[SidIndex].Data[29] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[30] * 65536 ) +
								                 ( pSid1[SidIndex].Data[31] * 256 ) +
								                 pSid1[SidIndex].Data[32];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD9_TIME2 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x10) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[33] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[34] * 65536 ) +
								                 ( pSid1[SidIndex].Data[35] * 256 ) +
								                 pSid1[SidIndex].Data[36];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD10_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}

								temp_data_long = ( pSid1[SidIndex].Data[37] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[38] * 65536 ) +
								                 ( pSid1[SidIndex].Data[39] * 256 ) +
								                 pSid1[SidIndex].Data[40];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD10_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}
						}
						break;

						case 0x83:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  NOx Sensor Data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x0F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  NOx Sensor Data support  No data supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  NOx Sensor Data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_long = (pSid1[SidIndex].Data[1] * 256) +
								                  pSid1[SidIndex].Data[2];
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  NOX11 = %d ppm\n", GetEcuId(EcuIndex), temp_data_long);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_long = (pSid1[SidIndex].Data[3] * 256) +
								                  pSid1[SidIndex].Data[4];
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  NOX12 = %d ppm\n", GetEcuId(EcuIndex), temp_data_long);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_long = (pSid1[SidIndex].Data[1] * 256) +
								                  pSid1[SidIndex].Data[2];
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  NOX21 = %d ppm\n", GetEcuId(EcuIndex), temp_data_long);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_long = (pSid1[SidIndex].Data[3] * 256) +
								                  pSid1[SidIndex].Data[4];
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  NOX22 = %d ppm\n", GetEcuId(EcuIndex), temp_data_long);
							}
						}
						break;

						case 0x84:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  MST = %d C\n", GetEcuId(EcuIndex), ( pSid1[SidIndex].Data[0] ) -40 );
						}
						break;

						case 0x85:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  NOx Reagent data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x0F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  NOx Reagent data support  No data supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  NOx Reagent data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_float = ( ( pSid1[SidIndex].Data[1] * 256 ) +
								                    pSid1[SidIndex].Data[2] ) * (float)0.005;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  REAG_RATE = %.3f L/h\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_float = ( (pSid1[SidIndex].Data[3] * 256) +
								                    pSid1[SidIndex].Data[4] ) * (float)0.005;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  REAG_DEMD = %.3f L/h\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[5] * 100) / 255;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  REAG_LVL = %d %%\n", GetEcuId(EcuIndex), temp_data_long);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[6] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[7] * 65536 ) +
								                 ( pSid1[SidIndex].Data[8] * 256 ) +
								                 pSid1[SidIndex].Data[9];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  NWI_TIME = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}
						}
						break;

						case 0x86:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  PM data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x03) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  PM data support  No data supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xFC )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  PM data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_float = ( ( pSid1[SidIndex].Data[1] * 256 ) +
								                    pSid1[SidIndex].Data[2] ) * (float)0.0125;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  PM11 = %.3f mg/m3\n", GetEcuId(EcuIndex), temp_data_float);
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_float = ( (pSid1[SidIndex].Data[3] * 256) +
								                    pSid1[SidIndex].Data[4] ) * (float)0.0125;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  PM21 = %.3f mg/m3\n", GetEcuId(EcuIndex), temp_data_float);
							}
						}
						break;

						case 0x87:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  MAP data support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x03) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  MAP data support  No data supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xFC )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  MAP data support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_float = ( ( pSid1[SidIndex].Data[1] * 256 ) +
								                    pSid1[SidIndex].Data[2] ) * (float)0.03125;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  MAP_A = %.3f kPa\n", GetEcuId(EcuIndex), temp_data_float);

								if ( OBDEngineDontCare == FALSE && gOBDEngineRunning == TRUE )  // test 5.10 & 10.13
								{
									if ( (temp_data_float == 0.0) && (gOBDHybridFlag != TRUE) )
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  MAP_A must be non zero\n", GetEcuId(EcuIndex) );
									}
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_float = ( (pSid1[SidIndex].Data[3] * 256) +
								                    pSid1[SidIndex].Data[4] ) * (float)0.03125;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  MAP_B = %.3f kPa\n", GetEcuId(EcuIndex), temp_data_float);

								if ( OBDEngineDontCare == FALSE && gOBDEngineRunning == TRUE )  // test 5.10 & 10.13
								{
									if ( (temp_data_float == 0.0) && (gOBDHybridFlag != TRUE) )
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  MAP_B must be non zero\n", GetEcuId(EcuIndex) );
									}
								}
							}
						}
						break;

						case 0x88:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SCR_INDUCE_SYSTEM = $%02X\n",
							     GetEcuId(EcuIndex),
							     pSid1[SidIndex].Data[0] );

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SCR_INDUCE_SYSTEM_HIST12 = $%02X\n",
							     GetEcuId(EcuIndex),
							     pSid1[SidIndex].Data[1] );

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SCR_INDUCE_SYSTEM_HIST34 = $%02X\n",
							     GetEcuId(EcuIndex),
							     pSid1[SidIndex].Data[2] );

							temp_data_long = ( (pSid1[SidIndex].Data[3] * 256) +
							                    pSid1[SidIndex].Data[4] );
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SCR_INDUCE_DIST_1N = %d km\n",
							     GetEcuId(EcuIndex),
							     temp_data_long);

							temp_data_long = ( (pSid1[SidIndex].Data[5] * 256) +
							                    pSid1[SidIndex].Data[6] );
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SCR_INDUCE_DIST_1D = %d km\n",
							     GetEcuId(EcuIndex),
							     temp_data_long);

							temp_data_long = ( (pSid1[SidIndex].Data[7] * 256) +
							                    pSid1[SidIndex].Data[8] );
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SCR_INDUCE_DIST_2N = %d km\n",
							     GetEcuId(EcuIndex),
							     temp_data_long);

							temp_data_long = ( (pSid1[SidIndex].Data[9] * 256) +
							                    pSid1[SidIndex].Data[10] );
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SCR_INDUCE_DIST_3N = %d km\n",
							     GetEcuId(EcuIndex),
							     temp_data_long);

							temp_data_long = ( (pSid1[SidIndex].Data[11] * 256) +
							                    pSid1[SidIndex].Data[12] );
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SCR_INDUCE_DIST_4N = %d km\n",
							     GetEcuId(EcuIndex),
							     temp_data_long);
						}
						break;

						case 0x89:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Run Time for AECD #11-#15 support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x1F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Run Time for AECD #11-#15 support  No data supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xE0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Run Time for AECD #11-#15 support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[1] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[2] * 65536 ) +
								                 ( pSid1[SidIndex].Data[3] * 256 ) +
								                 pSid1[SidIndex].Data[4];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD11_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}

								temp_data_long = ( pSid1[SidIndex].Data[5] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[6] * 65536 ) +
								                 ( pSid1[SidIndex].Data[7] * 256 ) +
								                 pSid1[SidIndex].Data[8];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD11_TIME2 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[9] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[10] * 65536 ) +
								                 ( pSid1[SidIndex].Data[11] * 256 ) +
								                 pSid1[SidIndex].Data[12];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD12_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}

								temp_data_long = ( pSid1[SidIndex].Data[13] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[14] * 65536 ) +
								                 ( pSid1[SidIndex].Data[15] * 256 ) +
								                 pSid1[SidIndex].Data[16];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD12_TIME2 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[17] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[18] * 65536 ) +
								                 ( pSid1[SidIndex].Data[19] * 256 ) +
								                 pSid1[SidIndex].Data[20];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD13_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}

								temp_data_long = ( pSid1[SidIndex].Data[21] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[22] * 65536 ) +
								                 ( pSid1[SidIndex].Data[23] * 256 ) +
								                 pSid1[SidIndex].Data[24];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD13_TIME2 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[25] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[26] * 65536 ) +
								                 ( pSid1[SidIndex].Data[27] * 256 ) +
								                 pSid1[SidIndex].Data[28];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD14_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}

								temp_data_long = ( pSid1[SidIndex].Data[29] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[30] * 65536 ) +
								                 ( pSid1[SidIndex].Data[31] * 256 ) +
								                 pSid1[SidIndex].Data[32];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD14_TIME2 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x10) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[33] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[34] * 65536 ) +
								                 ( pSid1[SidIndex].Data[35] * 256 ) +
								                 pSid1[SidIndex].Data[36];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD15_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}

								temp_data_long = ( pSid1[SidIndex].Data[37] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[38] * 65536 ) +
								                 ( pSid1[SidIndex].Data[39] * 256 ) +
								                 pSid1[SidIndex].Data[40];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD15_TIME2 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}
						}
						break;

						case 0x8A:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Run Time for AECD #16-#20 support = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x1F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Run Time for AECD #16-#20 support  No data supported\n", GetEcuId(EcuIndex) );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xE0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Run Time for AECD #16-#20 support  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							if ( (pSid1[SidIndex].Data[0] & 0x01) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[1] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[2] * 65536 ) +
								                 ( pSid1[SidIndex].Data[3] * 256 ) +
								                 pSid1[SidIndex].Data[4];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD16_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}

								temp_data_long = ( pSid1[SidIndex].Data[5] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[6] * 65536 ) +
								                 ( pSid1[SidIndex].Data[7] * 256 ) +
								                 pSid1[SidIndex].Data[8];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD16_TIME2 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x02) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[9] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[10] * 65536 ) +
								                 ( pSid1[SidIndex].Data[11] * 256 ) +
								                 pSid1[SidIndex].Data[12];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD17_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}

								temp_data_long = ( pSid1[SidIndex].Data[13] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[14] * 65536 ) +
								                 ( pSid1[SidIndex].Data[15] * 256 ) +
								                 pSid1[SidIndex].Data[16];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD17_TIME2 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x04) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[17] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[18] * 65536 ) +
								                 ( pSid1[SidIndex].Data[19] * 256 ) +
								                 pSid1[SidIndex].Data[20];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD18_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}

								temp_data_long = ( pSid1[SidIndex].Data[21] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[22] * 65536 ) +
								                 ( pSid1[SidIndex].Data[23] * 256 ) +
								                 pSid1[SidIndex].Data[24];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD18_TIME2 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x08) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[25] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[26] * 65536 ) +
								                 ( pSid1[SidIndex].Data[27] * 256 ) +
								                 pSid1[SidIndex].Data[28];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD19_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}

								temp_data_long = ( pSid1[SidIndex].Data[29] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[30] * 65536 ) +
								                 ( pSid1[SidIndex].Data[31] * 256 ) +
								                 pSid1[SidIndex].Data[32];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD19_TIME2 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}

							if ( (pSid1[SidIndex].Data[0] & 0x10) != 0 )
							{
								temp_data_long = ( pSid1[SidIndex].Data[33] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[34] * 65536 ) +
								                 ( pSid1[SidIndex].Data[35] * 256 ) +
								                 pSid1[SidIndex].Data[36];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD20_TIME1 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}

								temp_data_long = ( pSid1[SidIndex].Data[37] * 16777216 ) +
								                 ( pSid1[SidIndex].Data[38] * 65536 ) +
								                 ( pSid1[SidIndex].Data[39] * 256 ) +
								                 pSid1[SidIndex].Data[40];
								if ( temp_data_long == -1 )
								{
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD1_TIME1 = n.a.\n" );
								}
								else
								{
									GetHoursMinsSecs( temp_data_long, &hours, &mins, &secs);
									Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  AECD20_TIME2 = %d sec (%d hrs, %d mins, %d secs)\n",
									     GetEcuId(EcuIndex),
									     temp_data_long,
									     hours, mins, secs );
								}
							}
						}
						break;

						case 0x8B:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Diesel Aftertreatment Status support (BYTE A) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );
							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0x80 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Diesel Aftertreatment Status support (BYTE A)  Reserved bits set\n", GetEcuId(EcuIndex) );
							}

							/* Diesel Aftertreatment Status bits */
							if ( pSid1[SidIndex].Data[0] & 0x01 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         DPF_REGEN_STATUS = %s\n",
								     (pSid1[SidIndex].Data[1] & 0x01) ? "YES" : "NO" );
							}
							if ( pSid1[SidIndex].Data[0] & 0x02 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         DPF_REGEN_TYPE = %s\n",
								     (pSid1[SidIndex].Data[1] & 0x02) ? "ACTIVE" : "PASSIVE" );
							}
							if ( pSid1[SidIndex].Data[0] & 0x04 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         NOX_ADS_REGEN = %s\n",
								     (pSid1[SidIndex].Data[1] & 0x04) ? "YES" : "NO" );
							}
							if ( pSid1[SidIndex].Data[0] & 0x08 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         NOX_ADS_DESULF = %s\n",
								     (pSid1[SidIndex].Data[1] & 0x08) ? "YES" : "NO" );
							}
							if ( pSid1[SidIndex].Data[0] & 0x10 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DPF_REGEN_PCT = %.1f %%\n",
								     GetEcuId(EcuIndex),
								     (float)pSid1[SidIndex].Data[2] * (float)(100.0 / 255.0) );
							}
							if ( pSid1[SidIndex].Data[0] & 0x20 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DPF_REGEN_AVGT = %d minutes\n",
								     GetEcuId(EcuIndex),
								     (pSid1[SidIndex].Data[3] * 256) +
								     pSid1[SidIndex].Data[4] );
							}
							if ( pSid1[SidIndex].Data[0] & 0x40 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DPF_REGEN_AVGD = %d km\n",
								     GetEcuId(EcuIndex),
								     (pSid1[SidIndex].Data[5] * 256) +
								     pSid1[SidIndex].Data[6] );
							}
							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[1] & 0xF0 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  Diesel Aftertreatment Status (BYTE B) Reserved bits set\n", GetEcuId(EcuIndex) );
							}
						}
						break;

						case 0x8C:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  O2 Sensor (Wide Range) support (BYTE A) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );

							/* O2 Sensor (Wide Range) support bits */
							if ( pSid1[SidIndex].Data[0] & 0x01 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         Concentration S11 = %s\n",
								     (pSid1[SidIndex].Data[0] & 0x01) ? "YES" : "NO" );
							}
							if ( pSid1[SidIndex].Data[0] & 0x02 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         Concentration S12 = %s\n",
								     (pSid1[SidIndex].Data[0] & 0x02) ? "YES" : "NO" );
							}
							if ( pSid1[SidIndex].Data[0] & 0x04 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         Concentration S21 = %s\n",
								     (pSid1[SidIndex].Data[0] & 0x04) ? "YES" : "NO" );
							}
							if ( pSid1[SidIndex].Data[0] & 0x08 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         Concentration S22 = %s\n",
								     (pSid1[SidIndex].Data[0] & 0x08) ? "YES" : "NO" );
							}
							if ( pSid1[SidIndex].Data[0] & 0x10 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         Lambda S11 = %s\n",
								     (pSid1[SidIndex].Data[0] & 0x10) ? "YES" : "NO" );
							}
							if ( pSid1[SidIndex].Data[0] & 0x20 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         Lambda S12 = %s\n",
								     (pSid1[SidIndex].Data[0] & 0x20) ? "YES" : "NO" );
							}
							if ( pSid1[SidIndex].Data[0] & 0x40 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         Lambda S21 = %s\n",
								     (pSid1[SidIndex].Data[0] & 0x40) ? "YES" : "NO" );
							}
							if ( pSid1[SidIndex].Data[0] & 0x80 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         Lambda S22 = %s\n",
								     (pSid1[SidIndex].Data[0] & 0x80) ? "YES" : "NO" );
							}

							/* display associated values */
							if ( pSid1[SidIndex].Data[0] & 0x01 )
							{
								temp_data_float = (float)((pSid1[SidIndex].Data[1] * 256) + pSid1[SidIndex].Data[2]) * (float)(.001526);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         O2S11_PCT = %.6f %%\n", temp_data_float);
							}
							if ( pSid1[SidIndex].Data[0] & 0x02 )
							{
								temp_data_float = (float)((pSid1[SidIndex].Data[3] * 256) + pSid1[SidIndex].Data[4]) * (float)(.001526);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         O2S12_PCT = %.6f %%\n", temp_data_float);
							}
							if ( pSid1[SidIndex].Data[0] & 0x04 )
							{
								temp_data_float = (float)((pSid1[SidIndex].Data[5] * 256) + pSid1[SidIndex].Data[6]) * (float)(.001526);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         O2S21_PCT = %.6f %%\n", temp_data_float);
							}
							if ( pSid1[SidIndex].Data[0] & 0x08 )
							{
								temp_data_float = (float)((pSid1[SidIndex].Data[7] * 256) + pSid1[SidIndex].Data[8]) * (float)(.001526);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         O2S22_PCT = %.6f %%\n", temp_data_float);
							}
							if ( pSid1[SidIndex].Data[0] & 0x10 )
							{
								temp_data_float = (float)((pSid1[SidIndex].Data[9] * 256) + pSid1[SidIndex].Data[10]) * (float)(.000122);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         LAMBDA11 = %.3f\n", temp_data_float);
							}
							if ( pSid1[SidIndex].Data[0] & 0x20 )
							{
								temp_data_float = (float)((pSid1[SidIndex].Data[11] * 256) + pSid1[SidIndex].Data[12]) * (float)(.000122);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         LAMBDA12 = %.3f\n", temp_data_float);
							}
							if ( pSid1[SidIndex].Data[0] & 0x40 )
							{
								temp_data_float = (float)((pSid1[SidIndex].Data[13] * 256) + pSid1[SidIndex].Data[14]) * (float)(.000122);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         LAMBDA21 = %.3f\n", temp_data_float);
							}
							if ( pSid1[SidIndex].Data[0] & 0x80 )
							{
								temp_data_float = (float)((pSid1[SidIndex].Data[15] * 256) + pSid1[SidIndex].Data[16]) * (float)(.000122);
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         LAMBDA22 = %.3f\n", temp_data_float);
							}
						}
						break;

						case 0x8D:
						{
							temp_data_float = (float)pSid1[SidIndex].Data[0] * (float)(100.0 / 255.0);
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  TP_G = %.1f %%\n", GetEcuId(EcuIndex), temp_data_float);
						}
						break;

						case 0x8E:
						{
							temp_data_long = (signed long)(pSid1[SidIndex].Data[0]) - 125;
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  TQ_FR = %d %%\n", GetEcuId(EcuIndex), temp_data_long);
						}
						break;

						case 0x8F:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  PM Sensor support (BYTE A) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );

							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x0F) == 0 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         PM Sensor Data support  No data supported\n" );
							}
							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         PM Sensor Data support  Reserved bits set\n" );
							}

							/* PM Sensor Operating Status Bank 1 Sensor 1 */
							if ( pSid1[SidIndex].Data[0] & 0x01 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  PM11 Sensor status (BYTE B) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[1] );

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         PM11_Active = %s\n",
								     (pSid1[SidIndex].Data[1] & 0x01) ? "YES" : "NO" );

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         PM11_REGEN = %s\n",
								     (pSid1[SidIndex].Data[1] & 0x02) ? "YES" : "NO" );

								if ( pSid1[SidIndex].Data[1] & 0xFC )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "         PM11 Sensor Operating Status  Reserved bits set\n" );
								}
							}

							/* PM Sensor Normalized Output Bank 1 Sensor 1 */
							if ( pSid1[SidIndex].Data[0] & 0x02 )
							{
								temp_data_float = (float)((signed short)( (pSid1[SidIndex].Data[2] * 256) +
								                                          pSid1[SidIndex].Data[3] )) * (float)0.01;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  PM11 = %.2f %%\n", GetEcuId(EcuIndex), temp_data_float);

								if ( temp_data_float < -100 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  PM11 must be greater than -100%%\n", GetEcuId(EcuIndex), temp_data_float);
								}
							}

							/* PM Sensor Operating Status Bank 2 Sensor 1 */
							if ( pSid1[SidIndex].Data[0] & 0x04 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  PM21 Sensor status (BYTE E) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[4] );

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         PM21_Active = %s\n",
								     (pSid1[SidIndex].Data[4] & 0x01) ? "YES" : "NO" );

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         PM21_REGEN = %s\n",
								     (pSid1[SidIndex].Data[4] & 0x02) ? "YES" : "NO" );

								if ( pSid1[SidIndex].Data[4] & 0xFC )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "         PM21 Sensor Operating Status  Reserved bits set\n" );
								}
							}

							/* PM Sensor Normalized Output Bank 2 Sensor 1 */
							if ( pSid1[SidIndex].Data[0] & 0x08 )
							{
								temp_data_float = (float)((signed short)( (pSid1[SidIndex].Data[5] * 256) +
								                                          pSid1[SidIndex].Data[6] )) * (float)0.01;
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  PM21 = %.2f %%\n", GetEcuId(EcuIndex), temp_data_float);

								if ( temp_data_float < -100 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  PM21 must be greater than -100%%\n", GetEcuId(EcuIndex), temp_data_float);
								}
							}
						}
						break;

						case 0x90:
						{
							/* WWH-OBD Vehicle OBD System Information */
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  WWH-OBD Vehicle OBD System Information support (BYTE A) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "         MI_DISP_VOBD = $%02X\n",
							     pSid1[SidIndex].Data[0] & 0x03 );

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "         MI_MODE_VOBD = $%02X\n",
							     (pSid1[SidIndex].Data[0] & 0x3C)>>2 );

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "         VOBD_RDY = %s\n",
							     (pSid1[SidIndex].Data[0] & 0x40) ? "YES" : "NO" );

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0x80 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         WWH-OBD Vehicle OBD System Information  Reserved bits set\n" );
							}

							temp_data_long = (pSid1[SidIndex].Data[1] * 256) +
							                 pSid1[SidIndex].Data[2];
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  VOBD_MI_TIME = %d hrs\n", GetEcuId(EcuIndex), temp_data_long);
						}
						break;

						case 0x91:
						{
							/* WWH-OBD Vehicle OBD System Information */
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  WWH-OBD ECU OBD System Information support (BYTE A) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "         MI_MODE_ECU = $%02X\n",
							     pSid1[SidIndex].Data[0] & 0x0F );

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         WWH-OBD ECU OBD System Information support  Reserved bits set\n" );
							}

							temp_data_long = (pSid1[SidIndex].Data[1] * 256) +
							                 pSid1[SidIndex].Data[2];
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  OBD_MI_TIME = %d hrs\n", GetEcuId(EcuIndex), temp_data_long);

							temp_data_long = (pSid1[SidIndex].Data[3] * 256) +
							                 pSid1[SidIndex].Data[4];
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  OBD_B1_TIME = %d hrs\n", GetEcuId(EcuIndex), temp_data_long);
						}
						break;

						case 0x92:
						{
							/* Fuel System Control Status (Compression Ignition) */
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Fuel System Control Status support (BYTE A) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );

							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0xFF) == 0 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         Fuel System Control Status support  No data supported\n" );
							}

							/* Fuel System Status */
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  FUELSYS = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[1] );

							/* Fuel Pressure Control 1 Status */
							if ( pSid1[SidIndex].Data[0] & 0x01 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         FP1_CL  = %c\n",
								     (pSid1[SidIndex].Data[1] & 0x01) ? '1' : '0' );
							}

							/* Fuel Injection Quantity Control 1 Status */
							if ( pSid1[SidIndex].Data[0] & 0x02 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         FIQ1_CL = %c\n",
								     (pSid1[SidIndex].Data[1] & 0x02) ? '1' : '0' );
							}

							/* Fuel Injection Timing Control 1 Status */
							if ( pSid1[SidIndex].Data[0] & 0x04 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         FIT1_CL = %c\n",
								     (pSid1[SidIndex].Data[1] & 0x04) ? '1' : '0' );
							}

							/* Idle Fuel Balance/Contribution Control 1 Status */
							if ( pSid1[SidIndex].Data[0] & 0x08 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         IFB1_CL = %c\n",
								     (pSid1[SidIndex].Data[1] & 0x08) ? '1' : '0' );
							}

							/* Fuel Pressure Control 2 Status */
							if ( pSid1[SidIndex].Data[0] & 0x10 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         FP2_CL  = %c\n",
								     (pSid1[SidIndex].Data[1] & 0x10) ? '1' : '0' );
							}

							/* Fuel Injection Quantity Control 2 Status */
							if ( pSid1[SidIndex].Data[0] & 0x20 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         FIQ2_CL = %c\n",
								     (pSid1[SidIndex].Data[1] & 0x20) ? '1' : '0' );
							}

							/* Fuel Injection Timing Control 2 Status */
							if ( pSid1[SidIndex].Data[0] & 0x40 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         FIT2_CL = %c\n",
								     (pSid1[SidIndex].Data[1] & 0x40) ? '1' : '0' );
							}

							/* Idle Fuel Balance/Contribution Control 2 Status */
							if ( pSid1[SidIndex].Data[0] & 0x80 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         IFB2_CL = %c\n",
								     (pSid1[SidIndex].Data[1] & 0x80) ? '1' : '0' );
							}
						}
						break;

						case 0x93:
						{
							/* WWH-OBD Vehicle OBD Counters support */
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  WWH-OBD Vehicle OBD Counters support (BYTE A) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xFE )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         WWH-OBD Vehicle OBD Counters support  Reserved bits set\n" );
							}

							if ( pSid1[SidIndex].Data[0] & 0x01 )
							{
								temp_data_long = (pSid1[SidIndex].Data[1] * 256) +
								                 pSid1[SidIndex].Data[2];
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  MI_TIME_CUM = %d hrs\n", GetEcuId(EcuIndex), temp_data_long);
							}
						}
						break;

						case 0x94:
						{
							/* NOx Warning And Inducement System support */
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  NOx Warning And Inducement System support (BYTE A) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );

							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x3F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         NOx Warning And Inducement System support  No data supported\n" );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xC0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         NOx Warning And Inducement System support  Reserved bits set\n" );
							}

							/* NOx Warning And Inducement System support */
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  NOx Warning And Inducement System status (BYTE B) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );

							if ( pSid1[SidIndex].Data[0] & 0x01 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         NOX_WARN_ACT = %s\n",
								     (pSid1[SidIndex].Data[1] & 0x01) ? "YES" : "NO" );

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         INDUC_L1 = $%02X\n",
								     (pSid1[SidIndex].Data[1] & 0x06)>>1 );

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         INDUC_L2 = $%02X\n",
								     (pSid1[SidIndex].Data[1] & 0x18)>>3 );

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         INDUC_L3 = $%02X\n",
								     (pSid1[SidIndex].Data[1] & 0x60)>>5 );

								if ( pSid1[SidIndex].Data[0] & 0x80 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "         NOx Warning And Inducement Ststem status  Reserved bits set\n" );
								}
							}

							temp_data_long = (pSid1[SidIndex].Data[2] * 256) +
							                 pSid1[SidIndex].Data[3];
							if ( pSid1[SidIndex].Data[0] & 0x02 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  REAG_QUAL_TIME = %d hrs\n", GetEcuId(EcuIndex), temp_data_long);
							}
							else if ( temp_data_long != 65535 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  REAG_QUAL_TIME must be 65535 if it is not supported\n", GetEcuId(EcuIndex) );
							}

							temp_data_long = (pSid1[SidIndex].Data[4] * 256) +
							                 pSid1[SidIndex].Data[5];
							if ( pSid1[SidIndex].Data[0] & 0x04 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  REAG_CON_TIME = %d hrs\n", GetEcuId(EcuIndex), temp_data_long);
							}
							else if ( temp_data_long != 65535 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  REAG_CON_TIME must be 65535 if it is not supported\n", GetEcuId(EcuIndex) );
							}

							temp_data_long = (pSid1[SidIndex].Data[6] * 256) +
							                 pSid1[SidIndex].Data[7];
							if ( pSid1[SidIndex].Data[0] & 0x08 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  REAG_DOSE_TIME = %d hrs\n", GetEcuId(EcuIndex), temp_data_long);
							}
							else if ( temp_data_long != 65535 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  REAG_DOSE_TIME must be 65535 if it is not supported\n", GetEcuId(EcuIndex) );
							}

							temp_data_long = (pSid1[SidIndex].Data[8] * 256) +
							                 pSid1[SidIndex].Data[9];
							if ( pSid1[SidIndex].Data[0] & 0x10 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGR_TIME = %d hrs\n", GetEcuId(EcuIndex), temp_data_long);
							}
							else if ( temp_data_long != 65535 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGR_TIME must be 65535 if it is not supported\n", GetEcuId(EcuIndex) );
							}

							temp_data_long = (pSid1[SidIndex].Data[10] * 256) +
							                 pSid1[SidIndex].Data[11];
							if ( pSid1[SidIndex].Data[0] & 0x20 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  NOX_DTC_TIME = %d hrs\n", GetEcuId(EcuIndex), temp_data_long);
							}
							else if ( temp_data_long != 65535 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  NOX_DTC_TIME must be 65535 if it is not supported\n", GetEcuId(EcuIndex) );
							}
						}
						break;

						case 0x98:
						{
							/* Exhaust Gas Temperature Sensor support */
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Exhaust Gas Temperature Sensor Bank 1 support (BYTE A) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );

							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x0F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         Exhaust Gas Temperature Sensor Bank 1 support  No data supported\n" );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         Exhaust Gas Temperature Sensor Bank 1 support  Reserved bits set\n" );
							}

							if ( pSid1[SidIndex].Data[0] & 0x01 )
							{
								temp_data_float = ((float)((pSid1[SidIndex].Data[1] * 256) +
								                            pSid1[SidIndex].Data[2]) * (float).1) - (float)40;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGT15 = %.1f C\n", GetEcuId(EcuIndex),
								     temp_data_float );
							}

							if ( pSid1[SidIndex].Data[0] & 0x02 )
							{
								temp_data_float = ((float)((pSid1[SidIndex].Data[3] * 256) +
								                            pSid1[SidIndex].Data[4]) * (float).1) - (float)40;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGT16 = %.1f C\n", GetEcuId(EcuIndex),
								     temp_data_float );
							}

							if ( pSid1[SidIndex].Data[0] & 0x04 )
							{
								temp_data_float = ((float)((pSid1[SidIndex].Data[5] * 256) +
								                            pSid1[SidIndex].Data[6]) * (float).1) - (float)40;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGT17 = %.1f C\n", GetEcuId(EcuIndex),
								     temp_data_float );
							}

							if ( pSid1[SidIndex].Data[0] & 0x08 )
							{
								temp_data_float = ((float)((pSid1[SidIndex].Data[7] * 256) +
								                            pSid1[SidIndex].Data[8]) * (float).1) - (float)40;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGT18 = %.1f C\n", GetEcuId(EcuIndex),
								     temp_data_float );
							}
						}
						break;

						case 0x99:
						{
							/* Exhaust Gas Temperature Sensor support */
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Exhaust Gas Temperature Sensor Bank 2 support (BYTE A) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );

							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x0F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         Exhaust Gas Temperature Sensor Bank 2 support  No data supported\n" );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         Exhaust Gas Temperature Sensor Bank 2 support  Reserved bits set\n" );
							}

							if ( pSid1[SidIndex].Data[0] & 0x01 )
							{
								temp_data_float = ((float)((pSid1[SidIndex].Data[1] * 256) +
								                            pSid1[SidIndex].Data[2]) * (float).1) - (float)40;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGT25 = %.1f C\n", GetEcuId(EcuIndex),
								     temp_data_float );
							}

							if ( pSid1[SidIndex].Data[0] & 0x02 )
							{
								temp_data_float = ((float)((pSid1[SidIndex].Data[3] * 256) +
								                            pSid1[SidIndex].Data[4]) * (float).1) - (float)40;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGT26 = %.1f C\n", GetEcuId(EcuIndex),
								     temp_data_float );
							}

							if ( pSid1[SidIndex].Data[0] & 0x04 )
							{
								temp_data_float = ((float)((pSid1[SidIndex].Data[5] * 256) +
								                            pSid1[SidIndex].Data[6]) * (float).1) - (float)40;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGT27 = %.1f C\n", GetEcuId(EcuIndex),
								     temp_data_float );
							}

							if ( pSid1[SidIndex].Data[0] & 0x08 )
							{
								temp_data_float = ((float)((pSid1[SidIndex].Data[7] * 256) +
								                            pSid1[SidIndex].Data[8]) * (float).1) - (float)40;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EGT28 = %.1f C\n", GetEcuId(EcuIndex),
								     temp_data_float );
							}
						}
						break;

						case 0x9A:
						{
							/* Hybrid/EV Vehicle System Data support */
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Hybrid/EV Vehicle System Data support (BYTE A) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         Hybrid/EV Vehicle System Data support  Reserved bits set\n" );
							}

							/* Hybrid/EV Vehicle state */
							if ( pSid1[SidIndex].Data[0] & 0x01 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  HEV_MODE = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[1] & 0x01 );

								if ( pSid1[SidIndex].Data[1] & 0xF8 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "         Hybrid/EV Vehicle Charging state  Reserved bits set\n" );
								}
							}

							/* Hybrid/EV Battery System Voltage */
							if ( pSid1[SidIndex].Data[0] & 0x02 )
							{
								temp_data_float = (float)((unsigned short)( (pSid1[SidIndex].Data[2] * 256) +
								                                   pSid1[SidIndex].Data[3] )) * (float)0.015625;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  HEV_BATT_V = %.6f V\n", GetEcuId(EcuIndex), temp_data_float);

								if ( temp_data_float <= 0 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  HEV_BATT_V must be greater than 0 V\n", GetEcuId(EcuIndex), temp_data_float );
								}
							}

							/* Hybrid/EV Battery System Current */
							if ( pSid1[SidIndex].Data[0] & 0x04 )
							{
								temp_data_float = (float)((signed short)( (pSid1[SidIndex].Data[4] * 256) +
								                                          pSid1[SidIndex].Data[5] )) * (float)0.1;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  HEV_BATT_A = %.2f A\n", GetEcuId(EcuIndex), temp_data_float );
							}
						}
						break;

						case 0x9B:
						{
							/* Diesel Exhaust Fluid Sensor Data support */
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Diesel Exhaust Fluid Sensor Data support (BYTE A bits 0-3) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );

							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x07) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         Diesel Exhaust Fluid Sensor Data support  No data supported\n" );
							}

							/* Diesel Exhaust Fluid Type */
							if ( pSid1[SidIndex].Data[0] & 0x01 )
							{
								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DEF_Type (BYTE A bits 4-7) = $%02X\n", GetEcuId(EcuIndex), (pSid1[SidIndex].Data[0] >> 4) );
							}

							/* Diesel Exhaust Fluid Concentration */
							if ( pSid1[SidIndex].Data[0] & 0x02 )
							{
								temp_data_float = (float)((unsigned short)(pSid1[SidIndex].Data[1])) * (float)0.25;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DEF_CON = %.2f %%\n", GetEcuId(EcuIndex), temp_data_float );
							}

							/* Diesel Exhaust Fluid Tank Temperature */
							if ( pSid1[SidIndex].Data[0] & 0x04 )
							{
								temp_data_float = (float)((unsigned short)(pSid1[SidIndex].Data[2])) - (float)40.00;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DEF_T = %f C\n", GetEcuId(EcuIndex), temp_data_float);
							}

							/* Diesel Exhaust Fluid Tank Level */
							if ( pSid1[SidIndex].Data[0] & 0x08 )
							{
								temp_data_float = (float)((unsigned short)(pSid1[SidIndex].Data[3])) * (float)(100.0/255.0);

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DEF_LVL = %.2f %%\n", GetEcuId(EcuIndex), temp_data_float );
							}
						}
						break;

						case 0x9C:
						{
							/* Exhaust Gas Temperature Sensor support */
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  O2 Sensor Data support (BYTE A) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );

							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0xFF) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         O2 Sensor Data support  No data supported\n" );
							}

							if ( pSid1[SidIndex].Data[0] & 0x01 )
							{
								temp_data_float = (float)((pSid1[SidIndex].Data[1] * 256) +
								                            pSid1[SidIndex].Data[2]) * (float).001526;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  O2S13_PCT = %.6f %%\n", GetEcuId(EcuIndex),
								     temp_data_float );
							}

							if ( pSid1[SidIndex].Data[0] & 0x02 )
							{
								temp_data_float = (float)((pSid1[SidIndex].Data[3] * 256) +
								                            pSid1[SidIndex].Data[4]) * (float).001526;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  O2S14_PCT = %.6f %%\n", GetEcuId(EcuIndex),
								     temp_data_float );
							}

							if ( pSid1[SidIndex].Data[0] & 0x04 )
							{
								temp_data_float = (float)((pSid1[SidIndex].Data[5] * 256) +
								                            pSid1[SidIndex].Data[6]) * (float).001526;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  O2S23_PCT = %.6f %%\n", GetEcuId(EcuIndex),
								     temp_data_float );
							}

							if ( pSid1[SidIndex].Data[0] & 0x08 )
							{
								temp_data_float = (float)((pSid1[SidIndex].Data[7] * 256) +
								                            pSid1[SidIndex].Data[8]) * (float).001526;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  O2S24_PCT = %.6f %%\n", GetEcuId(EcuIndex),
								     temp_data_float );
							}

							if ( pSid1[SidIndex].Data[0] & 0x10 )
							{
								temp_data_float = (float)((pSid1[SidIndex].Data[9] * 256) +
								                            pSid1[SidIndex].Data[10]) * (float).000122;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  LAMBDA13 = %.6f\n", GetEcuId(EcuIndex),
								     temp_data_float );
							}

							if ( pSid1[SidIndex].Data[0] & 0x20 )
							{
								temp_data_float = (float)((pSid1[SidIndex].Data[11] * 256) +
								                            pSid1[SidIndex].Data[12]) * (float).000122;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  LAMBDA14 = %.6f\n", GetEcuId(EcuIndex),
								     temp_data_float );
							}

							if ( pSid1[SidIndex].Data[0] & 0x40 )
							{
								temp_data_float = (float)((pSid1[SidIndex].Data[13] * 256) +
								                            pSid1[SidIndex].Data[14]) * (float).000122;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  LAMBDA23 = %.6f\n", GetEcuId(EcuIndex),
								     temp_data_float );
							}

							if ( pSid1[SidIndex].Data[0] & 0x80 )
							{
								temp_data_float = (float)((pSid1[SidIndex].Data[15] * 256) +
								                            pSid1[SidIndex].Data[16]) * (float).000122;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  LAMBDA24 = %.6f\n", GetEcuId(EcuIndex),
								     temp_data_float );
							}
						}
						break;

						case 0x9D:
						{
							/* Engine Fuel Rate */
							temp_data_float = (float)((unsigned short)((pSid1[SidIndex].Data[0] * 256) +
							                            pSid1[SidIndex].Data[1])) * (float).02;

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  ENG_FUEL_RATE = %.2f g/s\n", GetEcuId(EcuIndex), temp_data_float );

							if ( gOBDEngineRunning == FALSE && temp_data_float != 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         ENG_FUEL_RATE must be 0 g/s when engine is not running\n" );
							}
							else if ( gOBDEngineRunning == TRUE && temp_data_float == 0 && gOBDHybridFlag != TRUE )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         ENG_FUEL_RATE must be greater than 0 g/s when engine is running\n" );
							}

							temp_data_float = (float)((unsigned short)((pSid1[SidIndex].Data[2] * 256) +
							                            pSid1[SidIndex].Data[3])) * (float).02;

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  VEH_FUEL_RATE = %.2f g/s\n", GetEcuId(EcuIndex), temp_data_float );

							if ( gOBDEngineRunning == FALSE && temp_data_float != 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         VEH_FUEL_RATE must be 0 g/s when engine is not running\n" );
							}
							else if ( gOBDEngineRunning == TRUE && temp_data_float == 0 && gOBDHybridFlag != TRUE )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         VEH_FUEL_RATE must be greater than 0 g/s when engine is running\n" );
							}
						}
						break;

						case 0x9E:
						{
							/* Engine Exhaust Flow Rate */
							temp_data_float = (float)((unsigned short)((pSid1[SidIndex].Data[0] * 256) +
							                            pSid1[SidIndex].Data[1])) * (float).2;

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  EXH_RATE = %.2f kg/h\n", GetEcuId(EcuIndex), temp_data_float );

							if ( gOBDEngineRunning == FALSE && temp_data_float != 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         EXH_RATE must be 0 kg/h when engine is not running\n" );
							}
							else if ( gOBDEngineRunning == TRUE && temp_data_float == 0 && gOBDHybridFlag != TRUE )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         EXH_RATE must be greater than 0 kg/h when engine is running\n" );
							}
						}
						break;

						case 0x9F:
						{
							/* Fuel System Percentage Use support */
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Fuel System Percentage Use support (BYTE A) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );

							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x03) == 0 &&  // bits 0,1  Fuel System A,B uses Bank 1
							     (pSid1[SidIndex].Data[0] & 0x0C) == 0 &&  // bits 2,3  Fuel System A,B uses Bank 2
							     (pSid1[SidIndex].Data[0] & 0x30) == 0 &&  // bits 4,5  Fuel System A,B uses Bank 3
							     (pSid1[SidIndex].Data[0] & 0xC0) == 0 )   // bits 6,7  Fuel System A,B uses Bank 4
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         Fuel System Percentage Use support  (At least one bank must be supported)\n" );
							}

							if ( pSid1[SidIndex].Data[0] & 0x01 )
							{
								temp_data_float = (float)((unsigned short)(pSid1[SidIndex].Data[1]) * (float)(100.0/255.0));

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FUELSYSA_B1 = %.6f %%\n", GetEcuId(EcuIndex), temp_data_float );

								if ( gOBDEngineRunning == FALSE && temp_data_float != 0 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "         FUELSYSA_B1 must be 0 %% when engine is not running\n" );
								}
								else if ( (gOBDEngineRunning == TRUE && temp_data_float != 0) || gOBDHybridFlag == TRUE )
								{
									fPid9FSuccess = TRUE;
								}
							}

							if ( pSid1[SidIndex].Data[0] & 0x02 )
							{
								temp_data_float = (float)((unsigned short)(pSid1[SidIndex].Data[2])) * (float)(100.0/255.0);

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FUELSYSB_B1 = %.6f %%\n", GetEcuId(EcuIndex), temp_data_float );

								if ( gOBDEngineRunning == FALSE && temp_data_float != 0 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "         FUELSYSB_B1 must be 0 %% when engine is not running\n" );
								}
								else if ( (gOBDEngineRunning == TRUE && temp_data_float != 0) || gOBDHybridFlag == TRUE )
								{
									fPid9FSuccess = TRUE;
								}
							}

							if ( pSid1[SidIndex].Data[0] & 0x04 )
							{
								temp_data_float = (float)((unsigned short)(pSid1[SidIndex].Data[3])) * (float)(100.0/255.0);

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FUELSYSA_B2 = %.6f %%\n", GetEcuId(EcuIndex), temp_data_float );

								if ( gOBDEngineRunning == FALSE && temp_data_float != 0 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "         FUELSYSA_B2 must be 0 %% when engine is not running\n" );
								}
								else if ( (gOBDEngineRunning == TRUE && temp_data_float != 0) || gOBDHybridFlag == TRUE )
								{
									fPid9FSuccess = TRUE;
								}
							}

							if ( pSid1[SidIndex].Data[0] & 0x08 )
							{
								temp_data_float = (float)((unsigned short)(pSid1[SidIndex].Data[4])) * (float)(100.0/255.0);

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FUELSYSB_B2 = %.6f %%\n", GetEcuId(EcuIndex), temp_data_float );

								if ( gOBDEngineRunning == FALSE && temp_data_float != 0 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "         FUELSYSB_B2 must be 0 %% when engine is not running\n" );
								}
								else if ( (gOBDEngineRunning == TRUE && temp_data_float != 0) || gOBDHybridFlag == TRUE )
								{
									fPid9FSuccess = TRUE;
								}
							}

							if ( pSid1[SidIndex].Data[0] & 0x10 )
							{
								temp_data_float = (float)((unsigned short)(pSid1[SidIndex].Data[5])) * (float)(100.0/255.0);

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FUELSYSA_B3 = %.6f %%\n", GetEcuId(EcuIndex), temp_data_float );

								if ( gOBDEngineRunning == FALSE && temp_data_float != 0 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "         FUELSYSA_B3 must be 0 %% when engine is not running\n" );
								}
								else if ( (gOBDEngineRunning == TRUE && temp_data_float != 0) || gOBDHybridFlag == TRUE )
								{
									fPid9FSuccess = TRUE;
								}
							}

							if ( pSid1[SidIndex].Data[0] & 0x20 )
							{
								temp_data_float = (float)((unsigned short)(pSid1[SidIndex].Data[6])) * (float)(100.0/255.0);

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FUELSYSB_B3 = %.6f %%\n", GetEcuId(EcuIndex), temp_data_float );

								if ( gOBDEngineRunning == FALSE && temp_data_float != 0 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "         FUELSYSB_B3 must be 0 %% when engine is not running\n" );
								}
								else if ( (gOBDEngineRunning == TRUE && temp_data_float != 0) || gOBDHybridFlag == TRUE )
								{
									fPid9FSuccess = TRUE;
								}
							}

							if ( pSid1[SidIndex].Data[0] & 0x40 )
							{
								temp_data_float = (float)((unsigned short)(pSid1[SidIndex].Data[7])) * (float)(100.0/255.0);

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FUELSYSA_B4 = %.6f %%\n", GetEcuId(EcuIndex), temp_data_float );

								if ( gOBDEngineRunning == FALSE && temp_data_float != 0 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "         FUELSYSA_B4 must be 0 %% when engine is not running\n" );
								}
								else if ( (gOBDEngineRunning == TRUE && temp_data_float != 0) || gOBDHybridFlag == TRUE )
								{
									fPid9FSuccess = TRUE;
								}
							}

							if ( pSid1[SidIndex].Data[0] & 0x08 )
							{
								temp_data_float = (float)((unsigned short)(pSid1[SidIndex].Data[8])) * (float)(100.0/255.0);

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  FUELSYSB_B4 = %.6f %%\n", GetEcuId(EcuIndex), temp_data_float );

								if ( gOBDEngineRunning == FALSE && temp_data_float != 0 )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "         FUELSYSB_B4 must be 0 %% when engine is not running\n" );
								}
								else if ( (gOBDEngineRunning == TRUE && temp_data_float != 0) || gOBDHybridFlag == TRUE )
								{
									fPid9FSuccess = TRUE;
								}
							}

							if ( fPid9FSuccess == FALSE && gOBDEngineRunning == TRUE)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         At least one of the FUELSYS banks must be greater than 0 %% when engine is running\n" );
							}

						}
						break;

						case 0xA1:
						{
							/* NOx Sensor Corrected Data support */
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  NOx Sensor Corrected Data support (BYTE A) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );

							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x0F) == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         NOx Sensor Corrected Data  No data supported\n" );
							}


							if ( pSid1[SidIndex].Data[0] & 0x01 )
							{
								temp_data_float = (float)((unsigned short)((pSid1[SidIndex].Data[1] * 256) +
								                           pSid1[SidIndex].Data[2]));

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  NOXC11 = %f ppm\n", GetEcuId(EcuIndex), temp_data_float );

								if ( (pSid1[SidIndex].Data[0] & 0x10) == 0x10 && temp_data_float != 0xFFFF )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "         NOXC11 Data Availability Bit Set, Sensor Data should report $FFFF\n" );
								}
							}

							if ( pSid1[SidIndex].Data[0] & 0x02 )
							{
								temp_data_float = (float)((unsigned short)((pSid1[SidIndex].Data[3] * 256) +
								                           pSid1[SidIndex].Data[4]));

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  NOXC12 = %f ppm\n", GetEcuId(EcuIndex), temp_data_float );

								if ( (pSid1[SidIndex].Data[0] & 0x20) == 0x10 && temp_data_float != 0xFFFF )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "         NOXC12 Data Availability Bit Set, Sensor Data should report $FFFF\n" );
								}
							}

							if ( pSid1[SidIndex].Data[0] & 0x04 )
							{
								temp_data_float = (float)((unsigned short)((pSid1[SidIndex].Data[5] * 256) +
								                           pSid1[SidIndex].Data[6]));

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  NOXC21 = %f ppm\n", GetEcuId(EcuIndex), temp_data_float );

								if ( (pSid1[SidIndex].Data[0] & 0x40) == 0x10 && temp_data_float != 0xFFFF )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "         NOXC21 Data Availability Bit Set, Sensor Data should report $FFFF\n" );
								}
							}

							if ( pSid1[SidIndex].Data[0] & 0x08 )
							{
								temp_data_float = (float)((unsigned short)((pSid1[SidIndex].Data[7] * 256) +
								                           pSid1[SidIndex].Data[8]));

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  NOXC22 = %f ppm\n", GetEcuId(EcuIndex), temp_data_float );
							}

								if ( (pSid1[SidIndex].Data[0] & 0x80) == 0x10 && temp_data_float != 0xFFFF )
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "         NOXC22 Data Availability Bit Set, Sensor Data should report $FFFF\n" );
								}
						}
						break;

						case 0xA2:
						{
							/* Cylinder Fuel Rate */
								temp_data_float = (float)((unsigned short)((pSid1[SidIndex].Data[0] * 256) +
								                           pSid1[SidIndex].Data[1])) * (float)0.03125;

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  CYL_RATE = %.6f mg/stroke\n", GetEcuId(EcuIndex), temp_data_float );

							if ( gOBDEngineRunning == FALSE && temp_data_float != 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         CYL_RATE must be 0 mg/stroke when engine is not running\n" );
							}
							else if ( gOBDEngineRunning == TRUE && temp_data_float == 0 && gOBDHybridFlag != TRUE )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "         CYL_RATE must be greater than 0 mg/stroke when engine is running\n" );
							}
						}
						break;

						case 0xA3:
						{
							/* Evap System Vapor Pressure */
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Evaporative System Vapor Pressure support (BYTE A) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "        Pressure A Supported = %s\n",
							     (pSid1[SidIndex].Data[0] & 0x01) ? "YES" : "NO" );

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "        Pressure A (Wide Range) Supported = %s\n",
							     (pSid1[SidIndex].Data[0] & 0x02) ? "YES" : "NO" );

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "        Pressure B Supported = %s\n",
							     (pSid1[SidIndex].Data[0] & 0x04) ? "YES" : "NO" );

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "        Pressure B (Wide Range) Supported = %s\n",
							     (pSid1[SidIndex].Data[0] & 0x08) ? "YES" : "NO" );

							/* check that required bit is set */
							if ( (pSid1[SidIndex].Data[0] & 0x03) == 0x03 || (pSid1[SidIndex].Data[0] & 0x0C) == 0x0C)
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "           Evaporative System Vapor Pressure support  Two different scaling ranges supported for the same sensor\n" );
							}

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xF0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "            Evaporative System Vapor Pressure support  Reserved bits set\n" );
							}


							if ( pSid1[SidIndex].Data[0] & 0x01 )
							{
								temp_data_float = (float)((signed short)((pSid1[SidIndex].Data[1] * 256) +
								                           pSid1[SidIndex].Data[2])) * (float)0.25;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EVAP_A_VP = %f Pa\n", GetEcuId(EcuIndex), temp_data_float );
							}

							if ( pSid1[SidIndex].Data[0] & 0x02 )
							{
								temp_data_float = (float)((signed short)((pSid1[SidIndex].Data[3] * 256) +
								                           pSid1[SidIndex].Data[4])) * (float)2;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EVAP_A_VP (Wide Range) = %f Pa\n", GetEcuId(EcuIndex), temp_data_float );
							}

							if ( pSid1[SidIndex].Data[0] & 0x04 )
							{
								temp_data_float = (float)((unsigned short)((pSid1[SidIndex].Data[5] * 256) +
								                           pSid1[SidIndex].Data[6])) * (float)0.25;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EVAP_B_VP = %f Pa\n", GetEcuId(EcuIndex), temp_data_float );
							}

							if ( pSid1[SidIndex].Data[0] & 0x08 )
							{
								temp_data_float = (float)((unsigned short)((pSid1[SidIndex].Data[7] * 256) +
								                           pSid1[SidIndex].Data[8])) * (float)2;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  EVAP_B_VP (Wide Range) = %f Pa\n", GetEcuId(EcuIndex), temp_data_float );
							}
						}
						break;

						case 0xA4:
						{
							/* Transmission Actual Gear */
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Transmission Actual Gear Status support (BYTE A) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "        Actual Transmission Gear Supported = %s\n",
							     (pSid1[SidIndex].Data[0] & 0x01) ? "YES" : "NO" );

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "        Actual Transmission Gear Ratio Supported = %s\n",
							     (pSid1[SidIndex].Data[0] & 0x02) ? "YES" : "NO" );

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xFC )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "           Transmission Actual Gear Status support  Reserved bits set\n" );
							}


							if ( pSid1[SidIndex].Data[0] & 0x01 )
							{
								temp_data_float = (float)((unsigned short)((pSid1[SidIndex].Data[1])>>4));

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  GEAR_ACT = %f\n", GetEcuId(EcuIndex), temp_data_float );
							}

							if ( pSid1[SidIndex].Data[0] & 0x02 )
							{
								temp_data_float = (float)((unsigned short)((pSid1[SidIndex].Data[2] * 256) +
								                           pSid1[SidIndex].Data[3])) * (float)0.001;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  GEAR_RAT = %f\n", GetEcuId(EcuIndex), temp_data_float );
							}
						}
						break;

						case 0xA5:
						{
							/* Diesel Exhaust Fluid Dosing */
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Commanded DEF Dosing support (BYTE A) = $%02X\n", GetEcuId(EcuIndex), pSid1[SidIndex].Data[0] );

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "        DEF_CMD Supported = %s\n",
							     (pSid1[SidIndex].Data[0] & 0x01) ? "YES" : "NO" );

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "        DEF_UCDC Supported = %s\n",
							     (pSid1[SidIndex].Data[0] & 0x02) ? "YES" : "NO" );

							/* check that reserved bits are not set */
							if ( pSid1[SidIndex].Data[0] & 0xFC )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "            Commanded DEF Dosing support  Reserved bits set\n" );
							}


							if ( pSid1[SidIndex].Data[0] & 0x01 )
							{
								temp_data_float = (float)((unsigned short)(pSid1[SidIndex].Data[1])) * (float)0.5;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DEF_CMD = %f %\n", GetEcuId(EcuIndex), temp_data_float );
							}

							if ( pSid1[SidIndex].Data[0] & 0x02 )
							{
								temp_data_float = (float)((unsigned short)((pSid1[SidIndex].Data[2] * 256) +
								                           pSid1[SidIndex].Data[3])) * (float)0.0005;

								Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  DEF_UCDC = %f L\n", GetEcuId(EcuIndex), temp_data_float );
							}
						}
						break;

						case 0xA6:
						{
							temp_data_float = (float)((unsigned short)( pSid1[SidIndex].Data[0] * 16777216 ) +
							                                          ( pSid1[SidIndex].Data[1] * 65536 ) +
							                                          ( pSid1[SidIndex].Data[2] * 256 ) +
							                                          pSid1[SidIndex].Data[3]) * (float)0.1;

							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  ODO = %f km (%l miles)\n", GetEcuId(EcuIndex), temp_data_float, temp_data_float*0.621371 );
						}
						break;


						default:
						{
							/* Non-OBD PID */
						}
						break;

					}  // end switch(pSid1[SidIndex].PID)

				}  // end else ... datasize != 0

			}  // end if (IsSid1PidSupported (EcuIndex, IdIndex) == TRUE)

		}  // end for (EcuIndex

		// If there where any errors in the data, fail
		if ( ulTemp_FailureCount != GetFailureCount() )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Invalid SID $1 PID $%02X Data\n", IdIndex);
		}

	}  // end for (IdIndex



	if ( OBDEngineDontCare == FALSE)  // NOT Test 10.2
	{
		/**************************************************************************
		* "Either Or" required PIDs
		**************************************************************************/
		/* All vehicles require Pid $05 or $67 */
		if ( fPidSupported[0x05] == FALSE &&
		     fPidSupported[0x67] == FALSE )
		{
			fReqPidNotSupported = TRUE;
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Neither SID $1 PID $05 nor $67 supported (Support of one required for all vehicles)\n");
		}

		/* Gasoline vehicles require Pid $0B, $87, $10 or $66 */
		if ( gOBDDieselFlag == FALSE &&
		     ( fPidSupported[0x0B] == FALSE &&
		       fPidSupported[0x87] == FALSE &&
		       fPidSupported[0x10] == FALSE &&
		       fPidSupported[0x66] == FALSE ) )
		{
			fReqPidNotSupported = TRUE;
			Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $1 PID $0B, $87, $10 nor $66 supported (Support of one required for gasoline vehicles)\n");
		}

		/* Gasoline vehicles require Pid $0F or $68 */
		if ( gOBDDieselFlag == FALSE &&
		     ( fPidSupported[0x0F] == FALSE &&
		       fPidSupported[0x68] == FALSE ) )
		{
			fReqPidNotSupported = TRUE;
			Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Neither SID $1 PID $0F nor $68 supported (Support of one required for gasoline vehicles)\n");
		}

		/* Gasoline vehicles require Pid $13 or $1D */
		if ( gOBDDieselFlag == FALSE &&
		     ( fPidSupported[0x13] == FALSE &&
		       fPidSupported[0x1D] == FALSE ) )
		{
			fReqPidNotSupported = TRUE;
			Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Neither SID $1 PID $13 nor $1D supported (Support of one required for gasoline vehicles)\n");
		}

		/* ISO15765 vehicles require Pid $21 or $4D */
		if ( gOBDList[gOBDListIndex].Protocol == ISO15765 &&
		     ( fPidSupported[0x21] == FALSE &&
		       fPidSupported[0x4D] == FALSE ) )
		{
			fReqPidNotSupported = TRUE;
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Neither SID $1 PID $21 nor PID $4D supported (Support of one required for ISO15765 vehicles)\n", IdIndex );
		}

		/* MY 2010 diesel vehicles require Pid $2C or $69 */
		if ( gModelYear >= 2010 &&
		     gOBDDieselFlag == TRUE &&
		     ( gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD ) &&
		     ( fPidSupported[0x2C] == FALSE &&
		       fPidSupported[0x69] == FALSE ) )
		{
			fReqPidNotSupported = TRUE;
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Neither SID $1 PID $2C nor PID $69 supported (Support of one required for MY 2010 and beyond diesel vehicles)\n", IdIndex );
		}

		/* ISO15765 vehicles require Pid $31 or $4E */
		if ( gOBDList[gOBDListIndex].Protocol == ISO15765 &&
		     ( fPidSupported[0x31] == FALSE &&
		       fPidSupported[0x4E] == FALSE ) )
		{
			fReqPidNotSupported = TRUE;
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Neither SID $1 PID $31 nor PID $4E supported (Support of one required for ISO15765 vehicles)\n", IdIndex );
		}

		/* Gasoline vehicles require Pid $45 or $6C */
		if ( gOBDList[gOBDListIndex].Protocol == ISO15765 &&
		     gOBDDieselFlag == FALSE &&
		     ( fPidSupported[0x45] == FALSE &&
		       fPidSupported[0x6C] == FALSE ) )
		{
			if ( gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD )
			{
				fReqPidNotSupported = TRUE;
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "Neither SID $1 PID $45 nor PID $6C supported (Support of one required for ISO15765 gasoline vehicles)\n", IdIndex );
			}
			else
			{
				Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "Neither SID $1 PID $45 nor PID $6C supported (Support of one required for ISO15765 gasoline vehicles)\n", IdIndex );
			}
		}

		/**************************************************************************
		* Individually required PIDs
		**************************************************************************/
		if ( fPidSupported[0x01] == FALSE )
		{
			/* Must be supported for all vehicles */
			fReqPidNotSupported = TRUE;
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $1 PID $01 not supported (Support required for all vehicles)\n" );
		}

		else
		{
			/* check the 'required by at least one ECU' bits */
			if ( (ulIMBitsForVehicle & DATA_B_BIT_0) != DATA_B_BIT_0 &&
			     ( (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) ||
			       ( (gUserInput.eComplianceType != US_OBDII && gUserInput.eComplianceType != HD_OBD) &&
			         (gOBDDieselFlag == FALSE) ) ) )
			{
				/* Must be supported for all US_OBDII and EOBD gas vehicles */
				fReqPidNotSupported = TRUE;
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "SID $1 PID $01 DATA_B Bit 0 not supported (Support required for all vehicles)\n" );
			}
			if ((ulIMBitsForVehicle & DATA_B_BIT_1) != DATA_B_BIT_1)
			{
				/* Must be supported for all vehicles */
				fReqPidNotSupported = TRUE;
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "SID $1 PID $01 DATA_B Bit 1 not supported (Support required for all vehicles)\n" );
			}
			if (gOBDDieselFlag == FALSE)
			{
				if ((ulIMBitsForVehicle & DATA_C_BIT_0) != DATA_C_BIT_0)
				{
					/* Must be supported for all spark ignition vehicles */
					fReqPidNotSupported = TRUE;
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "SID $1 PID $01 DATA_C Bit 0 not supported (Support required for all vehicles)\n" );
				}
				if ( (ulIMBitsForVehicle & DATA_C_BIT_2) != DATA_C_BIT_2 &&
				     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
				{
					/* Must be supported for all US_OBDII spark ignition vehicles */
					fReqPidNotSupported = TRUE;
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "SID $1 PID $01 DATA_C Bit 2 not supported (Support required for all vehicles)\n" );
				}
			}
			else
			{
				if ( (ulIMBitsForVehicle & DATA_C_BIT_3) != DATA_C_BIT_3 &&
				     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
				{
					/* Must be supported for all US_OBDII compression ignition vehicles */
					fReqPidNotSupported = TRUE;
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "SID $1 PID $01 DATA_C Bit 3 not supported (Support required for all vehicles)\n" );
				}
				if ( (ulIMBitsForVehicle & DATA_C_BIT_7) != DATA_C_BIT_7 &&
				     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
				{
					/* Must be supported for all US_OBDII compression ignition vehicles */
					fReqPidNotSupported = TRUE;
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "SID $1 PID $01 DATA_C Bit 7 not supported (Support required for all vehicles)\n" );
				}
			}
			if ( (ulIMBitsForVehicle & DATA_C_BIT_5) != DATA_C_BIT_5 &&
			     ( (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) ||
			       ( (gUserInput.eComplianceType != US_OBDII && gUserInput.eComplianceType != HD_OBD) &&
			         (gOBDDieselFlag == FALSE) ) )
			   )
			{
				/* Must be supported for all US_OBDII and EOBD gas vehicles */
				fReqPidNotSupported = TRUE;
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "SID $1 PID $01 DATA_C Bit 5 not supported (Support required)\n" );
			}
			if ((ulIMBitsForVehicle & DATA_C_BIT_6) != DATA_C_BIT_6)
			{
				/* Must be supported for all vehicles */
				fReqPidNotSupported = TRUE;
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "SID $1 PID $01 DATA_C Bit 6 not supported (Support required for all vehicles)\n" );
			}
		}

		for (IdIndex = 0x02; IdIndex < MAX_PIDS; IdIndex++ )
		{
			if ( fPidSupported[IdIndex] == FALSE )
			{
				switch ( IdIndex )
				{
					case 0x04:
					case 0x0C:
					case 0x1C:
					{
						/* Must be supported for all vehicles */
						fReqPidNotSupported = TRUE;
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "SID $1 PID $%02X not supported (Support required for all vehicles)\n", IdIndex );
					}
					break;

					case 0x06:
					case 0x07:
					{
						/* Warn if not supported for gasoline vehicles */
						if ( gOBDDieselFlag == FALSE && gUserInput.eComplianceType == US_OBDII != OBDBr_NO_IUMPR )
						{
							fReqPidNotSupported = TRUE;
							Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for gasoline vehicles)\n", IdIndex );
						}
					}
					break;

					case 0x0D:
					{
						/* Must be supported for all light duty vehicles */
						if ( gUserInput.eVehicleType == LD )
						{
							fReqPidNotSupported = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for all light duty vehicles)\n", IdIndex );
						}
						/* Warn if not supported for gasoline vehicles */
						else if ( gUserInput.eVehicleType == MD ||  gUserInput.eVehicleType == HD )
						{
							Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for all medium and heavy duty vehicles)\n", IdIndex );
						}
					}
					break;

					case 0x03:
					case 0x11:
					case 0x0E:
					{
						/* Must be supported for gasoline vehicles */
						if ( gOBDDieselFlag == FALSE )
						{
							fReqPidNotSupported = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for gasoline vehicles)\n", IdIndex );
						}
					}
					break;

					case 0x2F:
					{
						/* Warn if not supported for ISO15765 vehicles */
						if ( gOBDList[gOBDListIndex].Protocol == ISO15765 )
						{
							fReqPidNotSupported = TRUE;
							Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for ISO15765 vehicles)\n", IdIndex );
						}
					}
					break;

					case 0x41:
					{
						/* Must be supported for ISO15765 protocol on US_OBDII */
						if (gOBDList[gOBDListIndex].Protocol == ISO15765)
						{
							if ( gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD )
							{
								fReqPidNotSupported = TRUE;
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "SID $1 PID $%02X not supported (Support required for ISO15765 protocol vehicles)\n", IdIndex );
							}
							else
							{
								Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "SID $1 PID $%02X not supported\n", IdIndex );
							}
						}
					}
					break;

					case 0x1F:
					case 0x30:
					case 0x33:
					case 0x42:
					{
						/* Must be supported for ISO15765 protocol */
						if (gOBDList[gOBDListIndex].Protocol == ISO15765)
						{
							fReqPidNotSupported = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for ISO15765 protocol vehicles)\n", IdIndex );
						}
					}
					break;

					case 0x2E:
					{
						/* Warn if not supported for ISO15765 gasoline vehicles */
						if ( gOBDList[gOBDListIndex].Protocol == ISO15765 &&
						     gOBDDieselFlag == FALSE )
						{
							fReqPidNotSupported = TRUE;
							Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for ISO15765 gasoline vehicles)\n", IdIndex );
						}
					}
					break;

					case 0x43:
					case 0x44:
					{
						/* Must be supported for ISO15765 gasoline vehicles */
						if ( gOBDList[gOBDListIndex].Protocol == ISO15765 &&
						     gOBDDieselFlag == FALSE )
						{
							if (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD)
							{
								fReqPidNotSupported = TRUE;
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "SID $1 PID $%02X not supported (Support required for ISO15765 gasoline vehicles)\n", IdIndex );
							}
							else
							{
								Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "SID $1 PID $%02X not supported\n", IdIndex );
							}
						}
					}
					break;

					case 0x46:
					{
						/* Must be supported for US vehicles */
						if ( gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD )
						{
							fReqPidNotSupported = TRUE;
							Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for all vehicles)\n", IdIndex );
						}
					}
					break;

					case 0x49:
					{
						/* Must be supported for ISO15765 diesel vehicles */
						if ( gOBDList[gOBDListIndex].Protocol == ISO15765 &&
						     gOBDDieselFlag == TRUE )
						{
							fReqPidNotSupported = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for ISO15765 diesel vehicles)\n", IdIndex );
						}
					}
					break;

					case 0x51:
					{
						/* Must be supported for MY 2015 vehicles */
						if ( gModelYear >= 2015  &&
						     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
						{
							fReqPidNotSupported = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2015 and beyond vehicles)\n", IdIndex );
						}
					}
					break;

					case 0x5C:
					case 0x5D:
					case 0x5E:
					case 0x65:
					{
						/* Warn if not supported for MY 2010 diesel vehicles */
						if ( gModelYear >= 2010 &&
						     gOBDDieselFlag == TRUE &&
						     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
						{
							Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2010 and beyond diesel vehicles)\n", IdIndex );
						}
					}
					break;

					case 0x5B:
					{
						/* Must be supported for MY 2013 hybrid vehicles */
						if ( gModelYear >= 2013 &&
						     (gUserInput.ePwrTrnType == HEV || gUserInput.ePwrTrnType == PHEV) &&
						     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
						{
							fReqPidNotSupported = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2013 and beyond hybrid vehicles)\n", IdIndex );
						}
					}
					break;

					case 0x61:
					{
						/* Must be supported for MY 2010 diesel vehicles */
						if ( gModelYear >= 2010 &&
						     gOBDDieselFlag == TRUE &&
						     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
						{
							fReqPidNotSupported = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2010 and beyond diesel vehicles)\n", IdIndex );
						}
					}
					break;

					case 0x62:
					{
						/* Must be supported for MY 2010 diesel vehicles */
						if ( gModelYear >= 2010 &&
						     gOBDDieselFlag == TRUE &&
						     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
						{
							fReqPidNotSupported = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2010 and beyond diesel vehicles)\n", IdIndex );
						}
						else if ( gModelYear >= 2019 && gModelYear < 2021 &&
						          gUserInput.eComplianceType == US_OBDII )
						{
							fReqPidNotSupported = TRUE;
							Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2019 and beyond OBD-II vehicles)\n", IdIndex );
						}
						else if ( gModelYear >= 2021 &&
						          gUserInput.eComplianceType == US_OBDII )
						{
							fReqPidNotSupported = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2021 and beyond OBD-II vehicles)\n", IdIndex );
						}
					}
					break;

					case 0x63:
					{
						/* Must be supported for MY 2010 heavy duty diesel vehicles */
						if ( gModelYear >= 2010 &&
						     gOBDDieselFlag == TRUE &&
						     gUserInput.eComplianceType == HD_OBD )
						{
							fReqPidNotSupported = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2010 and beyond heavy duty diesel vehicles)\n", IdIndex );
						}
						else if ( gModelYear >= 2019 &&
						          gOBDDieselFlag == TRUE &&
						          gUserInput.eComplianceType == US_OBDII && gUserInput.eVehicleType == MD )
						{
							fReqPidNotSupported = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2019 and beyond medium duty diesel vehicles)\n", IdIndex );
						}
						else if ( gModelYear >= 2019 && gModelYear < 2021 &&
						          gUserInput.eComplianceType == US_OBDII )
						{
							fReqPidNotSupported = TRUE;
							Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2019 and beyond OBD-II vehicles)\n", IdIndex );
						}
						else if ( gModelYear >= 2021 &&
						          gUserInput.eComplianceType == US_OBDII )
						{
							fReqPidNotSupported = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2021 and beyond OBD-II vehicles)\n", IdIndex );
						}
					}
					break;

					case 0x7D:
					case 0x7E:
					case 0x81:
					{
						/* Must be supported for MY 2010 heavy duty diesel vehicles */
						if ( gModelYear >= 2010 &&
						     gOBDDieselFlag == TRUE &&
						     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) &&
						     (gUserInput.eVehicleType == HD || gUserInput.eVehicleType == MD) )
						{
							fReqPidNotSupported = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2010 and beyond heavy duty diesel vehicles)\n", IdIndex );
						}
					}
					break;

					case 0x7F:
					{
						/* Must be supported for MY 2010 heavy duty vehicles */
						if ( gModelYear >= 2010 &&
						     ( gUserInput.eComplianceType == HD_OBD ||
						       (gOBDDieselFlag == TRUE && gUserInput.eVehicleType == MD) ) )
						{
							fReqPidNotSupported = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2010 and beyond heavy duty vehicles)\n", IdIndex );
						}
					}
					break;

					case 0x85:
					case 0x8B:
					{
						/* Must be supported for MY 2013 diesel vehicles */
						if ( gModelYear >= 2013 &&
						     gOBDDieselFlag == TRUE &&
						     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) &&
						     ( IdIndex != 0x8B || fPidSupported[0x9B] == FALSE) )
						{
							fReqPidNotSupported = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2013 and beyond diesel vehicles)\n", IdIndex );
						}
					}
					break;

					case 0x88:
					{
						/* Must be supported for MY 2013 heavy duty diesel vehicles */
						if ( gModelYear >= 2013 &&
						     gOBDDieselFlag == TRUE &&
						     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) &&
						     (gUserInput.eVehicleType == HD || gUserInput.eVehicleType == MD) )
						{
							fReqPidNotSupported = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2013 and beyond heavy duty diesel vehicles)\n", IdIndex );
						}
					}
					break;

					case 0x8E:
					{
						/* Must be supported for MY 2010 heavy duty diesel vehicles */
						if ( gModelYear >= 2010 &&
						     gOBDDieselFlag == TRUE &&
						     gUserInput.eComplianceType == HD_OBD )
						{
							Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2010 and beyond heavy duty diesel vehicles)\n", IdIndex );
						}
						else if ( gModelYear >= 2019 &&
						          gOBDDieselFlag == TRUE &&
						          gUserInput.eComplianceType == US_OBDII && gUserInput.eVehicleType == MD )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2019 and beyond heavy duty diesel vehicles)\n", IdIndex );
						}
						else if ( gModelYear >= 2019 && gModelYear < 2021 &&
						          gUserInput.eComplianceType == US_OBDII )
						{
							fReqPidNotSupported = TRUE;
							Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2019 and beyond OBD-II vehicles)\n", IdIndex );
						}
						else if ( gModelYear >= 2021 &&
						          gUserInput.eComplianceType == US_OBDII )
						{
							fReqPidNotSupported = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2021 and beyond OBD-II vehicles)\n", IdIndex );
						}
					}
					break;

					case 0x9A:
					{
						/* Must be supported for MY 2019 PHEV vehicles */
						if ( gModelYear >= 2019 &&
						     gOBDPlugInFlag == TRUE &&
						     gUserInput.eComplianceType == US_OBDII )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2019 and beyond plug-in hybrid electric vehicles)\n", IdIndex );
						}
					}
					break;

					case 0x9D:
					case 0x9E:
					{
						/* Must be supported for MY 2016 heavy duty diesel vehicles */
						if ( gModelYear >= 2016 &&
						     gOBDDieselFlag == TRUE &&
						     gUserInput.eComplianceType == HD_OBD )
						{
							Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2016 and beyond heavy duty diesel vehicles)\n", IdIndex );
						}
						else if ( gModelYear >= 2019 &&
						          gOBDDieselFlag == TRUE &&
						          gUserInput.eComplianceType == US_OBDII && gUserInput.eVehicleType == MD )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2019 and beyond medium duty diesel vehicles)\n", IdIndex );
						}
						else if ( gModelYear >= 2019 && gModelYear < 2021 &&
						          gUserInput.eComplianceType == US_OBDII )
						{
							fReqPidNotSupported = TRUE;
							Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2019 and beyond OBD-II vehicles)\n", IdIndex );
						}
						else if ( gModelYear >= 2021 &&
						          gUserInput.eComplianceType == US_OBDII )
						{
							fReqPidNotSupported = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2021 and beyond OBD-II vehicles)\n", IdIndex );
						}
					}
					break;

					case 0xA2:
					{
						/* Must be supported for MY 2016 heavy duty diesel vehicles */
						if ( gModelYear >= 2016 &&
						     gOBDDieselFlag == TRUE &&
						     gUserInput.eComplianceType == HD_OBD )
						{
							Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2016 and beyond heavy duty diesel vehicles)\n", IdIndex );
						}
						else if ( gModelYear >= 2019 &&
						          gOBDDieselFlag == TRUE &&
						          gUserInput.eComplianceType == US_OBDII )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2019 and beyond OBD-II diesel vehicles)\n", IdIndex );
						}
					}
					break;

					case 0xA5:
					{
						if ( gModelYear >= 2019 &&
						     gOBDDieselFlag == TRUE &&
						     gUserInput.eComplianceType == US_OBDII )
						{
							fReqPidNotSupported = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2019 and beyond OBD-II diesel vehicles)\n", IdIndex );
						}
					}
					break;

					case 0xA6:
					{
						if ( gModelYear >= 2019 && gModelYear < 2021 &&
						     gUserInput.eComplianceType == US_OBDII )
						{
							fReqPidNotSupported = TRUE;
							Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2019 and beyond OBD-II vehicles)\n", IdIndex );
						}
						else if ( gModelYear >= 2021 &&
						          gUserInput.eComplianceType == US_OBDII )
						{
							fReqPidNotSupported = TRUE;
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "SID $1 PID $%02X not supported (Support required for MY 2021 and beyond OBD-II vehicles)\n", IdIndex );
						}
					}
					break;
				}  /* end switch ( IdIndex ) */

			}  /* end if ( fPidSupported[IdIndex] == FALSE ) */

		}  /* end for ( IdIndex */


		if ( fReqPidNotSupported == FALSE )
		{
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "All required SID $1 PIDs supported!\n");
		}
		else
		{
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Not all required SID $1 PIDs are supported\n" );
		}


		/* Try group support if ISO15765 */
		if (gOBDList[gOBDListIndex].Protocol == ISO15765)
		{
			if ( VerifyGroupDiagnosticSupport() == FAIL )
			{
				if ( Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT,
				          "Problems verifying group diagnostic support.\n" ) == 'N' )
				{
					return (FAIL);
				}
			}
		}

		/* Link active test to verify communication remained active for ALL protocols */
		if (VerifyLinkActive() != PASS)
		{
			return (FAIL);
		}
	} /* end if ( OBDEngineDontCare == FALSE) */

	if ( ulInit_FailureCount != GetFailureCount() )
	{
		/* There could have been early/late responses that weren't treated as FAIL
		 * or PIDs out of range
		*/
		return(FAIL);
	}

	return(PASS);
}

/*
********************************************************************************
**	FUNCTION    VerifyM01P01
**
**	Purpose     Isolated function an align to specification SAEJ1699 rev 11.5.
********************************************************************************
**    DATE      MODIFICATION
**    05/14/04  Created function / aligned test to SAEJ1699 rev 11.5.
********************************************************************************
*/
STATUS VerifyM01P01(
                     SID1 *pSid1,                    // pointer to SID $01 Data
                     unsigned long SidIndex,         // offset into SID $01 Data
                     unsigned long EcuIndex,         // index of associated ECU
                     unsigned long *pulBitsSupported // bit map of bits that must
                                                     // set on at least one ECU
                   )
{

	BOOL bTestFailed = FALSE;


	/* Check if the MIL light is ON (SID 1 PID 1 Data A Bit 7) */
	if (pSid1[SidIndex].Data[0] & 0x80)
	{
		if (gOBDDTCStored == FALSE)
		{
			/* MIL is ON when there should not be a stored DTC */
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  MIL status failure.\n", GetEcuId(EcuIndex) );
			bTestFailed = TRUE;
		}
	}

	/* Check if any DTC status bits (SID 1 PID 1 Data A Bits 0-6) */
	if (pSid1[SidIndex].Data[0] & 0x7F)
	{
		if (gOBDDTCStored == FALSE)
		{
			/*
			** DTC status bit(s) set when there should not
			** be a stored DTC
			*/
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  DTC status failure.\n", GetEcuId(EcuIndex) );
			bTestFailed = TRUE;
		}
	}

	/* Evaluate Data B, BIT 0 - Misfire support */
	if ( (pSid1[SidIndex].Data[1] & 0x01) == 0x01)
	{
		*pulBitsSupported = *pulBitsSupported | DATA_B_BIT_0;
	}

	/* Evaluate Data B, BIT 1 - Fuel support */
	if ( (pSid1[SidIndex].Data[1] & 0x02) == 0x02)
	{
		*pulBitsSupported = *pulBitsSupported | DATA_B_BIT_1;
	}

	/* Evaluate Data B, BIT 2 - CCM support */
	if ( ( pSid1[SidIndex].Data[1] & 0x04 ) != 0x04 )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  CCM  monitor must be supported.\n", GetEcuId(EcuIndex) );
		bTestFailed = TRUE;
	}

	/* Check Data B BIT 3 state */
	/* only check if MY 2010 and beyond and ECU does not only supports CCM requirements (SID 1 PID 1 Data B bit 2==1) */
	if ( gModelYear >= 2010 &&
	     ( ((Sid1Pid1[EcuIndex].Data[1]) & 0x03) != 0x00 ||
	       ((Sid1Pid1[EcuIndex].Data[2]) & 0xFF) != 0x00 ) )
	{
		if ( (pSid1[SidIndex].Data[1] & 0x08) == 0 &&
		     gOBDDieselFlag == 1  )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $1 PID $1 Data B bit 3 clear (Spark Ignition) does not match user input (Diesel)\n", GetEcuId(EcuIndex) );
			bTestFailed = TRUE;
		}
		else if ( (pSid1[SidIndex].Data[1] & 0x08) == 0x08 &&
		     gOBDDieselFlag == 0  )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  SID $1 PID $1 Data B bit 3 set (Compression Ignition) does not match user input (Non Diesel)\n", GetEcuId(EcuIndex) );
			bTestFailed = TRUE;
		}
	}

	/* Evaluate Data B, BIT 4 - Misfire */
	if ( (pSid1[SidIndex].Data[1] & 0x01) == 0x01)
	 {
		/* if Misfire is supported */
		if (gOBDDieselFlag == FALSE )
		{
			/* if it's a spark engine */
			if ( ( pSid1[SidIndex].Data[1] & 0x10 ) != 0x00 )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  Misfire Monitor must be '0' for spark ignition vehicles or controllers that do not support misfire.\n", GetEcuId(EcuIndex) );
				bTestFailed = TRUE;
			}
		}
		else
		{
		/* Bit 4 may be 0 or 1 for compression ignition w/ or wo/ Engine running
		   except in Test 11 where it must be complete (0). */
			if (TestPhase == eTestPerformanceCounters)
			{
				if ((pSid1[SidIndex].Data[1] & 0x10) != 0x00)
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  Misfire Monitor must be '0' for compression ignition vehicles.\n", GetEcuId(EcuIndex) );
					bTestFailed = TRUE;
				}
			}
		}
	}
	/* If Misfire unsupported it must indicate complete */
	if ( ( ( pSid1[SidIndex].Data[1] & 0x01 ) == 0x00 ) &&
	       ( pSid1[SidIndex].Data[1] & 0x10 ) == 0x10 )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  Misfire test: unsupported monitor must indicate complete.\n", GetEcuId(EcuIndex) );
		bTestFailed = TRUE;
	}

	/* Evaluate Data B, BIT 5 - Fuel status */
	if ( (pSid1[SidIndex].Data[1] & 0x02 ) == 0x02)
	{
		// only check for tests 11.3 and 11.7 (not 10.6)
		if ( TestPhase == eTestPerformanceCounters &&
		     (pSid1[SidIndex].Data[1] & 0x20 ) == 0x20 )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  Fuel system must indicate complete.\n", GetEcuId(EcuIndex) );
			bTestFailed = TRUE;
		}
	}
	/* If fuel system monitor unsupported it must indicate complete */
	if ( ( ( pSid1[SidIndex].Data[1] & 0x02 ) == 0x00 ) &&
	       ( pSid1[SidIndex].Data[1] & 0x20 ) == 0x20 )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  Fuel system: unsupported monitor must indicate complete.\n", GetEcuId(EcuIndex) );
		bTestFailed = TRUE;
	}

	/* Evaluate Data B, BIT 6 - CCM status */
	if ( (pSid1[SidIndex].Data[1] & 0x40) == 0x40 )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  CCM must indicate complete.\n", GetEcuId(EcuIndex) );
		bTestFailed = TRUE;
	}

	/* Check Data B bit 7 if reserved bit is set */
	if (pSid1[SidIndex].Data[1] & 0x80)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  Reserved I/M readiness Data B bit 7 set\n", GetEcuId(EcuIndex) );
		bTestFailed = TRUE;
	}

	/* Evaluate Data C, BIT 0 */
	if (pSid1[SidIndex].Data[2] & 0x01)
	{
		*pulBitsSupported = *pulBitsSupported | DATA_C_BIT_0;
	}

	/* Evaluate Data C, BIT 2 */
	if (pSid1[SidIndex].Data[2] & 0x04)
	{
		if ( gOBDDieselFlag != TRUE &&
		     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
		{
			*pulBitsSupported = *pulBitsSupported | DATA_C_BIT_2;
		}
		else
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  Reserved I/M readiness Data C bit 2 set\n", GetEcuId(EcuIndex) );
			bTestFailed = TRUE;
		}
	}

	/* Evaluate Data C, BIT 3 */
	if (pSid1[SidIndex].Data[2] & 0x08)
	{
		*pulBitsSupported = *pulBitsSupported | DATA_C_BIT_3;
	}

	/* Evaluate Data C, BIT 4 */
	if (pSid1[SidIndex].Data[2] & 0x10)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  Reserved I/M readiness Data C bit 4 set\n", GetEcuId(EcuIndex) );
		bTestFailed = TRUE;
	}

	/* Evaluate Data C, BIT 5 */
	if (pSid1[SidIndex].Data[2] & 0x20)
	{
		*pulBitsSupported = *pulBitsSupported | DATA_C_BIT_5;
	}

	/* Evaluate Data C, BIT 6 */
	if (pSid1[SidIndex].Data[2] & 0x40)
	{
		*pulBitsSupported = *pulBitsSupported | DATA_C_BIT_6;
	}

	/* Evaluate Data C, BIT 7 */
	if (pSid1[SidIndex].Data[2] & 0x80)
	{
		*pulBitsSupported = *pulBitsSupported | DATA_C_BIT_7;
	}

	/* Evaluate Data D, BIT 2 */
	if ( ((pSid1[SidIndex].Data[3] & 0x04) == 0x04) && (gOBDDieselFlag == TRUE ) )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  Reserved I/M readiness Data D bit 2 set\n", GetEcuId(EcuIndex) );
		bTestFailed = TRUE;
	}

	/* Evaluate Data D, BIT 4 */
	if (pSid1[SidIndex].Data[3] & 0x10)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  Reserved I/M readiness Data D bit 4 set\n", GetEcuId(EcuIndex) );
		bTestFailed = TRUE;
	}

	if ( (gOBDEngineRunning == FALSE) && (TestPhase != eTestPerformanceCounters) )  // test 5.6 & 10.6
	{
		/*
		 * supported monitors should not be complete (except O2 on spark ignition)
		 */
		/* DATA_D Bit 0 */
		if (
		     ( (pSid1[SidIndex].Data[2] & 0x01) == 0x01 ) &&
		     ( (pSid1[SidIndex].Data[3] & 0x01) == 0x00 )
		   )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  %s: Supported monitor must indicate 'incomplete'\n",
			     GetEcuId(EcuIndex), gOBDDieselFlag ? "HCCAT" : "CAT" );
			bTestFailed = TRUE;
		}

		/* DATA_D Bit 1 */
		if (
		     ( (pSid1[SidIndex].Data[2] & 0x02) == 0x02 ) &&
		     ( (pSid1[SidIndex].Data[3] & 0x02) == 0x00 )
		   )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  %s: Supported monitor must indicate 'incomplete'\n",
			     GetEcuId(EcuIndex), gOBDDieselFlag ? "NCAT" : "HCAT" );
			bTestFailed = TRUE;
		}

		/* DATA_D Bit 2 */
		if (
		     ( (pSid1[SidIndex].Data[2] & 0x04) == 0x04 ) &&
		     ( (pSid1[SidIndex].Data[3] & 0x04) == 0x00 )
		   )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  %s: Supported monitor must indicate 'incomplete'\n",
			     GetEcuId(EcuIndex), gOBDDieselFlag ? "RESERVED2" : "EVAP" );
			bTestFailed = TRUE;
		}

		/* DATA_D Bit 3 */
		if (
		     ( (pSid1[SidIndex].Data[2] & 0x08) == 0x08 ) &&
		     ( (pSid1[SidIndex].Data[3] & 0x08) == 0x00 )
		   )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  %s: Supported monitor must indicate 'incomplete'\n",
			     GetEcuId(EcuIndex), gOBDDieselFlag ? "BP" : "AIR" );
			bTestFailed = TRUE;
		}

		/* DATA_D Bit 4 */
		if (
		     ( (pSid1[SidIndex].Data[2] & 0x10) == 0x10 ) &&
		     ( (pSid1[SidIndex].Data[3] & 0x10) == 0x00 )
		   )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  RESERVED4: Supported monitor must indicate 'incomplete'\n",
			     GetEcuId(EcuIndex) );
			bTestFailed = TRUE;
		}

		/* DATA_D Bit 5 */
		if (
		     ( (pSid1[SidIndex].Data[2] & 0x20) == 0x20 ) &&
		     ( (pSid1[SidIndex].Data[3] & 0x20) == 0x00 )
		   )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  %s: Supported monitor must indicate 'incomplete'\n",
			     GetEcuId(EcuIndex), gOBDDieselFlag ? "EGS" : "O2S" );
			bTestFailed = TRUE;
		}

		/* DATA_D Bit 6 */
		if (
		     ( (pSid1[SidIndex].Data[2] & 0x40) == 0x40 ) &&
		     ( (pSid1[SidIndex].Data[3] & 0x40) == 0x00 ) && gOBDDieselFlag
		   )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  %s: Supported monitor must indicate 'incomplete'\n",
			     GetEcuId(EcuIndex), gOBDDieselFlag ? "PM" : "HTR" );
			bTestFailed = TRUE;
		}

		/* DATA_D Bit 7 */
		if (
		     ( (pSid1[SidIndex].Data[2] & 0x80) == 0x80 ) &&
		     ( (pSid1[SidIndex].Data[3] & 0x80) == 0x00 )
		   )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  EGR: Supported monitor must indicate 'incomplete'\n",
			     GetEcuId(EcuIndex) );
			bTestFailed = TRUE;
		}
	}


	/*
	 * unsupported monitors should be complete
	 */
	/* DATA_D Bit 0 */
	if (
	     ( (pSid1[SidIndex].Data[2] & 0x01) == 0x00 ) &&
	     ( (pSid1[SidIndex].Data[3] & 0x01) == 0x01 )
	   )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  %s: Unsupported monitor must indicate 'complete'\n",
		     GetEcuId(EcuIndex), gOBDDieselFlag ? "HCCAT" : "CAT" );
		bTestFailed = TRUE;
	}

	/* DATA_D Bit 1 */
	if (
	     ( (pSid1[SidIndex].Data[2] & 0x02) == 0x00 ) &&
	     ( (pSid1[SidIndex].Data[3] & 0x02) == 0x02 )
	   )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  %s: Unsupported monitor must indicate 'complete'\n",
		     GetEcuId(EcuIndex), gOBDDieselFlag ? "NCAT" : "HCAT" );
		bTestFailed = TRUE;
	}

	/* DATA_D Bit 2 */
	if (
	     ( (pSid1[SidIndex].Data[2] & 0x04) == 0x00 ) &&
	     ( (pSid1[SidIndex].Data[3] & 0x04) == 0x04 )
	   )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  %s: Unsupported monitor must indicate 'complete'\n",
		     GetEcuId(EcuIndex), gOBDDieselFlag ? "RESERVED2" : "EVAP" );
		bTestFailed = TRUE;
	}

	/* DATA_D Bit 3 */
	if (
	     ( (pSid1[SidIndex].Data[2] & 0x08) == 0x00 ) &&
	     ( (pSid1[SidIndex].Data[3] & 0x08) == 0x08 )
	   )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  %s: Unsupported monitor must indicate 'complete'\n",
		     GetEcuId(EcuIndex), gOBDDieselFlag ? "BP" : "AIR" );
		bTestFailed = TRUE;
	}

	/* DATA_D Bit 4 */
	if (
	     ( (pSid1[SidIndex].Data[2] & 0x10) == 0x00 ) &&
	     ( (pSid1[SidIndex].Data[3] & 0x10) == 0x10 )
	   )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  RESERVED4: Unsupported monitor must indicate 'complete'\n",
		     GetEcuId(EcuIndex) );
		bTestFailed = TRUE;
	}

	/* DATA_D Bit 5 */
	if (
	     ( (pSid1[SidIndex].Data[2] & 0x20) == 0x00 ) &&
	     ( (pSid1[SidIndex].Data[3] & 0x20) == 0x20 )
	   )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  %s: Unsupported monitor must indicate 'complete'\n",
		     GetEcuId(EcuIndex), gOBDDieselFlag ? "EGS" : "O2S" );
		bTestFailed = TRUE;
	}

	/* DATA_D Bit 6 */
	if (
	     ( (pSid1[SidIndex].Data[2] & 0x40) == 0x00 ) &&
	     ( (pSid1[SidIndex].Data[3] & 0x40) == 0x40 )
	   )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  %s: Unsupported monitor must indicate 'complete'\n",
		     GetEcuId(EcuIndex), gOBDDieselFlag ? "PM" : "HTR" );
		bTestFailed = TRUE;
	}

	/* DATA_D Bit 7 */
	if (
	     ( (pSid1[SidIndex].Data[2] & 0x80) == 0x00 ) &&
	     ( (pSid1[SidIndex].Data[3] & 0x80) == 0x80 )
	   )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  EGR: Unsupported monitor must indicate 'complete'\n",
		     GetEcuId(EcuIndex) );
		bTestFailed = TRUE;
	}

	if ( TestPhase == eTestPerformanceCounters )
	{
		if (pSid1[SidIndex].Data[3] != 0)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  Supported monitors not complete after I/M drive cycle\n", GetEcuId(EcuIndex) );
			bTestFailed = TRUE;
		}
	}

	if (bTestFailed == TRUE)
	{
		return ( FAIL );
	}
	else
	{
		return ( PASS );
	}
}

/*
********************************************************************************
**	FUNCTION    VerifyIM_Ready
**
**	Purpose     Isolated function to request SID 1 PID 1 and check IM Ready
********************************************************************************
*/
STATUS VerifyIM_Ready (void)
{
	unsigned long EcuIndex;

	unsigned long temp_data_long;

	BOOL bTestFailed = FALSE;

	SID_REQ SidReq;
	SID1 *pSid1;

	if (IsSid1PidSupported (-1, 1) == FALSE)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $1 PID $01 not supported\n");
		bTestFailed = TRUE;
	}

	SidReq.SID = 1;
	SidReq.NumIds = 1;
	SidReq.Ids[0] = 1;
	if ( SidRequest(&SidReq, SID_REQ_NORMAL) == FAIL )
	{
		/* There must be a response for ISO15765 protocol */
		if (gOBDList[gOBDListIndex].Protocol == ISO15765)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $1 PID $01 request\n");
			bTestFailed = TRUE;
		}
	}
	else
	{
		for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
		{
			if (gOBDResponse[EcuIndex].Sid1PidSize != 0)
			{
				break;
			}
		}

		if (EcuIndex >= gOBDNumEcus)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "No SID $1 PID $01 data\n");
			bTestFailed = TRUE;
		}
		else
		{
			/* Verify that all SID 1 PID data is valid */
			for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
			{
				/* If PID is supported, check it */
				if (IsSid1PidSupported (EcuIndex, 1) == TRUE)
				{
					/* Check the data to see if it is valid */
					pSid1 = (SID1 *)&gOBDResponse[EcuIndex].Sid1Pid[0];
					if ( gOBDResponse[EcuIndex].Sid1PidSize != 0 )
					{
						if ( VerifyM01P01( pSid1, 0, EcuIndex, &temp_data_long ) != PASS )   // temp_data_long is ignored
						{
							bTestFailed = TRUE;
						}
					}
					else
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "SID $1 PID $1 data size is 0\n");
						bTestFailed = TRUE;
					}
				}
			}
		}
	}

	// if this is test 11.3, check for appropriate value of SID $1 PID $31 and $4E
	if ( (TestPhase == eTestPerformanceCounters) && (TestSubsection == 3) )
	{
		/* If PID $31 is supported, request it */
		if (IsSid1PidSupported ( -1, 0x31 ) == TRUE)
		{
			SidReq.SID = 1;
			SidReq.NumIds = 1;
			SidReq.Ids[0] = 0x31;
			if ( SidRequest(&SidReq, SID_REQ_NORMAL) == FAIL )
			{
				/* There must be a response for ISO15765 protocol */
				if (gOBDList[gOBDListIndex].Protocol == ISO15765)
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "SID $1 PID $31 request\n");
					bTestFailed = TRUE;
				}
			}
			else
			{
				/* Check the data to see if it is valid */
				for ( EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
				{
					if (IsSid1PidSupported ( EcuIndex, 0x31 ) == TRUE)
					{
						pSid1 = (SID1 *)&gOBDResponse[EcuIndex].Sid1Pid[0];
						temp_data_long = ((pSid1->Data[0] * 256) +
						                  pSid1->Data[1]);
						if ( gOBDResponse[EcuIndex].Sid1PidSize != 0 )
						{
							if ( temp_data_long == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  SID $1 PID $31 is 0\n", GetEcuId(EcuIndex) );
								bTestFailed = TRUE;
							}
						}
						else
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $1 PID $31 size is 0\n", GetEcuId(EcuIndex) );
							bTestFailed = TRUE;
						}
					}
				}
			}
		}

		/* If PID $4E is supported, request it */
		if (IsSid1PidSupported ( -1, 0x4E ) == TRUE)
		{
			SidReq.SID = 1;
			SidReq.NumIds = 1;
			SidReq.Ids[0] = 0x4E;
			if ( SidRequest(&SidReq, SID_REQ_NORMAL) == FAIL )
			{
				/* There must be a response for ISO15765 protocol */
				if (gOBDList[gOBDListIndex].Protocol == ISO15765)
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "SID $1 PID $4E request\n");
					bTestFailed = TRUE;
				}
			}
			else
			{
				/* Check the data to see if it is valid */
				for ( EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
				{
					if (IsSid1PidSupported ( EcuIndex, 0x4E ) == TRUE)
					{
						pSid1 = (SID1 *)&gOBDResponse[EcuIndex].Sid1Pid[0];
						temp_data_long = ((pSid1->Data[0] * 256) +
						                  pSid1->Data[1]);
						if ( gOBDResponse[EcuIndex].Sid1PidSize != 0 )
						{
							if ( temp_data_long == 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  SID $1 PID $4E is 0\n", GetEcuId(EcuIndex) );
								bTestFailed = TRUE;
							}
						}
						else
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $1 PID $4E size is 0\n", GetEcuId(EcuIndex) );
							bTestFailed = TRUE;
						}
					}
				}
			}
		}
	}

	if (bTestFailed == TRUE)
	{
		return ( FAIL );
	}
	else
	{
		return ( PASS );
	}
}


/*
*******************************************************************************
**	Function:   VerifySid1PidSupportData
**
**	Purpose:    Verify each controller supports at a minimum one PID.
**	            Any ECU that responds that does not support at least
**	            one PID is flagged as an error.
**
*******************************************************************************
*/
int VerifySid1PidSupportData (void)
{
	int             bReturn = PASS;
	int             bEcuResult;
	unsigned long   EcuIndex;
	unsigned long   Index;

	/* For each ECU */
	for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
	{
		bEcuResult = FAIL;
		/* PID $00 not required to indicate PIDs Supported */
		for (Index = 1; Index < gOBDResponse[EcuIndex].Sid1PidSupportSize; Index++)
		{
			/* If PID is supported, keep looking */
			if ( gOBDResponse[EcuIndex].Sid1PidSupport[Index].IDBits[0] ||
			     gOBDResponse[EcuIndex].Sid1PidSupport[Index].IDBits[1] ||
			     gOBDResponse[EcuIndex].Sid1PidSupport[Index].IDBits[2] ||
			     ((gOBDResponse[EcuIndex].Sid1PidSupport[Index].IDBits[3]) & 0xFE) != 0x00 )
			{
				bEcuResult = PASS;
				break;
			}
		}

		if ((bEcuResult == FAIL) && (gOBDResponse[EcuIndex].Sid1PidSupportSize > 1))
		{
			if ( Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT,
			          "ECU %X  SID $1 PID-Supported PIDs indicate no PIDs supported", GetEcuId(EcuIndex)) == 'N' )
			{
				bReturn = FAIL;
			}
		}
	}

	return bReturn;
}


/*
********************************************************************************
**	FUNCTION    RequestSID1SupportData
**
**	Purpose     Isolated function to identify support for SID 1 PID x
********************************************************************************
*/
STATUS RequestSID1SupportData (void)
{
	unsigned long EcuIndex;
	unsigned long IdIndex;
	unsigned long ulPIDSupport;	/* Evaluate $E0 PID support indication. */
	SID_REQ SidReq;

	/* Request SID 1 support data */
	for (IdIndex = 0x00; IdIndex < MAX_PIDS; IdIndex += 0x20)
	{
		SidReq.SID = 1;
		SidReq.NumIds = 1;
		SidReq.Ids[0] = (unsigned char)IdIndex;
		if ( SidRequest(&SidReq, SID_REQ_NORMAL) == FAIL )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "No response to SID $1 PID-Supported PID $%02X request\n", IdIndex );
			return (FAIL);
		}

		/* Check if we need to request the next group */
		for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
		{
			if (gOBDResponse[EcuIndex].Sid1PidSupport[IdIndex >> 5].IDBits[3] & 0x01)
			{
				break;
			}
		}
		if (EcuIndex >= gOBDNumEcus)
		{
			break;
		}
	}


	/* Verify support information */
	if ( EcuIndex != 0 && VerifySid1PidSupportData() == FAIL )
	{
		return (FAIL);
	}


	/* Verify support information if request is at upper limit of $E0 */
	if ( IdIndex == 0xE0 )
	{
		/* Init variable to no-support */
		ulPIDSupport = 0;

		/* For each ECU */
		for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
		{
			/* If MID is supported, keep looking */
			if ( ( gOBDResponse[EcuIndex].Sid1PidSupport[IdIndex >> 5].IDBits[0]        ||
			       gOBDResponse[EcuIndex].Sid1PidSupport[IdIndex >> 5].IDBits[1]        ||
			       gOBDResponse[EcuIndex].Sid1PidSupport[IdIndex >> 5].IDBits[2]        ||
			     ( gOBDResponse[EcuIndex].Sid1PidSupport[IdIndex >> 5].IDBits[3] & 0xFE ) ) != 0x00)
			{
				/* Flag as support indicated! */
				ulPIDSupport = 1;
			}
		}

		/* Flag as error if no support indicated in $E0 */
		if (ulPIDSupport == 0x00)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $1 PID $E0 support failure.  No PID support indicated!\n");
			return (FAIL);
		}
	}
	/* Request next unsupported PID-Supportd PID */
	else
	{
		SidReq.SID = 1;
		SidReq.NumIds = 1;
		SidReq.Ids[0] = (unsigned char)(IdIndex += 0x20);

		if ( SidRequest(&SidReq, SID_REQ_NORMAL|SID_REQ_IGNORE_NO_RESPONSE) != FAIL  )
		{
			/* J1850 & ISO9141 - No response preferred, but positive response
			** allowed
			*/
			if ( ( gOBDList[gOBDListIndex].Protocol == ISO15765 ) )
			{
				for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
				{
					if (gOBDResponse[EcuIndex].Sid1PidSize != 0)
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  Unexpected response to unsupported SID $1 PID $%02X!\n",
						     GetEcuId(EcuIndex),
						     IdIndex );
					}
				}
				return (FAIL);
			}
		}
	}

	return (PASS);
}

//*****************************************************************************
//
//	Function:   IsSid1PidSupported
//
//	Purpose:    Determine if SID 1 PID x is supported on specific ECU.
//	            Need to have called RequestSID1SupportData() previously.
//	            If EcuIndex < 0 then check all ECUs.
//
//*****************************************************************************
//
//	DATE        MODIFICATION
//	02/10/05    Created common function for this logic.
//
//*****************************************************************************
unsigned int IsSid1PidSupported (unsigned int EcuIndex, unsigned int PidIndex)
{
	int index1;
	int index2;
	int mask;

	if (PidIndex == 0)
		return TRUE;            // all modules must support SID 01 PID 00

	PidIndex--;

	index1 =  PidIndex >> 5;
	index2 = (PidIndex >> 3) & 0x03;
	mask   = 0x80 >> (PidIndex & 0x07);

	if ((signed int)EcuIndex < 0)
	{
		for (EcuIndex = 0; EcuIndex < gUserNumEcus; EcuIndex++)
		{
			if (gOBDResponse[EcuIndex].Sid1PidSupport[index1].IDBits[index2] & mask)
				return TRUE;
		}
	}
	else
	{
		if (gOBDResponse[EcuIndex].Sid1PidSupport[index1].IDBits[index2] & mask)
			return TRUE;
	}

	return FALSE;
}

//*****************************************************************************
//
//	Function:   DetermineVariablePidSize
//
//	Purpose:    Determine number of data bytes in PIDs $06 - $09, $55 - $58
//
//*****************************************************************************
STATUS DetermineVariablePidSize (void)
{
	SID_REQ SidReq;

	SID1    *pPid1;
	unsigned char pid[OBD_MAX_ECUS];
	unsigned long EcuIdIndex[OBD_MAX_ECUS];

	unsigned long EcuIndex, numResp;

	/* only need to check once */
	if (gSid1VariablePidSize != 0)
		return PASS;

	/* -1 ==> cannot determine the PID size */
	gSid1VariablePidSize = -1;

	/* only check if needed */
	if ((IsSid1PidSupported (-1, 0x06) == FALSE) &&
	    (IsSid1PidSupported (-1, 0x07) == FALSE) &&
	    (IsSid1PidSupported (-1, 0x08) == FALSE) &&
	    (IsSid1PidSupported (-1, 0x09) == FALSE) &&
	    (IsSid1PidSupported (-1, 0x55) == FALSE) &&
	    (IsSid1PidSupported (-1, 0x56) == FALSE) &&
	    (IsSid1PidSupported (-1, 0x57) == FALSE) &&
	    (IsSid1PidSupported (-1, 0x58) == FALSE) )
		return PASS;

	/* cannot support both PID $13 and $1D */
	if (IsSid1PidSupported (-1, 0x13) == TRUE &&
	    IsSid1PidSupported (-1, 0x1D) == TRUE)
	{
		for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
		{
			if (IsSid1PidSupported (EcuIndex, 0x13) == TRUE )
			{
				Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  Supports SID $01 PID $13\n", GetEcuId (EcuIndex));
			}

			if (IsSid1PidSupported (EcuIndex, 0x1D) == TRUE )
			{
				Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  Supports SID $01 PID $1D\n", GetEcuId (EcuIndex));
			}
		}
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Both PID $13 and $1D are supported\n");
		return FAIL;
	}

	/* check PID $13 first */
	if (IsSid1PidSupported (-1, 0x13) == TRUE)
	{
		SidReq.SID = 1;
		SidReq.NumIds = 1;
		SidReq.Ids[0] = 0x13;

		if ( SidRequest(&SidReq, SID_REQ_NORMAL) == FAIL )
		{
			/* There must be a response for ISO15765 protocol */
			if (gOBDList[gOBDListIndex].Protocol == ISO15765)
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "SID $1 PID $13 request\n");
				return FAIL;
			}
		}

		numResp = 0;
		for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
		{
			if (IsSid1PidSupported (EcuIndex, 0x13) == TRUE)
			{
				if (gOBDResponse[EcuIndex].Sid1PidSize > 0)
				{
					pPid1 = (SID1 *)(gOBDResponse[EcuIndex].Sid1Pid);
					if (pPid1->PID == 0x13)
					{
						Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  O2SLOC (PID $13) = $%02X\n", GetEcuId (EcuIndex), pPid1->Data[0] );
						EcuIdIndex[numResp] = EcuIndex;
						pid[numResp++] = pPid1->Data[0];
					}
				}
			}
		}

		if (numResp == 0 || numResp > 2)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $1 PID $13 supported by %d ECUs (1 to 2 expected)\n", numResp);
			return FAIL;
		}

		gSid1VariablePidSize = 1;
		return PASS;
	}

	/* check PID $1D second */
	if (IsSid1PidSupported (-1, 0x1D) == TRUE)
	{
		SidReq.SID = 1;
		SidReq.NumIds = 1;
		SidReq.Ids[0] = 0x1D;

		if ( SidRequest(&SidReq, SID_REQ_NORMAL) == FAIL )
		{
			/* There must be a response for ISO15765 protocol */
			if (gOBDList[gOBDListIndex].Protocol == ISO15765)
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "SID $1 PID $1D request\n");
				return FAIL;
			}
		}

		numResp = 0;
		for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
		{
			if (IsSid1PidSupported (EcuIndex, 0x1D) == TRUE)
			{
				if (gOBDResponse[EcuIndex].Sid1PidSize > 0)
				{
					pPid1 = (SID1 *)(gOBDResponse[EcuIndex].Sid1Pid);
					if (pPid1->PID == 0x1D)
					{
						Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  O2SLOC (PID $1D) = $%02X\n", GetEcuId (EcuIndex), pPid1->Data[0] );
						EcuIdIndex[numResp] = EcuIndex;
						pid[numResp++] = pPid1->Data[0];
					}
				}
			}
		}

		if (numResp == 1)
		{
			gSid1VariablePidSize = (pid[0] == 0xff) ? 2 : 1;
			return PASS;
		}

		if (numResp == 2)
		{
			if ( (pid[0] | pid[1]) != 0xff)
			{
				gSid1VariablePidSize = 1;
				return PASS;
			}

			if ( (pid[0] == 0xf0 && pid[1] == 0x0f) ||
			     (pid[0] == 0x0f && pid[1] == 0xf0) )
			{
				gSid1VariablePidSize = 2;
				return PASS;
			}

			if ( (pid[0] == 0xcc && pid[1] == 0x33) ||
			     (pid[0] == 0x33 && pid[1] == 0xcc) )
			{
				gSid1VariablePidSize = 2;
				return PASS;
			}

			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Unable to determine number of data bytes in PIDs $06 - $09\n");
			return FAIL;
		}

		/* numResp == 0 or numResp > 2 */
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     " SID $1 PID $1D supported by %d ECUs ( 1 to 2 expected)\n", numResp);
		return FAIL;
	}

	Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
	     "Neither SID $1 PID $13 nor $1D are supported\n");
	return FAIL;
}

//*****************************************************************************
//
//	Function:   GetPid4FArray
//
//	Purpose:    copy PID $4F values into an array.
//
//*****************************************************************************
STATUS GetPid4FArray (void)
{
	SID_REQ SidReq;
	SID1 *pPid1;
	unsigned int EcuIndex;

	for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
	{
		Pid4F[EcuIndex][0] = 0x00;
		Pid4F[EcuIndex][1] = 0x00;
		Pid4F[EcuIndex][2] = 0x00;
		Pid4F[EcuIndex][3] = 0x00;
	}

	if (IsSid1PidSupported (-1, 0x4F) == TRUE)
	{
		SidReq.SID = 1;
		SidReq.NumIds = 1;
		SidReq.Ids[0] = 0x4F;

		if ( SidRequest(&SidReq, SID_REQ_NORMAL) == FAIL )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $1 PID $4F request\n");
			return (FAIL);
		}

		for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
		{
			if (IsSid1PidSupported (EcuIndex, 0x4F) == TRUE)
			{
				if (gOBDResponse[EcuIndex].Sid1PidSize > 0)
				{
					pPid1 = (SID1 *)(gOBDResponse[EcuIndex].Sid1Pid);
					if (pPid1->PID == 0x4F)
					{
						Pid4F[EcuIndex][0] = pPid1->Data[0];
						Pid4F[EcuIndex][1] = pPid1->Data[1];
						Pid4F[EcuIndex][2] = pPid1->Data[2];
						Pid4F[EcuIndex][3] = pPid1->Data[3];
					}
				}
			}
		}
	}
	return (PASS);
}

//*****************************************************************************
//
//	Function:   GetPid50Array
//
//	Purpose:    copy PID $50 values into an array.
//
//*****************************************************************************
STATUS GetPid50Array (void)
{
	SID_REQ SidReq;
	SID1 *pPid1;
	unsigned int EcuIndex;

	if (IsSid1PidSupported (-1, 0x50) == TRUE)
	{
		SidReq.SID = 1;
		SidReq.NumIds = 1;
		SidReq.Ids[0] = 0x50;

		if ( SidRequest(&SidReq, SID_REQ_NORMAL) == FAIL )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $1 PID $50 request\n");
			return (FAIL);
		}

		for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
		{
			if (IsSid1PidSupported (EcuIndex, 0x50) == TRUE)
			{
				if (gOBDResponse[EcuIndex].Sid1PidSize > 0)
				{
					pPid1 = (SID1 *)(gOBDResponse[EcuIndex].Sid1Pid);
					if (pPid1->PID == 0x50)
					{
						Pid50[EcuIndex][0] = pPid1->Data[0];
						Pid50[EcuIndex][1] = pPid1->Data[1];
						Pid50[EcuIndex][2] = pPid1->Data[2];
						Pid50[EcuIndex][3] = pPid1->Data[3];
					}
				}
			}
		}
	}
	return (PASS);
}


//*****************************************************************************
//
//	Function:   GetHoursMinsSecs
//
//	Purpose:    Convert the time (in seconds) into hours, minutes and seconds .
//
//*****************************************************************************
void GetHoursMinsSecs( unsigned long time, unsigned long *hours, unsigned long *mins, unsigned long *secs)
{

	*hours = time/3600;

	*mins = (time - (*hours * 3600)) / 60;

	*secs = time - (*hours * 3600) - (*mins * 60);
}
