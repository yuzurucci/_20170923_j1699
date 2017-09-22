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
* DATE      MODIFICATION
* 05/05/04  Added function per draft specification J1699 rev 11.4 test case
*           5.18.1.
********************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "j2534.h"
#include "j1699.h"


BOOL IsPidInRequest(SID_REQ SidReqList, unsigned char ubPidId);


/*
*******************************************************************************
** VerifyReverseGroupDiagnosticSupport -
** Function to verify SID1 group support (ISO15765 only)
*******************************************************************************
*/
STATUS VerifyReverseGroupDiagnosticSupport(void)
{
	unsigned long EcuIndex;
	unsigned long IdIndex;
	unsigned long IdBitIndex;
	unsigned short usByteIndex;
	unsigned short usPidCount = 0;				/* temp. count of PIDs for current ECU */
	unsigned short usNumberOfPIDsRequested = 0;
	unsigned short usDesiredResponseSize = 0;	/* hold expected response size for current ECU */
	unsigned short rgusEcuPidSize[6];			/* holds the size for each PID the ECU supports */
	unsigned short rgusSinglePidSize[8][6];		/* for each ECU, holds the size for each PID in the group */
	SID_REQ rgsEcuSpecificSidReq[8];			/* holds the last 6 PIDs for each ECU */
	SID_REQ SingleSidReq;						/* holds single requests */
	SID_REQ GroupSidReq;						/* holds the group requests */
	BOOL bTestFailed = FALSE;


	/* reverse order request started */
	for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
	{
		gReverseOrderState[EcuIndex] = REVERSE_ORDER_REQUESTED;
	}

	/* Request SID 1 support data as a group */
	GroupSidReq.SID = 1;
	GroupSidReq.NumIds = 2;
	GroupSidReq.Ids[0] = 0xE0;
	GroupSidReq.Ids[1] = 0xC0;
	if (SidRequest(&GroupSidReq, SID_REQ_ALLOW_NO_RESPONSE|SID_REQ_IGNORE_NO_RESPONSE) == FAIL)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $1 reverse group ($E0,$C0) support request failed\n");
		bTestFailed = TRUE;
	}

	/* Request SID 1 support data as a group */
	GroupSidReq.SID = 1;
	GroupSidReq.NumIds = 6;
	GroupSidReq.Ids[0] = 0xA0;
	GroupSidReq.Ids[1] = 0x80;
	GroupSidReq.Ids[2] = 0x60;
	GroupSidReq.Ids[3] = 0x40;
	GroupSidReq.Ids[4] = 0x20;
	GroupSidReq.Ids[5] = 0x00;
	if (SidRequest(&GroupSidReq, SID_REQ_NORMAL) == FAIL)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "SID $1 reverse group ($A0-$00) support request failed\n");
		bTestFailed = TRUE;
	}

	/* Verify that all SID 1 PID support data compares */
	for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
	{
		for (IdIndex = 0; IdIndex < 8; IdIndex++)
		{
			for (IdBitIndex = 0; IdBitIndex < 4; IdBitIndex++)
			{
				if ( gOBDResponse[EcuIndex].Sid1PidSupport[IdIndex].IDBits[IdBitIndex] !=
					 gOBDCompareResponse[EcuIndex].Sid1PidSupport[IdIndex].IDBits[IdBitIndex] )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  SID $1 PID $%02X Byte %d reverse/individual group support mismatch\n",
					     GetEcuId(EcuIndex),
					     gOBDCompareResponse[EcuIndex].Sid1PidSupport[IdIndex].FirstID,
					     IdBitIndex );
					bTestFailed = TRUE;
				}
			}
		}

		if (gOBDResponse[EcuIndex].Sid1PidSupportSize != gOBDCompareResponse[EcuIndex].Sid1PidSupportSize)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				 "ECU %X  SID $1 PID Supported PID reverse/individual group size mismatch\n",
					  GetEcuId(EcuIndex) );
			bTestFailed = TRUE;
		}
	}


	/* Restore the original response data for comparison */
	memcpy(&gOBDResponse[0], &gOBDCompareResponse[0], sizeof(gOBDResponse));

	/* find the list of the last six PIDs supported by each ECU */
	for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
	{
		for (IdIndex = 0xFF, usPidCount = 0; (IdIndex > 0) && (usPidCount < 6); IdIndex--)
		{
			/* Don't use PID supported PIDs */
			if (
				(IdIndex != 0x00) && (IdIndex != 0x20) && (IdIndex != 0x40) &&
				(IdIndex != 0x60) && (IdIndex != 0x80) && (IdIndex != 0xA0) &&
				(IdIndex != 0xC0) && (IdIndex != 0xE0)
				)
			{
				/* If PID is supported, add it to the list */
				if (IsSid1PidSupported (EcuIndex, IdIndex) == TRUE)
				{
					rgsEcuSpecificSidReq[EcuIndex].Ids[usPidCount] = (unsigned char)IdIndex;
					usPidCount++;
				}
			}
		}
		rgsEcuSpecificSidReq[EcuIndex].NumIds = (unsigned char)usPidCount;
	}


	/* combine the list of PIDs from each ECU into one list with at least one PID from each ECU */
	GroupSidReq.SID = 1;
	GroupSidReq.NumIds = 0;
	IdIndex = 0;
	EcuIndex = 0;

	do
	{
		/* add one PID at a time to the actual request */
		if (IdIndex < rgsEcuSpecificSidReq[EcuIndex].NumIds)
		{
			/* still some PIDs in the specific ECU list */
			if (IsPidInRequest(GroupSidReq, rgsEcuSpecificSidReq[EcuIndex].Ids[IdIndex]) == FALSE)
			{
				/* not already in the list, so add it */
				GroupSidReq.Ids[GroupSidReq.NumIds] = rgsEcuSpecificSidReq[EcuIndex].Ids[IdIndex];
				GroupSidReq.NumIds++;
			}
		}

		/* point to next ECU */
		EcuIndex++;

		if (EcuIndex >= gOBDNumEcus)
		{
			/* at the end of the ECU list, start over with next PID for each ECU */
			EcuIndex = 0;
			IdIndex++;
		}

	} while ( (GroupSidReq.NumIds < 6) && (IdIndex < 6) );


	if (GroupSidReq.NumIds != 0)
	{
		/* request the PIDs individually, to get the size information */
		for (usPidCount = 0; usPidCount < GroupSidReq.NumIds; usPidCount++)
		{
			SingleSidReq.SID = 1;
			SingleSidReq.NumIds = 1;
			SingleSidReq.Ids[0] = GroupSidReq.Ids[usPidCount];

			if (SidRequest(&SingleSidReq, SID_REQ_NORMAL) == FAIL)
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					 "SID $1 single PID $%02X request failed\n", SingleSidReq.Ids[0] );
				bTestFailed = TRUE;
			}

			/* save the response PID size */
			for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
			{
				rgusSinglePidSize[EcuIndex][usPidCount] = gOBDResponse[EcuIndex].Sid1PidSize;
			}
		}


		/* request the PIDs as a group and check the responses */
		if (SidRequest(&GroupSidReq, SID_REQ_NORMAL) == FAIL)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				 "SID $1 reverse group PID request failed\n");
			bTestFailed = TRUE;
		}
		else
		{
			for (EcuIndex = 0; EcuIndex < gOBDNumEcus; EcuIndex++)
			{
				/* rebuild ECU specific PID list, based on final group request */
				for (
					IdIndex = 0, usPidCount = 0, usDesiredResponseSize = 0;
					IdIndex < 6;
					IdIndex++
					)
				{
					if (IsSid1PidSupported (EcuIndex, GroupSidReq.Ids[IdIndex]) == TRUE)
					{
						rgsEcuSpecificSidReq[EcuIndex].Ids[usPidCount] = GroupSidReq.Ids[IdIndex];
						rgusEcuPidSize[usPidCount] = rgusSinglePidSize[EcuIndex][IdIndex];
						usDesiredResponseSize += rgusSinglePidSize[EcuIndex][IdIndex];
						usPidCount++;
					}
				}
				rgsEcuSpecificSidReq[EcuIndex].NumIds = (unsigned char)usPidCount;

				/* check the response size */
				if (gOBDResponse[EcuIndex].Sid1PidSize != usDesiredResponseSize)
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						 "ECU %X  SID $1 reverse group size mis-match\n", GetEcuId(EcuIndex) );
					bTestFailed = TRUE;
				}
				else if (gOBDResponse[EcuIndex].Sid1PidSize != 0)
				{
					/* size is good, check message content */
					usNumberOfPIDsRequested = rgsEcuSpecificSidReq[EcuIndex].NumIds;

					for (
							usByteIndex = 0;
							(usNumberOfPIDsRequested > 0) && (usByteIndex < gOBDResponse[EcuIndex].Sid1PidSize);
							usNumberOfPIDsRequested--
						) 
					{
						for (IdIndex = 0; IdIndex < rgsEcuSpecificSidReq[EcuIndex].NumIds; IdIndex++)
						{
							if (gOBDResponse[EcuIndex].Sid1Pid[usByteIndex] == rgsEcuSpecificSidReq[EcuIndex].Ids[IdIndex])
							{
								break;
							}
						}

						if (IdIndex < rgsEcuSpecificSidReq[EcuIndex].NumIds)
						{
							/* found the PID, get the offset to the next PID */
							usByteIndex += rgusEcuPidSize[IdIndex];
						}
						else
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								 "ECU %X  SID $1 reverse group PID mis-match\n", GetEcuId(EcuIndex) );
							bTestFailed = TRUE;
							break;
						}
					} /* end for (each PID) */
				} /* end else (size is good) */
			} /* end for (each ECU) */
		}
	}
				
	if ( bTestFailed == TRUE )
	{
		return(FAIL);
	}

	return(PASS);
}


/*
*******************************************************************************
** IsPidInRequest -
** Function to check if the designated PID is already in the request
*******************************************************************************
*/
BOOL IsPidInRequest(SID_REQ SidReqList, unsigned char ubPidId)
{
	unsigned short usPidIndex;


	for (usPidIndex = 0; usPidIndex < SidReqList.NumIds; usPidIndex++)
	{
		if (SidReqList.Ids[usPidIndex] == ubPidId)
		{
			return (TRUE);
		}
	}

	return (FALSE);
}
