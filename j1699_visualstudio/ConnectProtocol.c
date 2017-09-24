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
** ConnectProtocol - Function to connect to a protocol
*******************************************************************************
*/
STATUS ConnectProtocol(void)
{
	PASSTHRU_MSG MaskMsg;
	PASSTHRU_MSG PatternMsg;
	PASSTHRU_MSG FlowMsg;
	PASSTHRU_MSG StartDiagMsg;
	PASSTHRU_MSG StartDiagRespMsg;
	SCONFIG_LIST ConfigList;
	SCONFIG ConfigParameter;
	SBYTE_ARRAY InputData;
	SBYTE_ARRAY OutputData;
	unsigned char InputBytes[8];
	unsigned long RetVal;
	unsigned long EcuIndex;
	unsigned long InitFlags;

	printf("ConnectProtocol was called.\n");
	/* Set the request delay, maximum response time and init flags according to the protocol */
	InitFlags = 0;
	if (gOBDList[gOBDListIndex].Protocol == ISO15765)
	{
		/* 50msec for ISO15765 */
		gOBDRequestDelay = 50;
		gOBDMaxResponseTimeMsecs = 50;
		gOBDMinResponseTimeMsecs = 0;		//06/07/04 - Added to support for min time verification
		InitFlags = (gOBDList[gOBDListIndex].InitFlags & CAN_29BIT_ID);
	}
	else if ((gOBDList[gOBDListIndex].Protocol == ISO9141) ||
	         (gOBDList[gOBDListIndex].Protocol == ISO14230))
	{
		/* Spec times for ISO9141 / ISO14230, required to verify P2 Min and P2 Max */
		gOBDRequestDelay = 300;
		gOBDMaxResponseTimeMsecs = 50;
		gOBDMinResponseTimeMsecs = 25;
		InitFlags = 0; //(gOBDList[gOBDListIndex].InitFlags & ISO9141_K_LINE_ONLY);
	}
	else
	{
		/* 100msec for all others */
		gOBDRequestDelay = 100;				/*Restored value to 100 ms for J1850*/
		gOBDMaxResponseTimeMsecs = 100;		/*Restored value to 100 ms for J1850*/
		gOBDMinResponseTimeMsecs = 0;		/*06/07/04 - Added to support for min time verification. */
	}

	/* Connect to protocol */
	//modified RetVal = PassThruConnect (gulDeviceID, gOBDList[gOBDListIndex].Protocol, InitFlags, gOBDList[gOBDListIndex].BaudRate, &gOBDList[gOBDListIndex].ChannelID);
	RetVal = STATUS_NOERROR;
	if ((RetVal != STATUS_NOERROR) && (gOBDList[gOBDListIndex].Protocol == ISO9141))
	{
		Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "PassThru Device unable to connect ISO9141 using K & L Line initialization.  Attempting to connect using K Line Only\n");
		/* try K-Line only */
		RetVal = PassThruConnect (gulDeviceID, gOBDList[gOBDListIndex].Protocol,
		                          gOBDList[gOBDListIndex].InitFlags & ISO9141_K_LINE_ONLY,
		                          gOBDList[gOBDListIndex].BaudRate,
		                          &gOBDList[gOBDListIndex].ChannelID);
	}

	if (RetVal != STATUS_NOERROR)
	{
		Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "%s returned %ld", "PassThruConnect", RetVal);
		return (FAIL);
	}
	printf("passed over Connect to protocol in ConnectProtocol.c .\n");

	/* Turn on LOOPBACK to see transmitted messages */
	ConfigList.NumOfParams = 1;
	ConfigList.ConfigPtr = &ConfigParameter;
	ConfigParameter.Parameter = LOOPBACK;
	ConfigParameter.Value = 1;
	RetVal = PassThruIoctl(gOBDList[gOBDListIndex].ChannelID, SET_CONFIG, &ConfigList, NULL);
	if (RetVal != STATUS_NOERROR)
	{
		Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "%s returned %ld", "PassThruIoctl(LOOPBACK)", RetVal);
		return(FAIL);
	}

	/* Setup the node IDs, functional message ID and filter(s) based on protocol */
	switch(gOBDList[gOBDListIndex].Protocol)
	{
		case ISO9141:
		{
			/* Set W4 */
			ConfigList.NumOfParams = 1;
			ConfigList.ConfigPtr = &ConfigParameter;
			ConfigParameter.Parameter = W4;
			ConfigParameter.Value = 40;
			RetVal = PassThruIoctl(gOBDList[gOBDListIndex].ChannelID, SET_CONFIG, &ConfigList, NULL);
			if (RetVal != STATUS_NOERROR)
			{
				Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "%s returned %ld", "PassThruIoctl(W4)", RetVal);
				return(FAIL);
			}

			/* Five baud initialization */
			InputData.NumOfBytes = 1;
			InputData.BytePtr = InputBytes;
			OutputData.NumOfBytes = sizeof(gOBDKeywords);
			OutputData.BytePtr = gOBDKeywords;
			InputBytes[0] = 0x33;
			RetVal = PassThruIoctl(gOBDList[gOBDListIndex].ChannelID, FIVE_BAUD_INIT, &InputData, &OutputData);
			if (RetVal == STATUS_NOERROR)
			{
				if ((gOBDKeywords[0] != 0) || (gOBDKeywords[1] != 0))
				{
					Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "Keyword 1 = $%02X   Keyword 2 = $%02X\n",
					     gOBDKeywords[0], gOBDKeywords[1]);

					if ( ( gOBDKeywords[0] == 0x94 ) && (gOBDKeywords[1] == 0x94 ) )
					{
						gOBDMinResponseTimeMsecs = 0;		//07/13/04 - ISO9141 KB 94 94 allow 0 ms response time.
					}
					else if ( !(( gOBDKeywords[0] == 0x08 ) && (gOBDKeywords[1] == 0x08 )) )
					{
						// these are not OBD Key Bytes
						Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "Non-OBD Key Bytes detected\n");
						return(FAIL);
					}
				}
			}
			else
			{
				// Do not log J2534 errors.  If there is no ISO9141 ECU,
				// it is proper for there to be no response.
				return(FAIL);
			}
		}
		/* ISO9141 case falls through to J1850VPW to setup same message filter */
		case J1850VPW:
		{
			/* Setup pass filter to read only OBD requests / responses */
			MaskMsg.ProtocolID = gOBDList[gOBDListIndex].Protocol;
			MaskMsg.TxFlags = 0;
			MaskMsg.DataSize = 3;
			MaskMsg.Data[0] = 0x00;
			MaskMsg.Data[1] = 0xFE;
			MaskMsg.Data[2] = 0x00;
			PatternMsg.ProtocolID = gOBDList[gOBDListIndex].Protocol;
			PatternMsg.TxFlags = 0;
			PatternMsg.DataSize = 3;
			PatternMsg.Data[0] = 0x00;
			PatternMsg.Data[1] = 0x6A;
			PatternMsg.Data[2] = 0x00;
			RetVal = PassThruStartMsgFilter(gOBDList[gOBDListIndex].ChannelID,
			PASS_FILTER, &MaskMsg, &PatternMsg, NULL, &gOBDList[gOBDListIndex].FilterID);
			if (RetVal != STATUS_NOERROR)
			{
				Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "%s returned %ld", "PassThruStartMsgFilter", RetVal);
				return(FAIL);
			}

			/* Setup tester present keep alive message using Mode 1 PID 0 */
			gTesterPresentMsg.ProtocolID = gOBDList[gOBDListIndex].Protocol;
			gTesterPresentMsg.TxFlags = 0;
			gTesterPresentMsg.DataSize = 5;
			gTesterPresentMsg.Data[0] = 0x68;
			gTesterPresentMsg.Data[1] = 0x6A;
			gTesterPresentMsg.Data[2] = TESTER_NODE_ADDRESS;
			gTesterPresentMsg.Data[3] = 0x01;
			gTesterPresentMsg.Data[4] = 0x00;
		}
		break;
		case J1850PWM:
		{
			/* Setup node ID */
			ConfigList.NumOfParams = 1;
			ConfigList.ConfigPtr = &ConfigParameter;
			ConfigParameter.Parameter = NODE_ADDRESS;
			ConfigParameter.Value = TESTER_NODE_ADDRESS;
			RetVal = PassThruIoctl(gOBDList[gOBDListIndex].ChannelID, SET_CONFIG, &ConfigList, NULL);
			if (RetVal != STATUS_NOERROR)
			{
				Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "%s returned %ld", "PassThruIoctl(SET_CONFIG/NODE_ADDRESS)", RetVal);
				return(FAIL);
			}

			/* Setup functional message lookup table */
			InputData.NumOfBytes = 1;
			InputData.BytePtr = InputBytes;
			InputBytes[0] = 0x6B;
			RetVal = PassThruIoctl(gOBDList[gOBDListIndex].ChannelID, ADD_TO_FUNCT_MSG_LOOKUP_TABLE, &InputData, NULL);
			if (RetVal != STATUS_NOERROR)
			{
				Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "%s returned %ld", "PassThruIoctl(ADD_TO_FUNC_MSG_LOOKUP_TABLE)", RetVal);
				return(FAIL);
			}

			/* Setup pass filter to read only OBD requests / responses */
			MaskMsg.ProtocolID = gOBDList[gOBDListIndex].Protocol;
			MaskMsg.TxFlags = 0;
			MaskMsg.DataSize = 3;
			MaskMsg.Data[0] = 0x00;
			MaskMsg.Data[1] = 0xFE;     /* Setup mask to see both request and response */
			MaskMsg.Data[2] = 0x00;
			PatternMsg.ProtocolID = gOBDList[gOBDListIndex].Protocol;
			PatternMsg.TxFlags = 0;
			PatternMsg.DataSize = 3;
			PatternMsg.Data[0] = 0x00;
			PatternMsg.Data[1] = 0x6A;
			PatternMsg.Data[2] = 0x00;
			RetVal = PassThruStartMsgFilter(gOBDList[gOBDListIndex].ChannelID,
			PASS_FILTER, &MaskMsg, &PatternMsg, NULL, &gOBDList[gOBDListIndex].FilterID);
			if (RetVal != STATUS_NOERROR)
			{
				Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "%s returned %ld", "PassThruStartMsgFilter", RetVal);
				return(FAIL);
			}

			/* Setup tester present keep alive message using Mode 1 PID 0 */
			gTesterPresentMsg.ProtocolID = gOBDList[gOBDListIndex].Protocol;
			gTesterPresentMsg.TxFlags = 0;
			gTesterPresentMsg.DataSize = 5;
			gTesterPresentMsg.Data[0] = 0x61;
			gTesterPresentMsg.Data[1] = 0x6A;
			gTesterPresentMsg.Data[2] = TESTER_NODE_ADDRESS;
			gTesterPresentMsg.Data[3] = 0x01;
			gTesterPresentMsg.Data[4] = 0x00;
		}
		break;
		case ISO14230:
		{
			/* Handle both five baud and fast initialization cases */
			if (gOBDList[gOBDListIndex].InitFlags == FAST_INIT)
			{
				/* Fast initialization */
				StartDiagMsg.ProtocolID = gOBDList[gOBDListIndex].Protocol;
				StartDiagMsg.TxFlags = 0;
				StartDiagMsg.DataSize = 4;
				StartDiagMsg.Data[0] = 0xC1;
				StartDiagMsg.Data[1] = 0x33;
				StartDiagMsg.Data[2] = TESTER_NODE_ADDRESS;
				StartDiagMsg.Data[3] = 0x81;

				StartDiagRespMsg.ProtocolID = gOBDList[gOBDListIndex].Protocol;
				StartDiagRespMsg.TxFlags = 0;
				StartDiagRespMsg.DataSize = 20;

				RetVal = PassThruIoctl(gOBDList[gOBDListIndex].ChannelID, FAST_INIT, &StartDiagMsg, &StartDiagRespMsg);
				if (RetVal == STATUS_NOERROR)
				{
					if	(
						 StartDiagRespMsg.Data[3] == 0xC1 &&
						 ((StartDiagRespMsg.Data[4] != 0) || (StartDiagRespMsg.Data[5] != 0))
						)
					{
						Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU $%02X   Keyword 1 = $%02X   Keyword 2 = $%02X\n",
						     StartDiagRespMsg.Data[2], StartDiagRespMsg.Data[4], StartDiagRespMsg.Data[5]);

						if (
						     (! ((StartDiagRespMsg.Data[4] == 0xE9) && (StartDiagRespMsg.Data[5] == 0x8F)) ) &&
						     (! ((StartDiagRespMsg.Data[4] == 0x6B) && (StartDiagRespMsg.Data[5] == 0x8F)) ) &&
						     (! ((StartDiagRespMsg.Data[4] == 0x6D) && (StartDiagRespMsg.Data[5] == 0x8F)) ) &&
						     (! ((StartDiagRespMsg.Data[4] == 0xEF) && (StartDiagRespMsg.Data[5] == 0x8F)) )
						   )
						{
							// these are not OBD Key Bytes
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "Non-OBD Key Bytes detected\n");
							return(FAIL);
						}
					}
					else
					{
						// this is not the right response or key bytes are 0
						return(FAIL);
					}
				}
				else
				{
					// Do not log J2534 errors.  If there is no ISO14230 ECU,
					// it is proper for there to be no response.
					return(FAIL);
				}
			}
			else
			{
				/* Set W4 */
				ConfigList.NumOfParams = 1;
				ConfigList.ConfigPtr = &ConfigParameter;
				ConfigParameter.Parameter = W4;
				ConfigParameter.Value = 40;
				RetVal = PassThruIoctl(gOBDList[gOBDListIndex].ChannelID, SET_CONFIG, &ConfigList, NULL);
				if (RetVal != STATUS_NOERROR)
				{
					Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "%s returned %ld", "PassThruIoctl(W4)", RetVal);
					return(FAIL);
				}

				/* Five baud initialization */
				InputData.NumOfBytes = 1;
				InputData.BytePtr = InputBytes;
				OutputData.NumOfBytes = sizeof(gOBDKeywords);
				OutputData.BytePtr = gOBDKeywords;
				InputBytes[0] = 0x33;
				RetVal = PassThruIoctl(gOBDList[gOBDListIndex].ChannelID, FIVE_BAUD_INIT, &InputData, &OutputData);
				if (RetVal == STATUS_NOERROR)
				{
					if ((gOBDKeywords[0] != 0) || (gOBDKeywords[1] != 0))
					{
						Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "Keyword 1 = $%02X   Keyword 2 = $%02X\n",
						gOBDKeywords[0], gOBDKeywords[1]);

						if (
						     (! ((gOBDKeywords[0] == 0xE9) && (gOBDKeywords[1] == 0x8F)) ) &&
						     (! ((gOBDKeywords[0] == 0x6B) && (gOBDKeywords[1] == 0x8F)) ) &&
						     (! ((gOBDKeywords[0] == 0x6D) && (gOBDKeywords[1] == 0x8F)) ) &&
						     (! ((gOBDKeywords[0] == 0xEF) && (gOBDKeywords[1] == 0x8F)) )
						   )
						{
							// these are not OBD Key Bytes
							Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "Non-OBD Key Bytes detected\n");
							return(FAIL);
						}
					}
					else
					{
						return(FAIL);
					}
				}
				else
				{
					// Do not log J2534 errors.  If there is no ISO14230 ECU,
					// it is proper for there to be no response.
					return(FAIL);
				}
			}

			/* Setup tester present keep alive message using Mode 1 PID 0 */
			gTesterPresentMsg.ProtocolID = gOBDList[gOBDListIndex].Protocol;
			gTesterPresentMsg.TxFlags = 0;
			gTesterPresentMsg.DataSize = 5;
			gTesterPresentMsg.Data[0] = 0xC2;
			gTesterPresentMsg.Data[1] = 0x33;
			gTesterPresentMsg.Data[2] = TESTER_NODE_ADDRESS;
			gTesterPresentMsg.Data[3] = 0x01;
			gTesterPresentMsg.Data[4] = 0x00;

			/* Setup pass filter to read only OBD requests / responses */
			MaskMsg.ProtocolID = gOBDList[gOBDListIndex].Protocol;
			MaskMsg.TxFlags = 0;
			MaskMsg.DataSize = 1;
			MaskMsg.Data[0] = 0x80;
			PatternMsg.ProtocolID = gOBDList[gOBDListIndex].Protocol;
			PatternMsg.TxFlags = 0;
			PatternMsg.DataSize = 1;
			PatternMsg.Data[0] = 0x80;
			RetVal = PassThruStartMsgFilter(gOBDList[gOBDListIndex].ChannelID,
			PASS_FILTER, &MaskMsg, &PatternMsg, NULL, &gOBDList[gOBDListIndex].FilterID);
			if (RetVal != STATUS_NOERROR)
			{
				Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "%s returned %ld", "PassThruStartMsgFilter", RetVal);
				return(FAIL);
			}
		}
		break;
		case ISO15765:
		{
			/* Handle both 11-bit and 29-bit ID cases */
			if (gOBDList[gOBDListIndex].InitFlags & CAN_29BIT_ID)
			{
				/* Setup ISO15765 flow control filters */
				MaskMsg.ProtocolID = gOBDList[gOBDListIndex].Protocol;
				MaskMsg.TxFlags = ISO15765_FRAME_PAD | CAN_29BIT_ID;
				MaskMsg.DataSize = 4;
				MaskMsg.Data[0] = 0xFF;
				MaskMsg.Data[1] = 0xFF;
				MaskMsg.Data[2] = 0xFF;
				MaskMsg.Data[3] = 0xFF;
				PatternMsg.ProtocolID = gOBDList[gOBDListIndex].Protocol;
				PatternMsg.TxFlags = ISO15765_FRAME_PAD | CAN_29BIT_ID;
				PatternMsg.DataSize = 4;
				PatternMsg.Data[0] = 0x18;
				PatternMsg.Data[1] = 0xDA;		/* DB->DA By Honda */
				PatternMsg.Data[2] = 0xF1;
				PatternMsg.Data[3] = 0x00;
				FlowMsg.ProtocolID = gOBDList[gOBDListIndex].Protocol;
				FlowMsg.TxFlags = ISO15765_FRAME_PAD | CAN_29BIT_ID;
				FlowMsg.DataSize = 4;
				FlowMsg.Data[0] = 0x18;
				FlowMsg.Data[1] = 0xDA;			/* DB->DA By Honda */
				FlowMsg.Data[2] = 0x00;
				FlowMsg.Data[3] = 0xF1;

				/* Setup a flow control filter for each ECU that responded to SID1 PID0 */
				for (EcuIndex = 0; EcuIndex < gOBDNumEcusCan; EcuIndex++)  /* By Honda */
				{
					PatternMsg.Data[3] = gOBDResponseTA[EcuIndex];         /* By Honda */
					FlowMsg.Data[2] = gOBDResponseTA[EcuIndex];            /* By Honda */
					RetVal = PassThruStartMsgFilter(gOBDList[gOBDListIndex].ChannelID,
					FLOW_CONTROL_FILTER,  &MaskMsg, &PatternMsg, &FlowMsg,
					&gOBDList[gOBDListIndex].FlowFilterID[EcuIndex]);
					if (RetVal != STATUS_NOERROR)
					{
						Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "%s returned %ld", "PassThruStartMsgFilter", RetVal);
						return(FAIL);
					}
				}
			}
			else
			{
				/* Setup ISO15765 flow control filters */
				MaskMsg.ProtocolID = gOBDList[gOBDListIndex].Protocol;
				MaskMsg.TxFlags = ISO15765_FRAME_PAD;
				MaskMsg.DataSize = 4;
				MaskMsg.Data[0] = 0xFF;
				MaskMsg.Data[1] = 0xFF;
				MaskMsg.Data[2] = 0xFF;
				MaskMsg.Data[3] = 0xFF;
				PatternMsg.ProtocolID = gOBDList[gOBDListIndex].Protocol;
				PatternMsg.TxFlags = ISO15765_FRAME_PAD;
				PatternMsg.DataSize = 4;
				PatternMsg.Data[0] = 0x00;
				PatternMsg.Data[1] = 0x00;
				PatternMsg.Data[2] = 0x07;
				PatternMsg.Data[3] = 0xE8;
				FlowMsg.ProtocolID = gOBDList[gOBDListIndex].Protocol;
				FlowMsg.TxFlags = ISO15765_FRAME_PAD;
				FlowMsg.DataSize = 4;
				FlowMsg.Data[0] = 0x00;
				FlowMsg.Data[1] = 0x00;
				FlowMsg.Data[2] = 0x07;
				FlowMsg.Data[3] = 0xE0;

				/* Setup flow control filters for all allowable 11-bit OBD ID values */
				for (FlowMsg.Data[3] = 0xE0; FlowMsg.Data[3] < 0xE8; FlowMsg.Data[3]++, PatternMsg.Data[3]++)
				{
					RetVal = PassThruStartMsgFilter(gOBDList[gOBDListIndex].ChannelID,
					FLOW_CONTROL_FILTER,  &MaskMsg, &PatternMsg, &FlowMsg,
					&gOBDList[gOBDListIndex].FlowFilterID[FlowMsg.Data[3] & 0x07]);
					if (RetVal != STATUS_NOERROR)
					{
						Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "%s returned %ld", "PassThruStartMsgFilter", RetVal);
						return(FAIL);
					}
				}
			}

			/* Setup tester present keep alive message using Mode 1 PID 0 */
			gTesterPresentMsg.ProtocolID = gOBDList[gOBDListIndex].Protocol;
			gTesterPresentMsg.DataSize = 6;
			if (gOBDList[gOBDListIndex].InitFlags & CAN_29BIT_ID)
			{
				gTesterPresentMsg.TxFlags = ISO15765_FRAME_PAD | CAN_29BIT_ID;
				gTesterPresentMsg.Data[0] = 0x18;
				gTesterPresentMsg.Data[1] = 0xDB;
				gTesterPresentMsg.Data[2] = 0x33;
				gTesterPresentMsg.Data[3] = TESTER_NODE_ADDRESS;
			}
			else
			{
				gTesterPresentMsg.TxFlags = ISO15765_FRAME_PAD;
				gTesterPresentMsg.Data[0] = 0x00;
				gTesterPresentMsg.Data[1] = 0x00;
				gTesterPresentMsg.Data[2] = 0x07;
				gTesterPresentMsg.Data[3] = 0xDF;
			}
			gTesterPresentMsg.Data[4] = 0x01;
			gTesterPresentMsg.Data[5] = 0x00;
		}
		break;
		default:
		{
			return(FAIL);
		}
	}
	return(PASS);
}

/*
*******************************************************************************
** StartPeriodicMsg - Function to start tester present message
*******************************************************************************
*/
static BOOL bPeriocicActive = FALSE;

STATUS StartPeriodicMsg (void)
{
	unsigned long RetVal;

	if ( gPeriodicMsgEnabled == FALSE && gOBDList[gOBDListIndex].Protocol == ISO15765)
	{
		return(PASS);
	}

#ifdef _DEBUG
	Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
	     "Enter StartPeriodicMsg - STATE %s, ID %x\n",
	     (bPeriocicActive == TRUE) ? "ON" : "OFF",
	     gOBDList[gOBDListIndex].TesterPresentID);

	if (bPeriocicActive == TRUE)
	{
		Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
		      "StartPeriodicMsg:: periodic already active - ID: %u\n",
		      gOBDList[gOBDListIndex].TesterPresentID);
	}
#endif



	RetVal = PassThruStartPeriodicMsg (gOBDList[gOBDListIndex].ChannelID,
	                                   &gTesterPresentMsg,
	                                   &gOBDList[gOBDListIndex].TesterPresentID,
	                                   OBD_TESTER_PRESENT_RATE);

	if (RetVal != STATUS_NOERROR)
	{
		Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "%s returned %ld", "PassThruStartPeriodicMsg", RetVal);
		return FAIL;
	}

	bPeriocicActive = TRUE;



#ifdef _DEBUG
	Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
	     "Leave StartPeriodicMsg - STATE %s, ID %x\n",
	     (bPeriocicActive == TRUE) ? "ON" : "OFF",
	     gOBDList[gOBDListIndex].TesterPresentID);
#endif



	return PASS;
}

/*
*******************************************************************************
** StopPeriodicMsg - Function to stop tester present message
*******************************************************************************
*/
STATUS StopPeriodicMsg (BOOL bLogError)
{
	unsigned long RetVal;

	if ( gPeriodicMsgEnabled == FALSE && gOBDList[gOBDListIndex].Protocol == ISO15765 )
	{
		return(PASS);
	}

#ifdef _DEBUG
	/* Leave this in for now... for debugging purposes */
	Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
	     "Enter StopPeriodicMsg - STATE %s, ID %x\n",
	     (bPeriocicActive == TRUE) ? "ON" : "OFF",
	     gOBDList[gOBDListIndex].TesterPresentID);

	if (bPeriocicActive == FALSE)
	{
		Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
		     "StopPeriodicMsg called, but periodics not active\n");
	}
#endif


	RetVal = PassThruIoctl (gOBDList[gOBDListIndex].ChannelID, CLEAR_PERIODIC_MSGS, NULL, NULL);
	if (RetVal != STATUS_NOERROR)
	{
		if (bLogError == TRUE)
		{
			Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "%s returned %ld", "PassThruIoctl(CLEAR_PERIODIC_MSGS)", RetVal);
			return FAIL;
		}
	}

	RetVal = PassThruIoctl (gOBDList[gOBDListIndex].ChannelID, CLEAR_TX_BUFFER, NULL, NULL);
	if (RetVal != STATUS_NOERROR)
	{
		if (bLogError == TRUE)
		{
			Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "%s returned %ld", "PassThruIoctl(CLEAR_TX_BUFFER)", RetVal);
			return FAIL;
		}
	}

	bPeriocicActive = FALSE;



#ifdef _DEBUG
	/* Leave this in for now... for debugging purposes */
	Log( INFORMATION, SCREENOUTPUTOFF, LOGOUTPUTON, NO_PROMPT,
	     "Leave StopPeriodicMsg - STATE %s, ID %x\n",
	     (bPeriocicActive == TRUE) ? "ON" : "OFF",
	     gOBDList[gOBDListIndex].TesterPresentID);
#endif



	return PASS;
}
