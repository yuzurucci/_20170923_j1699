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
#include <string.h>
#include <time.h>
#include <windows.h>
#include "j2534.h"
#include "j1699.h"

/*  Funtion prototypes  */
STATUS IDOBDProtocol (unsigned long *);
STATUS LogJ2534InterfaceVersion (void);
STATUS VerifyBatteryVoltage (void);
STATUS SearchTAfor29bit (void);  /* By Honda */

STATUS SaveConnectInfo (void);
STATUS VerifyConnectInfo (void);


/*-----------------------------------------------------------------------------
   Local data

   Note:  In order to reduce the global namespace, the following
          definition/declaration(s) are made local since they are used by
          local functions only.

          They can be move to the global namespace if needed.
------------------------------------------------------------------------------*/
typedef struct
{
	unsigned long Protocol;
	unsigned long NumECUs;
	unsigned long HeaderSize;
	unsigned char Header[OBD_MAX_ECUS][4];
} INITIAL_CONNECT_INFO;

static INITIAL_CONNECT_INFO gInitialConnect = {0}; // initialize all fields to zero

static unsigned char gFirstConnectFlag = TRUE;


/*
*******************************************************************************
** DetermineProtocol - Function to see what protocol is used to support OBD
*******************************************************************************
*/
STATUS DetermineProtocol(void)
{
	unsigned long fOBDFound = FALSE;

	/* Get the version information and log it */
	if (LogJ2534InterfaceVersion() != PASS)
	{
		/* If routine logged an error, pass error up! */
		return (FAIL);
	}

	/* Check if battery is within a resonable range */
	if ( VerifyBatteryVoltage () == FAIL)
	{
		return (FAIL);
	}

	/* If already connected to a protocol, disconnect first */
	if (gOBDDetermined == TRUE)
	{
		DisconnectProtocol();
		gOBDDetermined = FALSE;

		/* Close J2534 device */
		PassThruClose (gulDeviceID);

		/* Add a delay if disconnecting from ISO9141/ISO14230 to make sure the ECUs
		 * are out of any previous diagnostic session before determining protocol.
		 */
		if ((gOBDList[gOBDListIndex].Protocol == ISO9141) ||
		    (gOBDList[gOBDListIndex].Protocol == ISO14230))
		{
			Sleep (5000);
		}
	
		/* Open J2534 device */
		{
			unsigned long RetVal = PassThruOpen (NULL, &gulDeviceID);
			if (RetVal != STATUS_NOERROR)
			{
				Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "%s returned %ld", "PassThruOpen", RetVal);
				exit (FAIL);
			}
		}
	}

	/* Reset globals */
	gOBDListIndex = 0;
	gOBDNumEcus = 0;

	/* Initialize protocol list */
	InitProtocolList();

	gDetermineProtocol = 1;
	/* Connect to each protocol in the list and check if SID 1 PID 0 is supported */
	if (IDOBDProtocol (&fOBDFound) != FAIL)
	{

		/* Connect to the OBD protocol */
		if (fOBDFound == TRUE)
		{
			gOBDListIndex  = gOBDFoundIndex;
			gOBDDetermined = TRUE;
			if (ConnectProtocol() != PASS)
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "Connect to protocol failed\n");
				return (FAIL);
			}

			if ( (gUserInput.eComplianceType == US_OBDII ||
			      gUserInput.eComplianceType == HD_OBD) &&
			     gModelYear >= 2008 &&
			     (gOBDList[gOBDListIndex].ProtocolTag != ISO15765_I &&
			      gOBDList[gOBDListIndex].ProtocolTag != ISO15765_29_BIT_I) )
			{
#ifndef _DEBUG
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "Current protocol is not allowed for OBD\n");
				return (FAIL);
#endif
			}
			else if ( (gUserInput.eComplianceType == EOBD_WITH_IUMPR ||
			           gUserInput.eComplianceType == EOBD_NO_IUMPR   ||
			           gUserInput.eComplianceType == HD_EOBD) &&
			          gModelYear >= 2014 &&
			          (gOBDList[gOBDListIndex].ProtocolTag != ISO15765_I &&
			           gOBDList[gOBDListIndex].ProtocolTag != ISO15765_29_BIT_I &&
			           gOBDList[gOBDListIndex].ProtocolTag != ISO15765_250K_11_BIT_I &&
			           gOBDList[gOBDListIndex].ProtocolTag != ISO15765_250K_29_BIT_I)
			        )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "Current protocol is not allowed for EOBD\n");
				return (FAIL);
			}

			else if ( gUserInput.eComplianceType == HD_IOBD_NO_IUMPR &&
			          gOBDList[gOBDListIndex].ProtocolTag != ISO15765_I &&
			          gOBDList[gOBDListIndex].ProtocolTag != ISO15765_29_BIT_I &&
			          gOBDList[gOBDListIndex].ProtocolTag != ISO15765_250K_11_BIT_I &&
			          gOBDList[gOBDListIndex].ProtocolTag != ISO15765_250K_29_BIT_I
			        )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "Current protocol is not allowed for HD_IOBD\n");
				return (FAIL);
			}

			/* Start tester present to keep vehicle alive */
			if (StartPeriodicMsg() != PASS)
				/* If routine logged an error, pass error up! */
				return (FAIL);

			gDetermineProtocol = 0;
			return (PASS);
		}

	}
	gDetermineProtocol = 0;
	return (FAIL);
}

//*****************************************************************************
//
//	Function:	IDOBDProtocol
//
//	Purpose:	Purpose is to attempt to communicate on each specified OBDII
//				protocol channel and attempt to communicate.  If the current
//				link communicate then return return result.
//
//*****************************************************************************
//
//	DATE		MODIFICATION
//	10/21/03	Isolated routine into a function.
//	05/21/04	Per SAEJ1699 v11.5, removed check for support of PID$01.
//
//*****************************************************************************
STATUS IDOBDProtocol (unsigned long *pfOBDFound)
{
	SID_REQ     SidReq;
	STATUS      RetCode = PASS;
	STATUS      RetVal  = PASS;
	unsigned int usRequestCount = 0;


	/* Setup initial conditions */
	*pfOBDFound = FALSE;


	/* Connect to each protocol in the list and check if SID 1 PID 0 is supported */
	for (gOBDListIndex = 0; gOBDListIndex < gUserInput.MaxProtocols && RetCode == PASS; gOBDListIndex++)
	{
		RetVal = PASS;

		/* Connect to the protocol */
		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Checking for OBD on %s protocol\n", gOBDList[gOBDListIndex].Name);
		if ((gOBDList[gOBDListIndex].Protocol == ISO15765) &&       /* By Honda */
		    (gOBDList[gOBDListIndex].InitFlags & CAN_29BIT_ID))
		{
			RetVal = SearchTAfor29bit();
		}

		if ( RetVal == PASS && ConnectProtocol() == PASS )
		{
			do
			{
				/* Check if SID 1 PID 0 supported */
				SidReq.SID      = 1;
				SidReq.NumIds   = 1;
				SidReq.Ids[0]   = 0;

				RetVal = SidRequest(&SidReq, SID_REQ_NORMAL);

				if ( RetVal == RETRY )
				{
					if ( ++usRequestCount >= 6 )
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "Maximum number of Request retries\n");
						RetCode = FAIL;
					}
					else
					{
						Sleep(200);
					}
				}
				else if ( RetVal != FAIL)
				{
					/* We've found an OBD supported protocol */
					Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "OBD on %s protocol detected\n", gOBDList[gOBDListIndex].Name);
					if (*pfOBDFound == TRUE)
					{
						/* Check if protocol is the same */
						if (gOBDList[gOBDListIndex].ProtocolTag  != gOBDList[gOBDFoundIndex].ProtocolTag)
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "Multiple protocols supporting OBD detected\n");
							RetCode = FAIL;
						}
					}

					if ( RetCode != FAIL )
					{
						/* Save connect parameters (protocol, # ECUs, ECU IDs) on very first connect.
						 * Connect parameters should match on each subsequent connect.
						 */
						if (gFirstConnectFlag == TRUE)
						{
							SaveConnectInfo();
							gFirstConnectFlag = FALSE;
						}
						else
						{
							if (VerifyConnectInfo () == FAIL)
							{
								return (FAIL);
							}
						}

						/* Set the found flag and globals */
						*pfOBDFound = TRUE;
						gOBDFoundIndex = gOBDListIndex;
					}
				}

			}
			while ( RetVal == RETRY && usRequestCount < 6 );

		}

		/* Disconnect from the protocol */
		DisconnectProtocol ();


		/*
		** Add a delay for ISO9141/ISO14230 to make sure the ECUs are out of
		** any previous diagnostic session.
		*/
		if ((gOBDList[gOBDListIndex].Protocol == ISO9141) ||
		    (gOBDList[gOBDListIndex].Protocol == ISO14230))
		{
			Sleep (5000);
		}
	}

	return (RetCode);
}

//
//*****************************************************************************
//
//	Function:	LogJ2534InterfaceVersion
//
//	Purpose:	Purpose is to read J2534 interface version information
//				then log it to the current log file.
//
//*****************************************************************************
//
//	DATE		MODIFICATION
//	10/11/03	Isolated routine into a function.
//
//*****************************************************************************
STATUS LogJ2534InterfaceVersion (void)
{
	long        RetVal;
	static char FirmwareVersion[80];
	static char DllVersion[80];
	static char ApiVersion[80];

	STATUS result = PASS;

	if ( (RetVal = PassThruReadVersion (gulDeviceID, FirmwareVersion, DllVersion, ApiVersion)) != STATUS_NOERROR)
	{
		Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "%s returned %ld", "PassThruReadVersion", RetVal);
		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "J2534 Version information not available\n");
	}
	else
	{
		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Firmware Version: %s\n", FirmwareVersion);
		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "DLL Version: %s\n", DllVersion);
		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "API Version: %s\n", ApiVersion);
	}

	return (result);
}

//*****************************************************************************
//
//	Function:	VerifyBatteryVoltage
//
//	Purpose:	Purpose is to read current vehicle system voltage then
//				verify if the voltage is within specified limits.
//
//*****************************************************************************
//
//	DATE		MODIFICATION
//	10/11/03	Isolated routine into a function.
//
//*****************************************************************************
STATUS VerifyBatteryVoltage (void)
{
	long          RetVal;
	unsigned long BatteryVoltage;
	STATUS        RetCode = FAIL;

	/* Check if battery is within a resonable range */
	RetVal = PassThruIoctl (gulDeviceID, READ_VBATT, NULL, &BatteryVoltage);
	if (RetVal != STATUS_NOERROR)
	{
		/* account for non-compliant J2534-1 devices that don't support READ_VBATT */
		Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Battery voltage cannot be read - %s returned %ld\n", "PassThruIoctl(READ_VBATT)", RetVal);
		RetCode = PASS;
	}
	else if (BatteryVoltage < OBD_BATTERY_MINIMUM)
	{
		if ( (Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT,
		           "Battery voltage is LOW (%ld.%03ldV)\n",
		           (BatteryVoltage / 1000), (BatteryVoltage % 1000)) ) == 'Y' )
		{
			RetCode = PASS;
		}
	}
	else if (BatteryVoltage > OBD_BATTERY_MAXIMUM)
	{
		if ( (Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, YES_NO_ALL_PROMPT,
		           "Battery voltage is HIGH (%ld.%03ldV)\n",
		           (BatteryVoltage / 1000), (BatteryVoltage % 1000)) ) == 'Y' )
		{
			RetCode = PASS;
		}
	}
	else
	{
		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Battery = %ld.%03ldV\n",
		     (BatteryVoltage / 1000), (BatteryVoltage % 1000));
		RetCode = PASS;
	}

	return RetCode;
}


/* Additinal Function By Honda */
//*****************************************************************************
//
//	Function:	SearchTAfor29bit
//
//	Purpose:	Purpose is to research ECU's TargetAddress and numer of ECUs.
//
//*****************************************************************************
//
//	DATE		MODIFICATION
//	07/05/04	additional by Honda.
//
//*****************************************************************************
STATUS SearchTAfor29bit (void)
{
	unsigned long RetVal;         /* Return Value */
	unsigned long NumMsgs;        /* Number of Messages */
	PASSTHRU_MSG  MaskMsg;
	PASSTHRU_MSG  PatternMsg;
	PASSTHRU_MSG  TxMsg;
	PASSTHRU_MSG  RxMsg;

	STATUS        RetCode = FAIL;

	/* Data Initalize */
	gOBDNumEcusCan = 0;

	/* Connect to protocol */
	RetVal = PassThruConnect (gulDeviceID, CAN, CAN_29BIT_ID, gOBDList[gOBDListIndex].BaudRate, &gOBDList[gOBDListIndex].ChannelID);
	if (RetVal != STATUS_NOERROR)
	{
		Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "%s returned %ld", "PassThruConnect(29Bit)", RetVal);
		return (FAIL);
	}

	/* Clear the message filters */
	RetVal = PassThruIoctl (gOBDList[gOBDListIndex].ChannelID, CLEAR_MSG_FILTERS, NULL, NULL);
	if (RetVal != STATUS_NOERROR)
	{
		Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "%s returned %ld", "PassThruIoctl(CLEAR_MSG_FILTERS)", RetVal);
		return (FAIL);
	}

	/* Setup pass filter to read only OBD requests / responses */
	MaskMsg.ProtocolID    = CAN;
	MaskMsg.TxFlags       = CAN_29BIT_ID;
	MaskMsg.DataSize      = 4;
	MaskMsg.Data[0]       = 0xFF;
	MaskMsg.Data[1]       = 0xFF;
	MaskMsg.Data[2]       = 0xFF;
	MaskMsg.Data[3]       = 0x00;

	PatternMsg.ProtocolID = CAN;
	PatternMsg.TxFlags    = CAN_29BIT_ID;
	PatternMsg.DataSize   = 4;
	PatternMsg.Data[0]    = 0x18;
	PatternMsg.Data[1]    = 0xDA;
	PatternMsg.Data[2]    = 0xF1;
	PatternMsg.Data[3]    = 0x00;

	RetVal = PassThruStartMsgFilter (gOBDList[gOBDListIndex].ChannelID, PASS_FILTER, &MaskMsg, &PatternMsg, NULL, &gOBDList[gOBDListIndex].FilterID);
	if (RetVal != STATUS_NOERROR)
	{
		Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "%s returned %ld", "PassThruStartMsgFilter", RetVal);
		return (FAIL);
	}

	/* Clear the receive queue before sending request */
	RetVal = PassThruIoctl (gOBDList[gOBDListIndex].ChannelID, CLEAR_RX_BUFFER, NULL, NULL);
	if (RetVal != STATUS_NOERROR)
	{
		Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "%s returned %ld", "PassThruIoctl(CLEAR_RX_BUFFER)", RetVal);
		return (FAIL);
	}

	/* Send the request by CAN Format as ISO15765 29bits */
	TxMsg.ProtocolID = CAN;
	TxMsg.RxStatus   = 0;
	TxMsg.TxFlags    = CAN_29BIT_ID;
	TxMsg.Data[0]    = 0x18;
	TxMsg.Data[1]    = 0xDB;
	TxMsg.Data[2]    = 0x33;
	TxMsg.Data[3]    = 0xF1;
	TxMsg.Data[4]    = 0x02;	/* PCI Byte */
	TxMsg.Data[5]    = 0x01;	/* Service ID */
	TxMsg.Data[6]    = 0x00;	/* PID */
	TxMsg.Data[6]    = 0x00;	/* Padding Data */
	TxMsg.Data[7]    = 0x00;	/* Padding Data */
	TxMsg.Data[8]    = 0x00;	/* Padding Data */
	TxMsg.Data[9]    = 0x00;	/* Padding Data */
	TxMsg.Data[10]   = 0x00;	/* Padding Data */
	TxMsg.Data[11]   = 0x00;	/* Padding Data */
	TxMsg.DataSize   = 12;

	NumMsgs = 1;
	LogMsg( &TxMsg, LOG_REQ_MSG );
	RetVal  = PassThruWriteMsgs (gOBDList[gOBDListIndex].ChannelID, &TxMsg, &NumMsgs, 500);
	if (RetVal != STATUS_NOERROR)
	{
		/*  don't log timeouts during DetermineProtocol */
		if (!(gDetermineProtocol == 1 && RetVal == ERR_TIMEOUT))
		{
			Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "%s returned %ld", "PassThruWriteMsgs", RetVal);
		}
		return (FAIL);
	}

	NumMsgs = 1;
	RetVal  = PassThruReadMsgs (gOBDList[gOBDListIndex].ChannelID, &RxMsg, &NumMsgs, 500);
	if (RetVal != STATUS_NOERROR)
	{
		/*  don't fail timeouts during DetermineProtocol */
		if (!(gDetermineProtocol == 1 && (RetVal == ERR_TIMEOUT || RetVal == ERR_BUFFER_EMPTY)))
		{
			Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "%s returned %ld", "PassThruReadMsgs", RetVal);
		}
		return (FAIL);
	}

	/* All OBD ECUs should respond within 50msec */
	while (NumMsgs == 1)
	{
		LogMsg(&RxMsg, LOG_NORMAL_MSG);

		if ( (RxMsg.DataSize >= 5) && (RxMsg.Data[5] == 0x41) )
		{
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU#%d: Target Address %02X(hex)\n",gOBDNumEcusCan+1, RxMsg.Data[3]);
			if (gOBDNumEcusCan < OBD_MAX_ECUS)
			{
				gOBDResponseTA[gOBDNumEcusCan] = RxMsg.Data[3]; /* ECU TA */
				gOBDNumEcusCan++;       /* Increment number of positive respond ECUs */
				RetCode = PASS;
			}
			else
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "Too many ISO 15765-4 (29 bit) OBD ECU responses\n" );
				RetCode = FAIL;
			}
		}
		NumMsgs = 1;

		RetVal = PassThruReadMsgs (gOBDList[gOBDListIndex].ChannelID, &RxMsg, &NumMsgs, 500);
		if (RetVal != STATUS_NOERROR)
		{
			/*  don't fail timeouts during DetermineProtocol */
			if (!(gDetermineProtocol == 1 && (RetVal == ERR_TIMEOUT || RetVal == ERR_BUFFER_EMPTY)))
			{
				Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "%s returned %ld", "PassThruReadMsgs", RetVal);
			}
		}
	}

	if (RetCode == PASS)
	{
		/* 
		 * if 29 bit ISO 15765 was found then disconnect, but keep the filters.
		 * otherwise, the calling function will disconnect and clear filters.
		 */
		RetVal = PassThruDisconnect (gOBDList[gOBDListIndex].ChannelID);
		if (RetVal != STATUS_NOERROR)
		{
			Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "%s returned %ld", "PassThruDisconnect", RetVal);
		}
	}

	return RetCode;
}

//*****************************************************************************
//
//	Function:	SaveConnectInfo
//
//	Purpose:	Save connection information from initial connect for comparison
//              during subsequent connects.
//
//*****************************************************************************
//
//	DATE		MODIFICATION
//	1/5/05	initial
//
//*****************************************************************************
STATUS SaveConnectInfo (void)
{
	unsigned long EcuIndex;

	memset (&gInitialConnect, 0, sizeof(gInitialConnect));

	gInitialConnect.Protocol   = gOBDList[gOBDListIndex].Protocol;
	gInitialConnect.NumECUs    = gOBDNumEcusResp;
	gInitialConnect.HeaderSize = gOBDList[gOBDListIndex].HeaderSize;

	for (EcuIndex = 0; (EcuIndex < gOBDNumEcusResp) && (EcuIndex < OBD_MAX_ECUS); EcuIndex++)
	{
		memcpy (gInitialConnect.Header[EcuIndex], gOBDResponse[EcuIndex].Header,
		        gOBDList[gOBDListIndex].HeaderSize);
	}

	return (PASS);
}

//*****************************************************************************
//
//	Function:	VerifyConnectInfo
//
//	Purpose:	Compare connection information for current connect with initial connect.
//
//*****************************************************************************
//
//	DATE		MODIFICATION
//	1/5/05	initial
//
//*****************************************************************************
STATUS VerifyConnectInfo (void)
{
	unsigned long EcuIndex;

	STATUS RetCode = PASS;

	/* same protocol ? */
	if (gOBDList[gOBDListIndex].Protocol != gInitialConnect.Protocol)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Different protocol (%s) than initial connect.\n",
		     gOBDList[gOBDListIndex].Name );
		RetCode = FAIL;
	}

	/* same number of ECUs ? */
	if (gOBDNumEcusResp != gInitialConnect.NumECUs)
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Number of ECUs responding (%d) different than number of ECUs from initial connect (%d)\n",
		     gOBDNumEcusResp,
		     gInitialConnect.NumECUs);
		RetCode = FAIL;
	}

	/* check each ECU ID */
	for (EcuIndex=0; EcuIndex<gOBDNumEcusResp; EcuIndex++)
	{
		if (VerifyEcuID (gOBDResponse[EcuIndex].Header) == FAIL)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU ID %02X doesn't match IDs from initial connect\n",
			     GetEcuId(EcuIndex) );
			RetCode = FAIL;
		}
	}

	return ( RetCode );
}

//*****************************************************************************
//
//	Function:	VerifyEcuID
//
//	Purpose:	Compare ECU ID with those from initial connect.
//
//*****************************************************************************
//
//	DATE		MODIFICATION
//	1/5/05	initial
//
//*****************************************************************************
STATUS VerifyEcuID (unsigned char EcuId[])
{
	unsigned long Index;

	for (Index=0; Index<gInitialConnect.NumECUs; Index++)
	{
		if (memcmp (EcuId, gInitialConnect.Header[Index], gInitialConnect.HeaderSize) == 0)
			return (PASS);
	}

	return (FAIL);
}

//*****************************************************************************
//
//	Function:	GetEcuId
//
//	Purpose:	return ECU ID independent of protocol.
//
//*****************************************************************************
//
//	DATE		MODIFICATION
//	3/1/05	initial
//
//*****************************************************************************
unsigned int GetEcuId (unsigned int EcuIndex)
{
	unsigned int HeaderIndex, ID = 0;

	if (EcuIndex < gUserNumEcus)
	{
		if (gOBDList[gOBDListIndex].Protocol == ISO15765)
		{
			// ISO15765
			for (HeaderIndex=0; HeaderIndex < gOBDList[gOBDListIndex].HeaderSize; ++HeaderIndex)
			{
				ID = (ID << 8) | gOBDResponse[EcuIndex].Header[HeaderIndex];
			}
		}
		else
		{
			// J1850PWM
			// J1850VPW
			// ISO9141
			// ISO14230
			ID = gOBDResponse[EcuIndex].Header[2];
		}
	}

	return ID;
}