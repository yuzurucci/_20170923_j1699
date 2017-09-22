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
** VerifyReservedServices -
** Function to verify that all ECUs respond correctly to reserved/unused services
*******************************************************************************
*/
STATUS VerifyReservedServices(void)
{
	BOOL bTestFailed = FALSE;
	unsigned char SidIndex;
	SID_REQ SidReq;

	if ( gOBDList[gOBDListIndex].Protocol != ISO15765 )
	{
		return (PASS);
	}

	/* For each SID */
	for ( SidIndex = 0x00; SidIndex <= 0x0F; SidIndex++ )
	{
		switch (SidIndex)
		{
			case 0x00:
			case 0x0B:
			case 0x0C:
			case 0x0D:
			case 0x0E:
			case 0x0F:
			{
				SidReq.SID = SidIndex;
				SidReq.NumIds = 0;
				if ( SidRequest( &SidReq, SID_REQ_NORMAL|SID_REQ_IGNORE_NO_RESPONSE ) != FAIL )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "Sid $%02X response\n", SidIndex );
					bTestFailed = TRUE;
					continue;
				}
			}
			break;
		}
	}

	if ( VerifyLinkActive() != PASS )
	{
		bTestFailed = TRUE;
	}

	if ( bTestFailed == TRUE )
	{
		return(FAIL);
	}

	return(PASS);
}