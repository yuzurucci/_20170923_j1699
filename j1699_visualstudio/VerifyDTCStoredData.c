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
** DATE         MODIFICATION
** 07/20/04     Integrate enhancements to allow ISO14230 verfication as
**              specified in SAEJ1699 Rev 11.6
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
** VerifyDTCStoredData - Function to verify SID3 DTC stored data
**
*******************************************************************************
*/
STATUS VerifyDTCStoredData(void)
{
	SID_REQ SidReq;
	unsigned long EcuIndex;
	unsigned long NumDTCs;
	unsigned long DataOffset;
	unsigned long ulInit_FailureCount = 0;

	ulInit_FailureCount = GetFailureCount();

	/* Request SID 3 data */
	SidReq.SID = 3;
	SidReq.NumIds = 0;
	if (SidRequest(&SidReq, SID_REQ_NORMAL) == FAIL)
	{
		/* If DTC stored or historical DTC or ISO15765 protocol, there must be a response */
		if ((gOBDDTCStored == TRUE) || (gOBDDTCHistorical == TRUE) ||
		    (gOBDList[gOBDListIndex].Protocol == ISO15765)         ||
		    (gOBDList[gOBDListIndex].Protocol == ISO14230))
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Sid $3 request failed\n");
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

	/* if a DTC is not supposed to be present (Test 5.12, 6.4, 10.8, 11.8)*/
	if ((gOBDDTCStored == FALSE) && (gOBDDTCHistorical == FALSE))
	{
		/* Verify that SID $3 reports no DTCs stored */
		for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
		{
			/* warn for ECUs which don't respond */
			if ( gOBDResponse[EcuIndex].Sid3Size == 0 )
			{
				Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  No response to SID $3 request\n", GetEcuId (EcuIndex) );
				continue;
			}

			/* If ISO15765 protocol, check that DTCs are $00 */
			if (gOBDList[gOBDListIndex].Protocol == ISO15765)
			{
				if ( (gOBDResponse[EcuIndex].Sid3[0] != 0x00) || ((gOBDResponse[EcuIndex].Sid3[1] != 0x00)) )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  SID $3 indicates DTC stored\n", GetEcuId (EcuIndex) );
					return(FAIL);
				}
			}
			/* If other protocol, check that DTCs are $00 */
			else
			{
				/* response should contain multiple of 6 data bytes */
				if (gOBDResponse[EcuIndex].Sid3Size % 6 != 0)
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  SID $3 response size error\n", GetEcuId (EcuIndex) );
					return(FAIL);
				}

				/* validation of all DTCs reported in response */
				for ( DataOffset = 0; DataOffset < (unsigned long)(gOBDResponse[EcuIndex].Sid3Size / 2); DataOffset++ )
				{
					if ( gOBDResponse[EcuIndex].Sid3[DataOffset*2] != 0x00 ||
					     gOBDResponse[EcuIndex].Sid3[DataOffset*2+1] != 0x00 )
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  SID $3 indicates DTC stored\n", GetEcuId (EcuIndex) );
						return(FAIL);
					}
				}
			}
		}
	}

	/* if a DTC is supposed to be present (Test 7.4, 8.4, 9.4)*/
	else
	{
		/* Verify that SID 3 reports DTCs stored */
		NumDTCs = 0;
		for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
		{
			/* warn for ECUs which don't respond */
			if ( gOBDResponse[EcuIndex].Sid3Size == 0 )
			{
				Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  No response to SID $3 request\n", GetEcuId (EcuIndex) );
				continue;
			}

			/* Print out all the DTCs */
			for (DataOffset = 0; DataOffset < gOBDResponse[EcuIndex].Sid3Size; DataOffset += 2)
			{
				if ((gOBDResponse[EcuIndex].Sid3[DataOffset] != 0) ||
				    (gOBDResponse[EcuIndex].Sid3[DataOffset + 1] != 0))
				{
					/* Process based on the type of DTC */
					switch (gOBDResponse[EcuIndex].Sid3[DataOffset] & 0xC0)
					{
						case 0x00:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								 "ECU %X  Stored DTC P%02X%02X detected\n",
							      GetEcuId (EcuIndex),
							      gOBDResponse[EcuIndex].Sid3[DataOffset] & 0x3F,
							      gOBDResponse[EcuIndex].Sid3[DataOffset + 1]);
						}
						break;
						case 0x40:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								 "ECU %X  Stored DTC C%02X%02X detected\n",
							     GetEcuId (EcuIndex),
							     gOBDResponse[EcuIndex].Sid3[DataOffset] & 0x3F,
							     gOBDResponse[EcuIndex].Sid3[DataOffset + 1]);
						}
						break;
						case 0x80:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								 "ECU %X  Stored DTC B%02X%02X detected\n",
							      GetEcuId (EcuIndex),
							      gOBDResponse[EcuIndex].Sid3[DataOffset] & 0x3F,
							      gOBDResponse[EcuIndex].Sid3[DataOffset + 1]);
						}
						break;
						case 0xC0:
						{
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								 "ECU %X  Stored DTC U%02X%02X detected\n",
							     GetEcuId (EcuIndex),
							     gOBDResponse[EcuIndex].Sid3[DataOffset] & 0x3F,
							     gOBDResponse[EcuIndex].Sid3[DataOffset + 1]);
						}
						break;
					}
					NumDTCs++;
				}
			}
		}

		/* If no stored DTCs found, then fail */
		if (NumDTCs == 0)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "No ECU indicates SID $3 DTC stored\n" );
			return(FAIL);
		}
	}

	/* Link active test to verify communication remained active for ALL protocols
	 */
	if (VerifyLinkActive() != PASS)
	{
		return(FAIL);
	}

	if ( ulInit_FailureCount != GetFailureCount() )
	{
		/* There could have been early/late responses that weren't treated as FAIL */
		return(FAIL);
	}
	else
	{
		return(PASS);
	}

}
