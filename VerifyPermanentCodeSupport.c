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
*******************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include "j2534.h"
#include "j1699.h"


unsigned char gService0ASupported = 0;
unsigned int  gSID0ASupECU[OBD_MAX_ECUS];


/*
*******************************************************************************
** VerifyPermanentCodeSupport -
** Function to verify SID A support
*******************************************************************************
*/
STATUS VerifyPermanentCodeSupport(void)
{
	SID_REQ SidReq;
	unsigned long EcuIndex;
	unsigned long NumDTCs;
	unsigned long DataOffset;
	unsigned long ulInit_FailureCount = 0;
	unsigned char TestFailed = FALSE;
	unsigned char i;


	if ( gOBDList[gOBDListIndex].Protocol != ISO15765 )
	{
		return(PASS);
	}

	ulInit_FailureCount = GetFailureCount();

	/* Count number of ECUs responding to SID A */
	SidReq.SID = 0x0A;
	SidReq.NumIds = 0;
	if ( SidRequest( &SidReq, SID_REQ_NORMAL|SID_REQ_IGNORE_NO_RESPONSE ) == FAIL )
	{
		/* An ECU supports Service 0A */
		for ( EcuIndex = 0, i = 0, gService0ASupported = 0; EcuIndex < gOBDNumEcus; EcuIndex++ )
		{
			if ( gOBDResponse[EcuIndex].SidASupported )
			{
				gService0ASupported++;
				gSID0ASupECU[i++] = GetEcuId(EcuIndex);
			}
		}

		/* Permanent code support required for MY 2010 and beyond */
		if ( (gModelYear >= 2010) &&
		     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "SID $0A request failed\n");
			return(FAIL);
		}
		else if ( ulInit_FailureCount != GetFailureCount() )
		{
			/* FAIL was due to actual problems */
			return(FAIL);
		}
		else
		{
			/* FAIL was due to no response, but none was required */
			return(PASS);
		}
	}


	for ( EcuIndex = 0, i = 0, gService0ASupported = 0; EcuIndex < gOBDNumEcus; EcuIndex++ )
	{
		if ( gOBDResponse[EcuIndex].SidASupported )
		{
			gService0ASupported++;
			gSID0ASupECU[i++] = GetEcuId(EcuIndex);
		}
	}


	/* If a permanent DTC is not supposed to be present (Test 5.4, 9.6, 9.22) */
	if ( gOBDDTCPermanent == FALSE )
	{
		/* Verify that SID A reports no DTCs stored */
		for ( EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++ )
		{
			/* check the Permanent DTCs count */
			if ( (gOBDResponse[EcuIndex].SidA[0] != 0x00) || (gOBDResponse[EcuIndex].SidA[1] != 0x00) )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  SID $0A reports Permanent DTC stored\n", GetEcuId(EcuIndex) );
				TestFailed = TRUE;
			}
		}
	}

	/* If a permanent DTC is supposed to be present (Test 8.6, 9.11, 9.15, 9.18) */
	else
	{
		/* Verify that SID A reports DTCs stored */
		NumDTCs = 0;
		for ( EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++ )
		{
			/* Print out all the DTCs */
			for (DataOffset = 0; DataOffset < gOBDResponse[EcuIndex].SidASize; DataOffset += 2)
			{
				if ((gOBDResponse[EcuIndex].SidA[DataOffset] != 0) ||
				    (gOBDResponse[EcuIndex].SidA[DataOffset + 1] != 0))
				{
					/* Process based on the type of DTC */
					switch (gOBDResponse[EcuIndex].SidA[DataOffset] & 0xC0)
					{
						case 0x00:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								 "ECU %X  Permanent DTC P%02X%02X detected\n",
							      GetEcuId (EcuIndex),
							      gOBDResponse[EcuIndex].SidA[DataOffset] & 0x3F,
							      gOBDResponse[EcuIndex].SidA[DataOffset + 1]);
						}
						break;
						case 0x40:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								 "ECU %X  Permanent DTC C%02X%02X detected\n",
							     GetEcuId (EcuIndex),
							     gOBDResponse[EcuIndex].SidA[DataOffset] & 0x3F,
							     gOBDResponse[EcuIndex].SidA[DataOffset + 1]);
						}
						break;
						case 0x80:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								 "ECU %X  Permanent DTC B%02X%02X detected\n",
							      GetEcuId (EcuIndex),
							      gOBDResponse[EcuIndex].SidA[DataOffset] & 0x3F,
							      gOBDResponse[EcuIndex].SidA[DataOffset + 1]);
						}
						break;
						case 0xC0:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								 "ECU %X  Permanent DTC U%02X%02X detected\n",
							     GetEcuId (EcuIndex),
							     gOBDResponse[EcuIndex].SidA[DataOffset] & 0x3F,
							     gOBDResponse[EcuIndex].SidA[DataOffset + 1]);
						}
						break;
					}
					NumDTCs++;
				}
			}
		}

		/* If no stored DTCs found, then fail */
		if ( NumDTCs == 0 )
		{
			if ( (gModelYear >= 2010) &&
			     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "All ECUs  SID $0A report no Permanent DTC stored\n");
				TestFailed = TRUE;
			}
			else
			{
				Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "All ECUs  SID $0A report no Permanent DTC stored\n");
			}
		}
	}

	/* if this is MY 2010 and beyond */
	if ( (gModelYear >= 2010) &&
	     (gUserInput.eComplianceType == US_OBDII || gUserInput.eComplianceType == HD_OBD) )
	{
		for ( EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++ )
		{
			if ( ( gOBDResponse[EcuIndex].Sid3Supported == TRUE ||
			       gOBDResponse[EcuIndex].Sid7Supported == TRUE ) &&
			     gOBDResponse[EcuIndex].SidASupported == FALSE )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  Supports SID $03 and SID $07 but not SID $0A\n", GetEcuId(EcuIndex) );
				TestFailed = TRUE;
			}
		}

	}

	if ( (TestFailed == TRUE) || (ulInit_FailureCount != GetFailureCount()) )
	{
		/* There could have been early/late responses that weren't treated as FAIL */
		return(FAIL);
	}
	else
	{
		return(PASS);
	}

}
