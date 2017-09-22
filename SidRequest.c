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
STATUS SetupRequestMSG    (SID_REQ *, PASSTHRU_MSG *);
STATUS ProcessLegacyMsg   (SID_REQ *, PASSTHRU_MSG *, unsigned long *, unsigned long *, unsigned long *, unsigned long *, unsigned long	*, unsigned long);
STATUS ProcessISO15765Msg (SID_REQ *, PASSTHRU_MSG *, unsigned long *, unsigned long *, unsigned long *, unsigned long *, unsigned long *);

/* Wait / Pending data */
static unsigned long ulEcuWaitFlags = 0;            /* up to 32 ECUs */
static unsigned long ulResponsePendingDelay = 0;
static unsigned char bPadErrorPermanent = FALSE;

/*
*******************************************************************************
**	SidRequest - Function to request a service ID
**
**	Returns:    RETRY - NRC=$21 durning initialization of ISO 15765
**	            ERRORS - One or more correct early/late responses
**	            FAIL - No response, wrong response, or catestrophic error
*	            PASS - One or more correct responses, all on time
*******************************************************************************
*/
STATUS SidRequest(SID_REQ *SidReq, unsigned long Flags)
{
	PASSTHRU_MSG  RxMsg;
	PASSTHRU_MSG  TxMsg;
	unsigned long NumMsgs;
	unsigned long RetVal;
	unsigned long StartTimeMsecs;
	unsigned long NumResponses;
	unsigned long NumFirstFrames;
	unsigned long TxTimestamp;
	unsigned long ExtendResponseTimeMsecs;
	unsigned long SOMTimestamp;
	unsigned char fFirstResponse;
	unsigned long ulResponseTimeoutMsecs;
	char bString[MAX_LOG_STRING_SIZE];
	unsigned long EcuTimingIndex;

	STATUS eReturnCode = PASS;    // saves the return code from function calls


	/* Initialize local variables */
	ExtendResponseTimeMsecs = 0;
	TxTimestamp             = 0;
	SOMTimestamp            = 0;

	/* Reset wait variables */
	ulEcuWaitFlags          = 0;
	ulResponsePendingDelay  = 0;

	/* Initialize ECU variables */
	for ( EcuTimingIndex = 0; EcuTimingIndex < OBD_MAX_ECUS; EcuTimingIndex++ )
	{
		gEcuTimingData[EcuTimingIndex].ExtendResponseTimeMsecs = 0;
		gEcuTimingData[EcuTimingIndex].ResponsePendingDelay    = 0;
		gEcuTimingData[EcuTimingIndex].NAKReceived             = FALSE;
	}

	/* if gSuspendLogOutput is true, then clear buffer */
	if (gSuspendLogOutput == TRUE)
	{
		SaveTransactionStart();
	}

	/* If not burst test, stop tester present message and delay
	** before each request to avoid exceeding minimum OBD request timing */
	if ( ( Flags & SID_REQ_NO_PERIODIC_DISABLE ) == 0)
	{
		/* Stop the tester present message before each request */
		if (StopPeriodicMsg (TRUE) == FAIL)
		{
			return(FAIL);
		}

		gOBDList[gOBDListIndex].TesterPresentID = -1;

		Sleep (gOBDRequestDelay);
	}

	/* Setup request message based on the protocol */
	if ( SetupRequestMSG( SidReq, &TxMsg ) != PASS )
	{
		return(FAIL);
	}

	/* Clear the transmit queue before sending request */
	RetVal = PassThruIoctl (gOBDList[gOBDListIndex].ChannelID, CLEAR_TX_BUFFER, NULL, NULL);

	if (RetVal != STATUS_NOERROR)
	{
		Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "%s returned %ld", "PassThruIoctl(CLEAR_TX_BUFFER)", RetVal);
		return(FAIL);
	}

	/* Clear the receive queue before sending request */
	RetVal = PassThruIoctl (gOBDList[gOBDListIndex].ChannelID, CLEAR_RX_BUFFER, NULL, NULL);

	if (RetVal != STATUS_NOERROR)
	{
		Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "%s returned %ld", "PassThruIoctl(CLEAR_RX_BUFFER)", RetVal);
		return(FAIL);
	}

	/* Send the request */
	NumMsgs = 1;
	RetVal  = PassThruWriteMsgs (gOBDList[gOBDListIndex].ChannelID, &TxMsg, &NumMsgs, 500);

	if (RetVal != STATUS_NOERROR)
	{
		/*  don't log timeouts during DetermineProtocol */
		if (!(gDetermineProtocol == 1 && RetVal == ERR_TIMEOUT))
		{
			Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "%s returned %ld", "PassThruWriteMsgs", RetVal);
			return(FAIL);
		}
	}

	/* Log the request message to compare to what is sent */
	LogMsg( &TxMsg, LOG_REQ_MSG );

	/* Reset the response data buffers */
	if ( SidResetResponseData( &TxMsg ) != PASS )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Cannot reset SID response data\n");
		return(FAIL);
	}

	/*
	** Read the response(s) with a timeout of twice what is allowed so
	** we can see late responses.
	*/
	NumResponses    = 0;
	NumFirstFrames  = 0;
	fFirstResponse  = TRUE;
	StartTimeMsecs  = GetTickCount();

	do
	{
		if ( fFirstResponse == TRUE )
		{
			fFirstResponse = FALSE;
			ulResponseTimeoutMsecs =  5 * gOBDMaxResponseTimeMsecs;
			sprintf ( bString, "In SidRequest - Initial PassThruReadMsgs" );
		}
		else
		{
			ulResponseTimeoutMsecs =  (5 * gOBDMaxResponseTimeMsecs) + ExtendResponseTimeMsecs;
			sprintf ( bString, "In SidRequest - Loop PassThruReadMsgs" );

		}

		/* Read the next response */
		NumMsgs = 1;
		RetVal = PassThruReadMsgs( gOBDList[gOBDListIndex].ChannelID,
		                           &RxMsg,
		                           &NumMsgs,
		                           ulResponseTimeoutMsecs );

		if ( (RetVal != STATUS_NOERROR) &&
		     (RetVal != ERR_BUFFER_EMPTY) &&
		     (RetVal != ERR_NO_FLOW_CONTROL) )
		{
			/* Log undesirable returns */
			Log( J2534_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "%s returned %ld", bString, RetVal);
			eReturnCode |= FAIL;
		}

		/* If a message was received, process it */
		if ( NumMsgs == 1 )
		{
			/* Save all read messages in the log file */
			LogMsg(&RxMsg, LOG_NORMAL_MSG);

			/* Process response based on protocol */
			switch (gOBDList[gOBDListIndex].Protocol)
			{
				case J1850VPW:
				case J1850PWM:
				case ISO9141:
				case ISO14230:
				{
					eReturnCode |= ProcessLegacyMsg ( SidReq,
					                                  &RxMsg,
					                                  &StartTimeMsecs,
					                                  &NumResponses,
					                                  &TxTimestamp,
					                                  &ExtendResponseTimeMsecs,
					                                  &SOMTimestamp,
					                                  Flags );
				}
				break;
				case ISO15765:
				{
					eReturnCode |= ( ProcessISO15765Msg(
					                                     SidReq,
					                                     &RxMsg,
					                                     &StartTimeMsecs,
					                                     &NumResponses,
					                                     &NumFirstFrames,
					                                     &TxTimestamp,
					                                     &ExtendResponseTimeMsecs) );
				}
				break;
				default:
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "Invalid protocol specified for response.\n");
					return (FAIL);
				}
			}
		}

		/* If all expected ECUs responded and flag is set, don't wait for timeout */
		/* NOTE: This mechanism is only good for single message response per ECU */
		if ( ( NumResponses >= gOBDNumEcus )	&&
		     ( Flags & SID_REQ_RETURN_AFTER_ALL_RESPONSES ) )
		{
			break;
		}
	}
	while (( NumMsgs == 1 ) &&
	        ( GetTickCount() - StartTimeMsecs ) < ( ( 5 * gOBDMaxResponseTimeMsecs ) + ExtendResponseTimeMsecs ) );  /*extend response time: the multiplier is changed to 5 from 3*/

	/* Restart the periodic message if protocol determined and not in burst test */
	if ( ( gOBDDetermined == TRUE )	&&
	     ( (Flags & SID_REQ_NO_PERIODIC_DISABLE ) == 0 ) )
	{
		if ( gOBDList[gOBDListIndex].TesterPresentID == -1 )
		{
			if (StartPeriodicMsg () != PASS)
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "Problems starting periodic messages.\n" );
				eReturnCode |= FAIL;
			}
		}
	}

	/* Preserve the total number of ECUs responding and
	*  allow calling routine to access information.
	*/
	gOBDNumEcusResp = NumResponses;

	/* Return code based on whether this protocol supports OBD */
	if (NumResponses > 0)
	{
		if ( gOBDDetermined == FALSE && ((eReturnCode & RETRY) != RETRY) )
		{
			gOBDNumEcus = NumResponses;
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "%d OBD ECU(s) found\n", gOBDNumEcus);

			/* Check if number of OBD-II ECUs entered by user matches the number detected */
			if (gOBDNumEcus != gUserNumEcus)
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "Number of OBD-II ECUs detected does not match the number entered.\n");
				eReturnCode |= FAIL;
			}

		}

		// If there weren't any errors since sending the request, pass
		if ( (eReturnCode & FAIL) != FAIL )
		{
			if ( (eReturnCode & RETRY) == RETRY )
			{
				return(RETRY);
			}
			else if ( (eReturnCode & ERRORS) == ERRORS )
			{
				return(ERRORS);
			}
			else
			{
				return(PASS);
			}
		}
	}

	// If there was at least one failure since sending the request, return FAIL
	if ( (eReturnCode & FAIL) == FAIL )
	{
		return(FAIL);
	}

	if (Flags & SID_REQ_ALLOW_NO_RESPONSE)
	{
		/* calling function must determine any actual responses */
		return PASS;
	}

	if ( (gOBDDetermined == TRUE ) && ( (Flags & SID_REQ_IGNORE_NO_RESPONSE) != SID_REQ_IGNORE_NO_RESPONSE ) )
	{
		Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "No response to OBD request\n");
	}
	return (FAIL);
}


//*****************************************************************************
//
//	Function:   ProcessLegacyMsg
//
//	Purpose:    Purpose is to process response based on connection to
//	            J1850VPW, J1850PWM, ISO9141, or ISO14230 .
//
//	Returns:    ERRORS, FAIL, or PASS
//
//	NOTE:       Considered consolidating ProcessISO15765Msg with
//	            ProcessLegacyMsg.  The many unique difference would
//	            complicate the 'streamling' of the logic.  Hence the
//	            routines have intentionally remained separate.
//
//*****************************************************************************
//
//	DATE        MODIFICATION
//	11/01/03    Isolated routine into a function.
//	07/16/04    Enhanced to turn on tester present in the event a timing error
//	            was flagged.
//
//*****************************************************************************
STATUS ProcessLegacyMsg( SID_REQ       *pSidReq,
                         PASSTHRU_MSG  *pRxMsg,
                         unsigned long *ulStartTimeMsecs,
                         unsigned long *pulNumResponses,
                         unsigned long *ulTxTimestamp,
                         unsigned long *ulExtendResponseTimeMsecs,
                         unsigned long *ulSOMTimestamp,
                         unsigned long Flags )
{
	unsigned long ulResponseTimeMsecs;
	unsigned long ulResponseDelta = 0;
	unsigned long EcuTimingIndex;
	unsigned long EcuIndex;
	unsigned long HeaderSize;
	unsigned long EcuId = 0;
	unsigned short i;
	STATUS eReturnCode = PASS;         // saves the return code from function calls
	BOOL RequestMsgLpbk = FALSE;

	/* Assume the header is three bytes, we verify this later (can't check Indications) */
	HeaderSize = 3;

	/* create the ECU ID for later use */
	for (i = 0; i < HeaderSize; i++)
	{
		EcuId = (EcuId << 8) + (pRxMsg->Data[i]);
	}

	if	(pRxMsg->RxStatus & TX_MSG_TYPE)
	{
		RequestMsgLpbk = TRUE;
	}
	else
	{	/* Find this ECU's Timing Structure */
		for ( EcuTimingIndex = 0; EcuTimingIndex < OBD_MAX_ECUS; EcuTimingIndex++ )
		{
			if ( gEcuTimingData[EcuTimingIndex].EcuId == EcuId || gEcuTimingData[EcuTimingIndex].EcuId == 0x00)
			{
				if ( gEcuTimingData[EcuTimingIndex].EcuId == 0x00 )
				{
					gEcuTimingData[EcuTimingIndex].EcuId = EcuId;
				}
				break;
			}
		}
	}

	/* Check for echoed request message */
	if (pRxMsg->RxStatus & TX_MSG_TYPE)
	{
		if ( (pRxMsg->DataSize >= HeaderSize+1) && (pRxMsg->Data[HeaderSize] == pSidReq->SID) )
		{
			/* Save the timestamp */
			*ulTxTimestamp = pRxMsg->Timestamp;
		}
	}

	/* Get Start of Message timestamp to compute response time (ISO9141 & ISO14230 only) */
	else if (pRxMsg->RxStatus & START_OF_MESSAGE)
	{
		*ulSOMTimestamp = pRxMsg->Timestamp;
	}

	/* Check for NAK response message */
	else if ( (pRxMsg->DataSize >= HeaderSize+3)  &&
	          (pRxMsg->Data[HeaderSize] == NAK) &&
	          (pRxMsg->Data[HeaderSize+1] == pSidReq->SID) )
	{
		/* Verify header is 3-byte for ISO14230 */
		if ( (gOBDList[gOBDListIndex].Protocol == ISO14230) &&  // if ISO14230 AND
		     ( (pRxMsg->Data[0] & 0x80) != 0x80 ||            // NOT functional/physical address OR
		     ( ((pRxMsg->Data[0] & 0x3F) < 0x01) && (pRxMsg->Data[0] & 0x3F) > 0x07)) )  // bad size
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ISO14230 NAK response had a bad Format byte\n" );
			return (FAIL);
			/*
			 * if the software were to continue on, HeaderSize calculations would be needed
			 * here, in SidSaveResponseData, and in LookupEcuIndex
			 */
		}

		if (gOBDList[gOBDListIndex].Protocol != ISO9141 && gOBDList[gOBDListIndex].Protocol != ISO14230)
		{
			*ulSOMTimestamp = pRxMsg->Timestamp;    /* for J1850xxx */
		}

		ulResponseDelta = *ulSOMTimestamp - *ulTxTimestamp;
		ulResponseTimeMsecs = ulResponseDelta / 1000;

		/* same timestamp logic used to get EOM, SOM and DELTA */

			/* Tally up the response statistics */
		gEcuTimingData[EcuTimingIndex].AggregateResponseTimeMsecs += ulResponseTimeMsecs;
		gEcuTimingData[EcuTimingIndex].AggregateResponses++;
		if ( ulResponseTimeMsecs > gEcuTimingData[EcuTimingIndex].LongestResponsesTime)
		{
			gEcuTimingData[EcuTimingIndex].LongestResponsesTime = ulResponseTimeMsecs;
		}

		/* Display for non-class 2 */
		if (gOBDList[gOBDListIndex].Protocol == ISO9141 ||
		    gOBDList[gOBDListIndex].Protocol == ISO14230)
		{
			/* 6/7/04 - Print time stamps to log file for visual time verification. */
			Log( NETWORK, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "EOM: %lu usec, SOM: %lu usec, DELTA: %lu usec\n",
			     *ulTxTimestamp, *ulSOMTimestamp, ulResponseDelta);
		}

		/* Check if response was too soon */
		if ( ulResponseDelta < (gOBDMinResponseTimeMsecs * 1000) )
		{
			if ( HeaderSize >= 3 )
			{
				Log( ERROR_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %02X  OBD Response was sooner than allowed (< %dmsec)\n",
				     pRxMsg->Data[2],
				     gOBDMinResponseTimeMsecs );
				eReturnCode = ERRORS;
			}
			else
			{
				Log( ERROR_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "OBD Response was sooner than allowed (< %dmsec)\n",
				     gOBDMinResponseTimeMsecs );
				eReturnCode = ERRORS;
			}

			gEcuTimingData[EcuTimingIndex].RespTimeOutofRange++;
			gEcuTimingData[EcuTimingIndex].RespTimeTooSoon++;

		}

		/* Check if response was late */
		if ( ulResponseDelta > ((gOBDMaxResponseTimeMsecs + *ulExtendResponseTimeMsecs) * 1000) )
		{
			if ( HeaderSize >= 3 )
			{
				Log( ERROR_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %02X  OBD Response was later than allowed (> %dmsec)\n",
				     pRxMsg->Data[2],
				     (gOBDMaxResponseTimeMsecs + *ulExtendResponseTimeMsecs) );
				eReturnCode = ERRORS;
			}
			else
			{
				Log( ERROR_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "OBD Response was later than allowed (> %dmsec)\n",
				     (gOBDMaxResponseTimeMsecs + *ulExtendResponseTimeMsecs) );
				eReturnCode = ERRORS;
			}

			gEcuTimingData[EcuTimingIndex].RespTimeOutofRange++;
			gEcuTimingData[EcuTimingIndex].RespTimeTooLate++;

		}

		/* If response was not late, reset the start time */
		*ulStartTimeMsecs = GetTickCount();
		*ulTxTimestamp = pRxMsg->Timestamp;

		/* Save the response information */
		if (SidSaveResponseData (pRxMsg, pSidReq, pulNumResponses) != PASS)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Cannot save SID response data\n");
			return (FAIL);
		}

		/*check the kind of response received for the vehicle*/
		if (pRxMsg->Data[HeaderSize+2] == NAK_NOT_SUPPORTED /*0x11*/)
		{
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Service $%02X not supported.\n", pRxMsg->Data[HeaderSize+1]);
		}
		else if (pRxMsg->Data[HeaderSize+2] == NAK_INVALID_FORMAT /*0x12*/)
		{
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Service $%02X supported. Unsupported PID request.\n", pRxMsg->Data[HeaderSize+1]);
		}
		else if (pRxMsg->Data[HeaderSize+2] == NAK_SEQUENCE_ERROR /*0x22*/)
		{
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Service $%02X supported. Conditions not correct.\n", pRxMsg->Data[HeaderSize+1]);
		}

		/* If response pending, extend the wait time to worst case P3 max */
		if (pRxMsg->Data[HeaderSize+2] == NAK_RESPONSE_PENDING)
		{
			/* Don't allow extended response time for infotype 6 */
			/* No group requests for legacy protocols */
			if (pSidReq->SID != 9 && pSidReq->Ids[0] != 6)
			{
				Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "Service $%02X supported. Response Pending.\n", pRxMsg->Data[HeaderSize+1]);
				*ulExtendResponseTimeMsecs = 3000;

				if (LookupEcuIndex (pRxMsg, &EcuIndex) == PASS)
				{
					gOBDResponse[EcuIndex].bResponseReceived = FALSE; // allow ECU to respond again
				}
				else
				{
					return (FAIL);
				}
			}
		}
	}

	/* Check for response message */
	else if ( pRxMsg->DataSize >= HeaderSize+1 )
	{
		/* Verify header is 3-byte for ISO14230 */
		if ( (gOBDList[gOBDListIndex].Protocol == ISO14230) &&  // if ISO14230 AND
		     ( (pRxMsg->Data[0] & 0x80) != 0x80 ||            // NOT functional/physical address OR
		     ( ((pRxMsg->Data[0] & 0x3F) < 0x01) && (pRxMsg->Data[0] & 0x3F) > 0x07)) )  // bad size
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ISO14230 response had a bad Format byte\n" );
			return (FAIL);
			/*
			 * if the software were to continue on, HeaderSize calculations would be needed
			 * here, in SidSaveResponseData, and in LookupEcuIndex
			 */
		}

		if (gOBDList[gOBDListIndex].Protocol != ISO9141 && gOBDList[gOBDListIndex].Protocol != ISO14230)
		{
			*ulSOMTimestamp = pRxMsg->Timestamp;    /* for J1850xxx */
		}

		ulResponseDelta = *ulSOMTimestamp - *ulTxTimestamp;
		ulResponseTimeMsecs = ulResponseDelta / 1000;

		/* Tally up the response statistics */
		gEcuTimingData[EcuTimingIndex].AggregateResponseTimeMsecs += ulResponseTimeMsecs;
		gEcuTimingData[EcuTimingIndex].AggregateResponses++;
		if ( ulResponseTimeMsecs > gEcuTimingData[EcuTimingIndex].LongestResponsesTime)
		{
			gEcuTimingData[EcuTimingIndex].LongestResponsesTime = ulResponseTimeMsecs;
		}

		/* Display for non-class 2 */
		if (gOBDList[gOBDListIndex].Protocol == ISO9141 ||
		    gOBDList[gOBDListIndex].Protocol == ISO14230)
		{
			/* 6/7/04 - Print time stamps to log file for visual time verification. */
			Log( NETWORK, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "EOM: %lu usec, SOM: %lu usec, DELTA: %lu usec\n",
			     *ulTxTimestamp, *ulSOMTimestamp, ulResponseDelta);
		}

		/* Check if response was too soon */
		/* Since under ISO9141 or ISO14230 we have saved off the ending timestamp
		** as ulTxTimestamp, we can now accurately calculate the difference between the
		** end of the last message to the beginning of this response.
		*/
		if ( (ulResponseDelta / 1000) < gOBDMinResponseTimeMsecs)
		{
			/* Exceeded maximum response time */
			if ( HeaderSize >= 3 )
			{
				Log( ERROR_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %02X  OBD Response was sooner than allowed (< %dmsec)\n",
				     pRxMsg->Data[2],
				     (gOBDMinResponseTimeMsecs) );
				eReturnCode = ERRORS;
			}
			else
			{
				Log( ERROR_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "OBD Response was sooner than allowed (< %dmsec)\n",
				     (gOBDMinResponseTimeMsecs) );
				eReturnCode = ERRORS;
			}

			if (!RequestMsgLpbk)
			{
				gEcuTimingData[EcuTimingIndex].RespTimeOutofRange++;
				gEcuTimingData[EcuTimingIndex].RespTimeTooSoon++;
			}

		}

		/* Check if response was late */
		/* Since under ISO9141 or ISO14230 we have saved off the ending timestamp
		** as ulTxTimestamp, we can now accurately calculate the difference between the
		** end of the last message to the beginning of this response.
		*/
		if ( ulResponseDelta > ((gOBDMaxResponseTimeMsecs + *ulExtendResponseTimeMsecs) * 1000) )
		{
			/* Exceeded maximum response time */
			if ( HeaderSize >= 3 )
			{
				Log( ERROR_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %02X  OBD Response was later than allowed (> %dmsec)\n",
				     pRxMsg->Data[2],
				     (gOBDMaxResponseTimeMsecs + *ulExtendResponseTimeMsecs) );
				eReturnCode = ERRORS;
			}
			else
			{
				Log( ERROR_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "OBD Response was later than allowed (> %dmsec)\n",
				     (gOBDMaxResponseTimeMsecs + *ulExtendResponseTimeMsecs) );
				eReturnCode = ERRORS;
			}

			if (!RequestMsgLpbk)
			{
				gEcuTimingData[EcuTimingIndex].RespTimeOutofRange++;
				gEcuTimingData[EcuTimingIndex].RespTimeTooLate++;
			}

		}

		/* If response was not late, reset the start time */
		*ulStartTimeMsecs = GetTickCount();
		*ulTxTimestamp = pRxMsg->Timestamp;

		// Check for proper SID response
		if ( pRxMsg->Data[HeaderSize] != ( pSidReq->SID + OBD_RESPONSE_BIT ) )
		{
			if ( HeaderSize >= 3 )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %02X  Invalid SID response to request\n",
				     pRxMsg->Data[2] );
			}
			else
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "Invalid SID response to request\n" );
			}
			return (FAIL);
		}

		// if a PID was requested
		if ( pSidReq->NumIds != 0 )
		{
			// Check for proper PID/MID/TID/INF response
			for ( i = 0; i < pSidReq->NumIds; i++ )
			{
				if ( pRxMsg->Data[HeaderSize+1] == pSidReq->Ids[i] )
				{
					break;
				}
			}

			if ( i == pSidReq->NumIds )
			{
				if ( HeaderSize >= 3 )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %02X  Invalid PID response to request\n",
					     pRxMsg->Data[2] );
				}
				else
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "Invalid PID response to request\n" );
				}
				return (FAIL);
			}
		}

		/* Save the response information */
		if (SidSaveResponseData (pRxMsg, pSidReq, pulNumResponses) != PASS)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Cannot save SID response data\n");
			return (FAIL);
		}
	}

	// Ignore, invalid message

	return (eReturnCode);
}


/******************************************************************************
**
**	Function:   ProcessISO15765Msg
**
**	Purpose:    Purpose is to process response based on connection to
**	            ISO15765.
**
**	Returns:    RETRY, ERRORS, FAIL, or PASS
**
**	NOTE:       Considered consolidating ProcessISO15765Msg with
**	            ProcessLegacyMsg.  The many unique difference would
**	            complicate the 'streamling' of the logic.  Hence the
**	            routines have intentionally remained separate.
**
*******************************************************************************
**
**	DATE        MODIFICATION
**	11/01/03    Isolated routine into a function.
**	02/23/04    Review of time evaluation logic for ISO15765 produced questions
**	            related to the evaluation logic used for CAN.  Corrected logic
**	            to perform a more absolute comparison of response time.
**	05/12/04    Enhanced routine to return a fail on any Mode $05 response.
*******************************************************************************
*/
STATUS ProcessISO15765Msg( SID_REQ       *pSidReq,
                           PASSTHRU_MSG  *pRxMsg,
                           unsigned long *ulStartTimeMsecs,
                           unsigned long *pulNumResponses,
                           unsigned long *ulNumFirstFrames,
                           unsigned long *ulTxTimestamp,
                           unsigned long *ulExtendResponseTimeMsecs )
{
	unsigned long ulResponseTimeMsecs;
	unsigned long EcuTimingIndex;
	unsigned long EcuIndex;
	unsigned long EcuId = 0;
	unsigned short i;
	STATUS eReturnCode = PASS;    // saves the return code from function calls
	BOOL RequestMsgLpbk = FALSE;

	/* create the ECU ID for later use */
	for (i = 0; i < 4; i++)
	{
		EcuId = (EcuId << 8) + (pRxMsg->Data[i]);
	}

	// if this is an echo of the request, don't track it's timing data
	if ( pRxMsg->RxStatus & TX_MSG_TYPE )
	{
		RequestMsgLpbk = TRUE;
	}
	else
	{
		/* Find this ECU's Timing Structure */
		for ( EcuTimingIndex = 0; EcuTimingIndex < OBD_MAX_ECUS; EcuTimingIndex++ )
		{
			if ( gEcuTimingData[EcuTimingIndex].EcuId == EcuId || gEcuTimingData[EcuTimingIndex].EcuId == 0x00)
			{
				if ( gEcuTimingData[EcuTimingIndex].EcuId == 0x00 )
				{
					gEcuTimingData[EcuTimingIndex].EcuId = EcuId;
				}
				break;
			}
		}
	}

	/* FAIL for padding error only once */
	if ( bPadErrorPermanent == FALSE )
	{
		if (pRxMsg->RxStatus & ISO15765_PADDING_ERROR)
		{
			bPadErrorPermanent = TRUE;

			Log( ERROR_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  ISO15765 message padding error.\n", EcuId);
			eReturnCode = ERRORS;
		}
	}

	ulResponseTimeMsecs = (pRxMsg->Timestamp - *ulTxTimestamp) / 1000;

	/* Check for FirstFrame indication */
	if ( pRxMsg->RxStatus & ISO15765_FIRST_FRAME )
	{
		/* Check if response was late (all first frame indications due in P2_MAX) */
		if ( (pRxMsg->Timestamp - *ulTxTimestamp) > (gOBDMaxResponseTimeMsecs * 1000) )
		{
			/* Exceeded maximum response time */
			Log( ERROR_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  OBD First Frame Indication was later than allowed (> %dmsec)\n",
			     EcuId, gOBDMaxResponseTimeMsecs );
			eReturnCode = ERRORS;

			if (!RequestMsgLpbk)
			{
				gEcuTimingData[EcuTimingIndex].RespTimeOutofRange++;
				gEcuTimingData[EcuTimingIndex].RespTimeTooLate++;
			}
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Calculated OBD Response time: %d msec\n",
			     ( (pRxMsg->Timestamp - *ulTxTimestamp) / 1000) );
		}

		/* Extend the response time to the worst case for segmented responses */
		gEcuTimingData[EcuTimingIndex].ExtendResponseTimeMsecs = 30000;
		if ( gEcuTimingData[EcuTimingIndex].ExtendResponseTimeMsecs > *ulExtendResponseTimeMsecs )
		{
			*ulExtendResponseTimeMsecs = gEcuTimingData[EcuTimingIndex].ExtendResponseTimeMsecs;
		}
		(*ulNumFirstFrames)++;
		Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Receiving segmented responses, please wait...\n");
	}

	/* Check for echoed request message */
	else if ( pRxMsg->RxStatus & TX_MSG_TYPE )
	{
		if ( ( pRxMsg->DataSize >= 5 ) &&
		     ( pRxMsg->Data[4]  == pSidReq->SID ) )
		{
			/* Save the timestamp */
			*ulTxTimestamp = pRxMsg->Timestamp;
		}
	}

	/* Check for NAK response message */
	else if ( ( pRxMsg->DataSize >= 7   ) &&
	          ( pRxMsg->Data[4]  == NAK ) &&
	          ( pRxMsg->Data[5]  == pSidReq->SID ) )
	{
		if ( gEcuTimingData[EcuTimingIndex].NAKReceived == FALSE )
		{
			/* Tally up the response statistics */
			gEcuTimingData[EcuTimingIndex].AggregateResponseTimeMsecs += ulResponseTimeMsecs;
			gEcuTimingData[EcuTimingIndex].AggregateResponses += ( ( ( pRxMsg->DataSize - 4 ) / ISO15765_MAX_BYTES_PER_FRAME ) + 1 );
			if ( ulResponseTimeMsecs > gEcuTimingData[EcuTimingIndex].LongestResponsesTime)
			{
				gEcuTimingData[EcuTimingIndex].LongestResponsesTime = ulResponseTimeMsecs;
			}

			gEcuTimingData[EcuTimingIndex].NAKReceived = TRUE;
		}

		/* Check if response was late (all negative respones due in P2_MAX) */
		if ( (pRxMsg->Timestamp - *ulTxTimestamp) > ((gOBDMaxResponseTimeMsecs +  gEcuTimingData[EcuTimingIndex].ResponsePendingDelay) * 1000) )
		{
			/* Exceeded maximum response time */
			Log( ERROR_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  OBD Negative Response was later than allowed (> %dmsec)\n",
			     EcuId, gOBDMaxResponseTimeMsecs + gEcuTimingData[EcuTimingIndex].ResponsePendingDelay );
			eReturnCode = ERRORS;

			gEcuTimingData[EcuTimingIndex].RespTimeOutofRange++;
			gEcuTimingData[EcuTimingIndex].RespTimeTooLate++;
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Calculated OBD Response time: %d msec\n",
			     ( (pRxMsg->Timestamp - *ulTxTimestamp) / 1000) );
		}

		/* Save the response information */
		if (SidSaveResponseData (pRxMsg, pSidReq, pulNumResponses) != PASS)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Cannot save SID response data\n");
			return (FAIL);
		}

		/*check the kind of response received for the vehicle*/
		switch( pRxMsg->Data[6] )
		{
			case NAK_GENERAL_REJECT: // 0x10
				Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  General reject to Service $%02X.\n", EcuId, pRxMsg->Data[5] );
				break;

			case NAK_NOT_SUPPORTED: // 0x11
				Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  Service $%02X not supported.\n", EcuId, pRxMsg->Data[5] );
				break;

			case NAK_INVALID_FORMAT: // 0x12
				Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  Service $%02X supported. Unsupported PID request.\n",
				     EcuId,
				     pRxMsg->Data[5] );
				break;

			case NAK_REPEAT_REQUEST: // 0x21
				Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  Service $%02X supported. Busy.\n",
				     EcuId,
				     pRxMsg->Data[5] );
				break;

			case NAK_SEQUENCE_ERROR: // 0x22
				Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  Service $%02X supported. Conditions not correct.\n",
				     EcuId,
				     pRxMsg->Data[5] );
				break;

			case NAK_RESPONSE_PENDING: // 0x78
				Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  Service $%02X supported. Response pending.\n",
				     EcuId,
				     pRxMsg->Data[5] );
				break;

			default:
				Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  Unknown negative response to Service $%02X.\n",
				     EcuId,
				     pRxMsg->Data[5] );
				break;
		}

		/*
		** NAK Truth table per J1699 Rev 11.6 Table B
		*/

		switch( pSidReq->SID )
		{
			case 0x01:
				if ( gDetermineProtocol == 1 &&
				     pSidReq->Ids[0] == 0x00 &&
				     pRxMsg->Data[6] == NAK_REPEAT_REQUEST )
				{
					return (RETRY);
				}
				else
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  SID $01 NAK response error!\n", EcuId );
					return (FAIL);
				}
				break;

			case 0x04:
				if ( pRxMsg->Data[6] == NAK_RESPONSE_PENDING )
				{
					/* If response pending, extend the wait time */
					gEcuTimingData[EcuTimingIndex].ExtendResponseTimeMsecs = 30000;
					if ( gEcuTimingData[EcuTimingIndex].ExtendResponseTimeMsecs > *ulExtendResponseTimeMsecs )
					{
						*ulExtendResponseTimeMsecs = gEcuTimingData[EcuTimingIndex].ExtendResponseTimeMsecs;
					}
					gEcuTimingData[EcuTimingIndex].ResponsePendingDelay = 5000;
					if ( gEcuTimingData[EcuTimingIndex].ResponsePendingDelay > ulResponsePendingDelay )
					{
						ulResponsePendingDelay = gEcuTimingData[EcuTimingIndex].ResponsePendingDelay;
					}

					/* set wait flag */
					if (LookupEcuIndex (pRxMsg, &EcuIndex) == PASS)
					{
						ulEcuWaitFlags |= (1<<EcuIndex);
						gOBDResponse[EcuIndex].bResponseReceived = FALSE; // allow ECU to respond again
					}
					else
					{
						return (FAIL);
					}
				}
				else if ( pRxMsg->Data[6] != NAK_SEQUENCE_ERROR )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  SID $04 NAK response error!\n", EcuId );
					return (FAIL);
				}
				break;

			case 0x09:
				// IF EOBD/IOBD/OBDBr and CVN
				if ( pSidReq->Ids[0] == 0x06 &&
				     ( gUserInput.eComplianceType != US_OBDII &&
				       gUserInput.eComplianceType != HD_OBD ) )
				{
					break;
				}

			default :
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  SID $%02X NAK response error!\n",
				     EcuId,
				     pSidReq->SID );
				return (FAIL);
				break;
		}
	}

	/* Check for response message */
	else if ( pRxMsg->DataSize >= 5 )
	{
		if ( gEcuTimingData[EcuTimingIndex].NAKReceived == FALSE )
		{
			/* Tally up the response statistics */
			gEcuTimingData[EcuTimingIndex].AggregateResponseTimeMsecs += ulResponseTimeMsecs;
			gEcuTimingData[EcuTimingIndex].AggregateResponses += ( ( ( pRxMsg->DataSize - 4 ) / ISO15765_MAX_BYTES_PER_FRAME ) + 1 );
			if ( ulResponseTimeMsecs > gEcuTimingData[EcuTimingIndex].LongestResponsesTime)
			{
				gEcuTimingData[EcuTimingIndex].LongestResponsesTime = ulResponseTimeMsecs;
			}
		}
		else
		{
			gEcuTimingData[EcuTimingIndex].NAKReceived = FALSE;
		}

		/* Check if response was late (compensate for segmented responses) */
		if ( (pRxMsg->Timestamp - *ulTxTimestamp) >
		     ((gOBDMaxResponseTimeMsecs + gEcuTimingData[EcuTimingIndex].ExtendResponseTimeMsecs) * 1000) )
		{
			/* Exceeded maximum response time */
			Log( ERROR_FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  OBD Response was later than allowed (> %dmsec)\n",
			     EcuId,
			     (gOBDMaxResponseTimeMsecs + gEcuTimingData[EcuTimingIndex].ExtendResponseTimeMsecs) );
			eReturnCode = ERRORS;

			gEcuTimingData[EcuTimingIndex].RespTimeOutofRange++;
			gEcuTimingData[EcuTimingIndex].RespTimeTooLate++;
			Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Calculated OBD Response time: %d msec\n",
			     ( (pRxMsg->Timestamp - *ulTxTimestamp) / 1000) );
		}

		/* clear wait flag */
		if (LookupEcuIndex (pRxMsg, &EcuIndex) == PASS)
		{
			ulEcuWaitFlags &= ~(1<<EcuIndex);
			gEcuTimingData[EcuTimingIndex].ResponsePendingDelay = 0;        /* ECU response no longer pending */
		}
		else
		{
			return (FAIL);
		}

		/* Check if all ECUs with response pending have responded */
		if (ulEcuWaitFlags == 0)
		{
			ulResponsePendingDelay = 0;         /* no ECU response is pending */

			if (*ulNumFirstFrames == 0)
			{
				*ulExtendResponseTimeMsecs = 0;  /* no First Frames, clear extended response time */
			}
		}

		/* Check if we can turn off the time extension for segmented frames */
		if ( pRxMsg->DataSize > ( 4 + ISO15765_MAX_BYTES_PER_FRAME ) )
		{
			if ( *ulNumFirstFrames > 0 )
			{
				if ( --(*ulNumFirstFrames) == 0 )
				{
					/*
					** If we have received as many segmented frames as
					** FirstFrame indications, restore 'response pending' time extension
					*/
					*ulExtendResponseTimeMsecs = ulResponsePendingDelay;
				}
			}
		}

		// Check for proper SID response
		if ( pRxMsg->Data[4] != ( pSidReq->SID + OBD_RESPONSE_BIT ) )
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "ECU %X  Invalid SID response to request\n", EcuId );
			return (FAIL);
		}

		// If a PID was requested
		if ( pSidReq->NumIds != 0 )
		{
			// Check for proper PID/MID/TID/INF response
			for ( i = 0; i < pSidReq->NumIds; i++ )
			{
				if ( pRxMsg->Data[5] == pSidReq->Ids[i] )
				{
					break;
				}
			}
			if ( i == pSidReq->NumIds )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  Invalid PID response to request\n", EcuId );
				return (FAIL);
			}
		}

		/* Save the response information */
		if (SidSaveResponseData(pRxMsg, pSidReq, pulNumResponses) != PASS)
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Cannot save SID response data\n");
			return (FAIL);
		}
	}

	// Ignore, invalid message

	return (eReturnCode);
}


//*****************************************************************************
//
//	Function:   SetupRequestMSG
//
//	Purpose:    Purpose is to setup request message based on the protocol.
//	            Routine will normalize the message data handling between
//	            the regulated OBD protocol message structures.
//
//*****************************************************************************
//
//	DATE        MODIFICATION
//	11/01/03    Isolated routine into a function.
//
//*****************************************************************************
STATUS SetupRequestMSG( SID_REQ *pSidReq, PASSTHRU_MSG *pTxMsg )
{

	/* Set message data information common to all protocols */
	pTxMsg->ProtocolID  = gOBDList[gOBDListIndex].Protocol;
	pTxMsg->RxStatus    = TX_MSG_TYPE;
	pTxMsg->TxFlags     = 0x00;	/*Change back to non-blocking*/

	/* Configure protocol specific message data. */
	switch(gOBDList[gOBDListIndex].Protocol)
	{
		case J1850VPW:
		case ISO9141:
		{
			pTxMsg->Data[0]  = 0x68;
			pTxMsg->Data[1]  = 0x6A;
			pTxMsg->Data[2]  = TESTER_NODE_ADDRESS;
			pTxMsg->Data[3]  = pSidReq->SID;

			memcpy( &pTxMsg->Data[4], &pSidReq->Ids[0], pSidReq->NumIds );
			pTxMsg->DataSize = 4 + pSidReq->NumIds;
		}
		break;
		case J1850PWM:
		{
			pTxMsg->Data[0]  = 0x61;
			pTxMsg->Data[1]  = 0x6A;
			pTxMsg->Data[2]  = TESTER_NODE_ADDRESS;
			pTxMsg->Data[3]  = pSidReq->SID;

			memcpy( &pTxMsg->Data[4], &pSidReq->Ids[0], pSidReq->NumIds );
			pTxMsg->DataSize = 4 + pSidReq->NumIds;
		}
		break;
		case ISO14230:
		{
			pTxMsg->Data[0]  = 0xC0 + pSidReq->NumIds + 1;
			pTxMsg->Data[1]  = 0x33;
			pTxMsg->Data[2]  = TESTER_NODE_ADDRESS;
			pTxMsg->Data[3]  = pSidReq->SID;

			memcpy( &pTxMsg->Data[4], &pSidReq->Ids[0], pSidReq->NumIds );
			pTxMsg->DataSize = 4 + pSidReq->NumIds;
		}
		break;
		case ISO15765:
		{
			pTxMsg->TxFlags |= ISO15765_FRAME_PAD;

			if (gOBDList[gOBDListIndex].InitFlags & CAN_29BIT_ID)
			{
				pTxMsg->TxFlags |= CAN_29BIT_ID;

				pTxMsg->Data[0]  = 0x18;
				pTxMsg->Data[1]  = 0xDB;
				pTxMsg->Data[2]  = 0x33;
				pTxMsg->Data[3]  = TESTER_NODE_ADDRESS;
				pTxMsg->Data[4]  = pSidReq->SID;

				memcpy( &pTxMsg->Data[5], &pSidReq->Ids[0], pSidReq->NumIds );
				pTxMsg->DataSize = 5 + pSidReq->NumIds;
			}
			else
			{
				pTxMsg->Data[0]  = 0x00;
				pTxMsg->Data[1]  = 0x00;
				pTxMsg->Data[2]  = 0x07;
				pTxMsg->Data[3]  = 0xDF;
				pTxMsg->Data[4]  = pSidReq->SID;

				memcpy( &pTxMsg->Data[5], &pSidReq->Ids[0], pSidReq->NumIds );
				pTxMsg->DataSize = 5 + pSidReq->NumIds;
			}
		}
		break;
		default:
		{
			Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
			     "Invalid protocol specified in SetupRequestMSG\n");
			return FAIL;
		}
	}

	return PASS;
}