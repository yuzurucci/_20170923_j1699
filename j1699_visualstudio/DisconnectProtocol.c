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
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include "j2534.h"
#include "j1699.h"


/*
*******************************************************************************
** DisconnectProtocol - Function to disconnect a protocol
*******************************************************************************
*/
STATUS DisconnectProtocol(void)
{
	unsigned long RetVal;
	STATUS RetCode = PASS;

	/* Turn off all filters and periodic messages before disconnecting */
	//RetVal = PassThruIoctl (gOBDList[gOBDListIndex].ChannelID, CLEAR_MSG_FILTERS, NULL, NULL); //modified.
	RetVal = STATUS_NOERROR;
	if (RetVal != STATUS_NOERROR)
	{
		// If determining protocol,
		// don't print to the screen j2534 device error caused by incorrect number of ecus entered by user
		if ( RetVal == ERR_DEVICE_NOT_CONNECTED &&
		     gOBDDetermined == FALSE )
		{
			Log( J2534_FAILURE, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
			     "%s returned %ld", "PassThruDisconnect", RetVal);
		}
		else
		{
			Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "%s returned %ld", "PassThruIoctl(CLEAR_MSG_FILTERS)", RetVal);
		}
		RetCode = FAIL;
	}

	StopPeriodicMsg (FALSE);

	/* Disconnect protocol */
	RetVal = PassThruDisconnect(gOBDList[gOBDListIndex].ChannelID);
	if (RetVal != STATUS_NOERROR)
	{
		// If determining protocol,
		// don't print to the screen j2534 device error caused by incorrect number of ecus entered by user
		if ( (RetVal == ERR_DEVICE_NOT_CONNECTED || gOBDList[gOBDListIndex].ChannelID == 0 ) &&
		     gOBDDetermined == FALSE )
		{
			Log( J2534_FAILURE, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
			     "%s returned %ld", "PassThruDisconnect", RetVal);
		}
		else
		{
			Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "%s returned %ld", "PassThruDisconnect", RetVal);
		}
		RetCode = FAIL;
	}

	return(RetCode);
}

