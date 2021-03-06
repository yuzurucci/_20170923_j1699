
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

void ChkIFRAdjust( PASSTHRU_MSG *RxMsg );	/* Logic ajustment for PID Group Reverse order request. */
STATUS IsMessageUnique
                      (
                        unsigned char *pucBuffer,	/* pointer to buffer */
                        unsigned short usSize,		/* size of buffer */
                        unsigned char *pucMsg,		/* pointer to message to be checked */
                        unsigned short usMsgSize	/* size of message */
                      );

/*
*******************************************************************************
** SidSaveResponseData -
** Function to save SID response data
*******************************************************************************
**	DATE        Modification
**	08/28/03    SF#790547:  Mode 6 MY Check not required.
**	            Testing 2005 model year vehicle has uncovered an issue with
**	            the source codes handling of mode 6 data.  Modification of
**	            routine, "SidSaveResponseData", are required to complete
**	            this change request.
*******************************************************************************
*/
STATUS SidSaveResponseData(PASSTHRU_MSG *RxMsg, SID_REQ *SidReq, unsigned long *pulNumResponses)
{
	unsigned long HeaderSize;
	unsigned long EcuIndex;
	unsigned long ByteIndex;
	unsigned char bElementOffset;
	unsigned long ulInx;	// coordinate mode 6 data respones for multiple data response.


	/* Set the response header size based on the protocol */
	HeaderSize = gOBDList[gOBDListIndex].HeaderSize;

	/* Get index into gOBDResponse struct */
	if (LookupEcuIndex (RxMsg, &EcuIndex) != PASS)
	{
		return(FAIL);
	}

	ChkIFRAdjust( RxMsg );

	if (gOBDResponse[EcuIndex].bResponseReceived == TRUE)
	{
		Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "ECU %X  has already responded\n", GetEcuId(EcuIndex) );
	}
	else
	{
		(*pulNumResponses)++;	// count the new response
	}

	/* Save the data in the appropriate SID/PID/MID/TID/InfoType */
	switch(RxMsg->Data[HeaderSize])
	{
		/* SID 1 (Mode 1) */
		case 0x41:
		{
			// indicate that this ECU has responded
			gOBDResponse[EcuIndex].bResponseReceived = TRUE;

			/* Process message based on PID number */
			switch (RxMsg->Data[HeaderSize + 1])
			{
				case 0x00:
				{
					if ( gVerifyLink == TRUE ||
					     gDetermineProtocol == 1 )
					{
						gOBDResponse[EcuIndex].LinkActive = TRUE;

						if ( gOBDResponse[EcuIndex].Sid1PidSupportSize != 0 )
							break;
					}
				}
				case 0x20:
				case 0x40:
				case 0x60:
				case 0x80:
				case 0xA0:
				case 0xC0:
				case 0xE0:
				{
					/* Make sure there is enough data in the message to process */
					if (RxMsg->DataSize < (HeaderSize + 1 + sizeof(ID_SUPPORT)))
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  Not enough data in SID $1 PID $%02X support response message to process\n",
						     GetEcuId(EcuIndex),
						     RxMsg->Data[HeaderSize + 1] );
						return(FAIL);
					}

					/* Save the data in the buffer and set the new size */
					for ( ulInx = HeaderSize + 1; ulInx < RxMsg->DataSize; ulInx += sizeof(ID_SUPPORT) )
					{
						/* find the element number (for the array of structures) */
						bElementOffset = (unsigned char)(RxMsg->Data[ulInx]) >> 5;

						/* move the data into the element */
						memcpy( &gOBDResponse[EcuIndex].Sid1PidSupport[bElementOffset],
						        &RxMsg->Data[ulInx],
						        sizeof( ID_SUPPORT ));

						/* increase the element count if the element in the array was previously unused */
						if ( (bElementOffset + 1) > gOBDResponse[EcuIndex].Sid1PidSupportSize)
						{
							gOBDResponse[EcuIndex].Sid1PidSupportSize = (bElementOffset + 1);
						}
					}
				}
				/*
				 * FALL THROUGH AND TREAT LIKE OTHER PIDS
				 */

				default:
				{
					/* If not PID 0, then check if this is a response to an unsupported PID */
					if ( gIgnoreUnsupported == FALSE &&
					     RxMsg->Data[HeaderSize + 1] != 0 &&
					     (gOBDResponse[EcuIndex].Sid1PidSupport[
					       (RxMsg->Data[HeaderSize + 1] - 1) >> 5].IDBits[
					       ((RxMsg->Data[HeaderSize + 1] - 1) >> 3) & 0x03]
					       & (0x80 >> ((RxMsg->Data[HeaderSize + 1] - 1) & 0x07))) == 0
					   )
					{
						Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  Unsupported SID $1 PID $%02X detected\n",
						     GetEcuId(EcuIndex),
						     RxMsg->Data[HeaderSize + 1] );
					}

					/* Make sure there is enough data in the message to process */
					if (RxMsg->DataSize <= (HeaderSize + 1))
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  Not enough data in SID $1 PID $%02X response message to process\n",
						     GetEcuId(EcuIndex),
						     RxMsg->Data[HeaderSize + 1] );
						return(FAIL);
					}

					/*
					** If only one PID requested, make sure the
					** PID has the correct amount of data
					*/
					if (SidReq->NumIds == 1)
					{
						switch (RxMsg->Data[HeaderSize + 1])
						{
							/* Single byte PIDs */
							case 0x04:
							case 0x05:
							case 0x0A:
							case 0x0B:
							case 0x0D:
							case 0x0E:
							case 0x0F:
							case 0x11:
							case 0x12:
							case 0x13:
							case 0x1C:
							case 0x1D:
							case 0x1E:
							case 0x2C:
							case 0x2D:
							case 0x2E:
							case 0x2F:
							case 0x30:
							case 0x33:
							case 0x45:
							case 0x46:
							case 0x47:
							case 0x48:
							case 0x49:
							case 0x4A:
							case 0x4B:
							case 0x4C:
							case 0x51:
							case 0x52:
							case 0x5A:
							case 0x5B:
							case 0x5C:
							case 0x5F:
							case 0x61:
							case 0x62:
							case 0x7D:
							case 0x7E:
							case 0x84:
							case 0x8D:
							case 0x8E:
							{
								/* Check for one data byte */
								if ((RxMsg->DataSize - (HeaderSize + 2)) != 1)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  SID $1 PID $%02X has %d data bytes (should be 1)\n",
									     GetEcuId(EcuIndex),
									     RxMsg->Data[HeaderSize + 1], (RxMsg->DataSize - (HeaderSize + 2)));
									return(FAIL);
								}
							}
							break;

							/* Two byte PIDs */
							case 0x02:
							case 0x03:
							case 0x0C:
							case 0x10:
							case 0x14:
							case 0x15:
							case 0x16:
							case 0x17:
							case 0x18:
							case 0x19:
							case 0x1A:
							case 0x1B:
							case 0x1F:
							case 0x21:
							case 0x22:
							case 0x23:
							case 0x31:
							case 0x32:
							case 0x3C:
							case 0x3D:
							case 0x3E:
							case 0x3F:
							case 0x42:
							case 0x43:
							case 0x44:
							case 0x4D:
							case 0x4E:
							case 0x53:
							case 0x54:
							case 0x59:
							case 0x5D:
							case 0x5E:
							case 0x63:
							case 0x65:
							case 0x92:
							case 0x9E:
							case 0xA2:
							{
								/* Check for two data bytes */
								if ((RxMsg->DataSize - (HeaderSize + 2)) != 2)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  SID $1 PID $%02X has %d data bytes (should be 2)\n",
									     GetEcuId(EcuIndex),
									     RxMsg->Data[HeaderSize + 1], (RxMsg->DataSize - (HeaderSize + 2)) );
									return(FAIL);
								}
							}
							break;

							/* Three byte PIDs */
							case 0x67:
							case 0x6F:
							case 0x90:
							case 0x93:
							{
								/* Check for four data bytes */
								if ((RxMsg->DataSize - (HeaderSize + 2)) != 3)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  SID $1 PID $%02X has %d data bytes (should be 3)\n",
									     GetEcuId(EcuIndex),
									     RxMsg->Data[HeaderSize + 1], (RxMsg->DataSize - (HeaderSize + 2)));
									return(FAIL);
								}
							}
							break;

							/* Four byte PIDs */
							case 0x00:
							case 0x20:
							case 0x40:
							case 0x60:
							case 0x80:
							case 0xA0:
							case 0xC0:
							case 0xE0:
							case 0x01:
							case 0x24:
							case 0x25:
							case 0x26:
							case 0x27:
							case 0x28:
							case 0x29:
							case 0x2A:
							case 0x2B:
							case 0x34:
							case 0x35:
							case 0x36:
							case 0x37:
							case 0x38:
							case 0x39:
							case 0x3A:
							case 0x3B:
							case 0x41:
							case 0x4F:
							case 0x50:
							case 0x9B:
							case 0x9D:
							{
								/* Check for four data bytes */
								if ((RxMsg->DataSize - (HeaderSize + 2)) != 4)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  SID $1 PID $%02X has %d data bytes (should be 4)\n",
									     GetEcuId(EcuIndex),
									     RxMsg->Data[HeaderSize + 1], (RxMsg->DataSize - (HeaderSize + 2)));
									return(FAIL);
								}
							}
							break;

							/* Five byte PIDs */
							case 0x64:
							case 0x66:
							case 0x6A:
							case 0x6B:
							case 0x6C:
							case 0x72:
							case 0x73:
							case 0x74:
							case 0x77:
							case 0x86:
							case 0x87:
							case 0x91:
							{
								if ((RxMsg->DataSize - (HeaderSize + 2)) != 5)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  SID $1 PID $%02X has %d data bytes (should be 5)\n",
									     GetEcuId(EcuIndex),
									     RxMsg->Data[HeaderSize + 1], (RxMsg->DataSize - (HeaderSize + 2)));
									return(FAIL);
								}
							}
							break;

							/* Six byte PIDs */
							case 0x71:
							case 0x9A:
							{
								if ((RxMsg->DataSize - (HeaderSize + 2)) != 6)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  SID $1 PID $%02X has %d data bytes (should be 6)\n",
									     GetEcuId(EcuIndex),
									     RxMsg->Data[HeaderSize + 1], (RxMsg->DataSize - (HeaderSize + 2)));
									return(FAIL);
								}
							}
							break;

							/* Seven byte PIDs */
							case 0x68:
							case 0x69:
							case 0x75:
							case 0x76:
							case 0x7A:
							case 0x7B:
							case 0x8B:
							case 0x8F:
							{
								if ((RxMsg->DataSize - (HeaderSize + 2)) != 7)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  SID $1 PID $%02X has %d data bytes (should be 7)\n",
									     GetEcuId(EcuIndex),
									     RxMsg->Data[HeaderSize + 1], (RxMsg->DataSize - (HeaderSize + 2)));
									return(FAIL);
								}
							}
							break;

							/* Nine byte PIDs */
							case 0x6E:
							case 0x78:
							case 0x79:
							case 0x7C:
							case 0x83:
							case 0x98:
							case 0x99:
							case 0x9F:
							case 0xA1:
							{
								if ((RxMsg->DataSize - (HeaderSize + 2)) != 9)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  SID $1 PID $%02X has %d data bytes (should be 9)\n",
									     GetEcuId(EcuIndex),
									     RxMsg->Data[HeaderSize + 1], (RxMsg->DataSize - (HeaderSize + 2)));
									return(FAIL);
								}
							}
							break;

							/* Ten byte PIDs */
							case 0x70:
							case 0x85:
							{
								if ((RxMsg->DataSize - (HeaderSize + 2)) != 10)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  SID $1 PID $%02X has %d data bytes (should be 10)\n",
									     GetEcuId(EcuIndex),
									     RxMsg->Data[HeaderSize + 1], (RxMsg->DataSize - (HeaderSize + 2)));
									return(FAIL);
								}
							}
							break;

							/* Eleven byte PIDs */
							case 0x6D:
							{
								if ((RxMsg->DataSize - (HeaderSize + 2)) != 11)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  SID $1 PID $%02X has %d data bytes (should be 11)\n",
									     GetEcuId(EcuIndex),
									     RxMsg->Data[HeaderSize + 1], (RxMsg->DataSize - (HeaderSize + 2)));
									return(FAIL);
								}
							}
							break;

							/* Twelve byte PIDs */
							case 0x94:
							{
								if ((RxMsg->DataSize - (HeaderSize + 2)) != 12)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  SID $1 PID $%02X has %d data bytes (should be 12)\n",
									     GetEcuId(EcuIndex),
									     RxMsg->Data[HeaderSize + 1], (RxMsg->DataSize - (HeaderSize + 2)));
									return(FAIL);
								}
							}
							break;

							/* Thirteen byte PIDs */
							case 0x7F:
							case 0x88:
							{
								if ((RxMsg->DataSize - (HeaderSize + 2)) != 13)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  SID $1 PID $%02X has %d data bytes (should be 13)\n",
									     GetEcuId(EcuIndex),
									     RxMsg->Data[HeaderSize + 1], (RxMsg->DataSize - (HeaderSize + 2)));
									return(FAIL);
								}
							}
							break;

							/* Seventeen byte PIDs */
							case 0x8C:
							case 0x9C:
							{
								if ((RxMsg->DataSize - (HeaderSize + 2)) != 17)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  SID $1 PID $%02X has %d data bytes (should be 17)\n",
									     GetEcuId(EcuIndex),
									     RxMsg->Data[HeaderSize + 1], (RxMsg->DataSize - (HeaderSize + 2)));
									return(FAIL);
								}
							}
							break;

							/* Forty-One byte PIDs */
							case 0x81:
							case 0x82:
							case 0x89:
							case 0x8A:
							{
								if ((RxMsg->DataSize - (HeaderSize + 2)) != 41)
								{
									Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
									     "ECU %X  SID $1 PID $%02X has %d data bytes (should be 41)\n",
									     GetEcuId(EcuIndex),
									     RxMsg->Data[HeaderSize + 1], (RxMsg->DataSize - (HeaderSize + 2)));
									return(FAIL);
								}
							}
							break;

							/* 1 or 2 byte PIDs */
							case 0x06:      /* PID $06 1 / 2-Byte definition */
							case 0x07:      /* PID $07 1 / 2-Byte definition */
							case 0x08:      /* PID $08 1 / 2-Byte definition */
							case 0x09:      /* PID $09 1 / 2-Byte definition */
							case 0x55:      /* PID $55 1 / 2-Byte definition */
							case 0x56:      /* PID $56 1 / 2-Byte definition */
							case 0x57:      /* PID $57 1 / 2-Byte definition */
							case 0x58:      /* PID $58 1 / 2-Byte definition */
							{
								/* Only check if successfully determined the (variable) PID size */
								if (gSid1VariablePidSize > 0)
								{
									/* Check for one or two data bytes */
									if ((RxMsg->DataSize - (HeaderSize + 2)) != (unsigned)gSid1VariablePidSize)
									{
										Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
										     "ECU %X  SID $1 PID $%02X has %d data bytes (should be %u)\n",
										     GetEcuId(EcuIndex),
										     RxMsg->Data[HeaderSize + 1],
										     (RxMsg->DataSize - (HeaderSize + 2)),
										     gSid1VariablePidSize);
										return(FAIL);
									}
								}
							}
							default:
							{
								/* Non-OBD PID */
							}
							break;
						}
					}

					/* Determine if there is enough room in the buffer to store the data */
					if ( (RxMsg->DataSize - HeaderSize - 1) > sizeof(gOBDResponse[EcuIndex].Sid1Pid) )
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  SID $1 response data exceeded buffer size\n",
						     GetEcuId(EcuIndex) );
						return(FAIL);
					}

					/* Save the data in the buffer and set the new size */
					memcpy( &gOBDResponse[EcuIndex].Sid1Pid[0],
					        &RxMsg->Data[HeaderSize + 1],
					        (RxMsg->DataSize - HeaderSize - 1) );
					gOBDResponse[EcuIndex].Sid1PidSize = (unsigned short)(RxMsg->DataSize - HeaderSize - 1);
				}
				break;
			}
		}
		break;


		/* SID 2 (Mode 2) */
		case 0x42:
		{
			// indicate that this ECU has responded
			gOBDResponse[EcuIndex].bResponseReceived = TRUE;

			/* Process message based on PID number */
			switch (RxMsg->Data[HeaderSize + 1])
			{
				case 0x00:
				case 0x20:
				case 0x40:
				case 0x60:
				case 0x80:
				case 0xA0:
				case 0xC0:
				case 0xE0:
				{
					/* Make sure there is enough data in the message to process */
					if (RxMsg->DataSize < (HeaderSize + 1 + sizeof(ID_SUPPORT)))
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  Not enough data in SID $2 support response message to process\n",
						     GetEcuId(EcuIndex) );
						return(FAIL);
					}

					/* Save the data in the buffer and set the new size */
					for ( ulInx = HeaderSize + 1; ulInx < RxMsg->DataSize; ulInx += sizeof(FF_SUPPORT) )
					{
						/* find the element number (for the array of structures) */
						bElementOffset = (unsigned char)(RxMsg->Data[ulInx]) >> 5;

						/* move the data into the element */
						memcpy( &gOBDResponse[EcuIndex].Sid2PidSupport[bElementOffset],
						        &RxMsg->Data[ulInx],
						        sizeof( FF_SUPPORT ));

						/* increase the element count if the element in the array was previously unused */
						if ( (bElementOffset + 1) > gOBDResponse[EcuIndex].Sid2PidSupportSize)
						{
							gOBDResponse[EcuIndex].Sid2PidSupportSize = (bElementOffset + 1);
						}
					}
				}
				/*
				 * FALL THROUGH AND TREAT LIKE OTHER PIDS
				 */

				default:
				{
					/* Check if this is a response to an unsupported PID */
					if ( gIgnoreUnsupported == FALSE &&
					     IsSid2PidSupported(EcuIndex, RxMsg->Data[HeaderSize + 1]) == FALSE )
					{
						Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  Unsupported SID $2 PID $%02X detected\n",
						     GetEcuId(EcuIndex),
						     RxMsg->Data[HeaderSize + 1] );
					}

					/* Make sure there is enough data in the message to process */
					if (RxMsg->DataSize <= (HeaderSize + 1))
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  Not enough data in SID $2 response message to process\n",
						     GetEcuId(EcuIndex) );
						return(FAIL);
					}

					/* Determine if there is enough room in the buffer to store the data */
					if ((RxMsg->DataSize - HeaderSize - 1) > sizeof(gOBDResponse[EcuIndex].Sid2Pid) )
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  SID $2 response data exceeded buffer size\n",
						     GetEcuId(EcuIndex) );
						return(FAIL);
					}

					/* Save the data in the buffer and set the new size */
					memcpy( &gOBDResponse[EcuIndex].Sid2Pid[0],
					        &RxMsg->Data[HeaderSize + 1],
					        (RxMsg->DataSize - HeaderSize - 1) );
					gOBDResponse[EcuIndex].Sid2PidSize = (unsigned short)(RxMsg->DataSize - HeaderSize - 1);
				}
				break;
			}
		}
		break;


		/* SID 3 (Mode 3) */
		case 0x43:
		{
			gOBDResponse[EcuIndex].Sid3Supported = TRUE;

			if (gOBDList[gOBDListIndex].Protocol != ISO15765)
			{
				if ( IsMessageUnique(&gOBDResponse[EcuIndex].Sid3[0],
				                     gOBDResponse[EcuIndex].Sid3Size,
				                     &RxMsg->Data[HeaderSize + 1],
				                     (unsigned short)(RxMsg->DataSize - HeaderSize - 1)) == FAIL )
				{
					Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  has already responded\n", GetEcuId(EcuIndex) );
					(*pulNumResponses)--;	// don't count the response
				}
				else
				{
					/* Determine if there is enough room in the buffer to store the data */
					if ( (RxMsg->DataSize - HeaderSize - 1) >
						 (sizeof(gOBDResponse[EcuIndex].Sid3) - gOBDResponse[EcuIndex].Sid3Size) )
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  SID $3 response data exceeded buffer size\n",
						     GetEcuId(EcuIndex) );
						return(FAIL);
					}

					/* Just copy the DTC bytes */
					memcpy( &gOBDResponse[EcuIndex].Sid3[gOBDResponse[EcuIndex].Sid3Size],
					        &RxMsg->Data[HeaderSize + 1],
					       RxMsg->DataSize - HeaderSize - 1);
					gOBDResponse[EcuIndex].Sid3Size += (unsigned short)(RxMsg->DataSize - HeaderSize - 1);
				}
			}
			else
			{
				// indicate that this ECU has responded
				gOBDResponse[EcuIndex].bResponseReceived = TRUE;

				/* Determine if there is enough room in the buffer to store the data */
				if ( (RxMsg->DataSize - HeaderSize - 1) > sizeof(gOBDResponse[EcuIndex].Sid3) )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  SID $3 response data exceeded buffer size\n",
					     GetEcuId(EcuIndex) );
					return(FAIL);
				}

				/* If ISO15765, skip the number of DTCs byte */
				if (RxMsg->DataSize <= (HeaderSize + 2) || (RxMsg->Data[HeaderSize + 1] == 0))
				{
					/* If no DTCs, set to zero */
					memset(&gOBDResponse[EcuIndex].Sid3[0], 0, 2);
					gOBDResponse[EcuIndex].Sid3Size = 2;
				}
				else
				{
					/* Otherwise copy the DTC data */
					memcpy( &gOBDResponse[EcuIndex].Sid3[0],
					        &RxMsg->Data[HeaderSize + 2],
					        (RxMsg->DataSize - HeaderSize - 2));
					gOBDResponse[EcuIndex].Sid3Size = (unsigned short)(RxMsg->DataSize - HeaderSize - 2);
				}

				/* Per J1699 rev 11.6- TC# 6.3 & 7.3 verify that reported number of DTCs
				** matches that of actual DTC count.
				*/
				if ( RxMsg->Data[HeaderSize + 1] != ( (RxMsg->DataSize - HeaderSize - 2) / 2 ) )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  SID $3 DTC Count must match the # of DTC's reported.\n",
					     GetEcuId(EcuIndex) );
					return (FAIL);
				}
			}
		}
		break;


		/* SID 4 (Mode 4) */
		case 0x44:
		{
			// indicate that this ECU has responded
			gOBDResponse[EcuIndex].bResponseReceived = TRUE;

			/* Indicate positive response */
			gOBDResponse[EcuIndex].Sid4[0] = 0x44;
			gOBDResponse[EcuIndex].Sid4Size = 1;
		}
		break;


		/* SID 5 (Mode 5) */
		case 0x45:
		{
			// indicate that this ECU has responded
			gOBDResponse[EcuIndex].bResponseReceived = TRUE;

			/* Indicate positive response */
			gOBDResponse[EcuIndex].Sid5Tid[0] = 0x45;
			gOBDResponse[EcuIndex].Sid5TidSize = 1;
		}
		break;


		/* SID 6 (Mode 6) */
		case 0x46:
		{
			if (gOBDList[gOBDListIndex].Protocol == ISO15765)
			{
				// indicate that this ECU has responded
				gOBDResponse[EcuIndex].bResponseReceived = TRUE;
			}

			/* Process message based on PID number */
			switch (RxMsg->Data[HeaderSize + 1])
			{
				case 0x00:
				case 0x20:
				case 0x40:
				case 0x60:
				case 0x80:
				case 0xA0:
				case 0xC0:
				case 0xE0:
				{
					/* Make sure there is enough data in the message to process */
					if (RxMsg->DataSize < (HeaderSize + 1 + sizeof(ID_SUPPORT)))
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  Not enough data in SID $6 support response message to process\n",
						     GetEcuId(EcuIndex) );
						return(FAIL);
					}

					/* If not ISO15765 protocol, get rid of 0xFF to make data look the same */
					if (gOBDList[gOBDListIndex].Protocol != ISO15765)
					{
						for (ByteIndex = (HeaderSize + 2); ByteIndex < (RxMsg->DataSize - 1);
						ByteIndex++)
						{
							RxMsg->Data[ByteIndex] = RxMsg->Data[ByteIndex + 1];
						}
						RxMsg->DataSize -= 2;
					}

					/* Save the data in the buffer and set the new size */
					for ( ulInx = HeaderSize + 1; ulInx < RxMsg->DataSize; ulInx += sizeof(ID_SUPPORT) )
					{
						/* find the element number (for the array of structures) */
						bElementOffset = (unsigned char)(RxMsg->Data[ulInx]) >> 5;

						/* move the data into the element */
						memcpy( &gOBDResponse[EcuIndex].Sid6MidSupport[bElementOffset],
						        &RxMsg->Data[ulInx],
						        sizeof(ID_SUPPORT) );

						/* increase the element count if the element in the array was previously unused */
						if ( (bElementOffset + 1) > gOBDResponse[EcuIndex].Sid6MidSupportSize)
						{
							gOBDResponse[EcuIndex].Sid6MidSupportSize = (bElementOffset + 1);
						}
					}
				}
				/*
				 * FALL THROUGH AND TREAT LIKE OTHER PIDS
				 */

				default:
				{
					/* Check if this is a response to an unsupported MID */
					if (IsSid6MidSupported(EcuIndex, RxMsg->Data[HeaderSize + 1]) == FALSE)
					{
						Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  Unsupported SID $6 MID $%02X detected\n",
						     GetEcuId(EcuIndex),
						     RxMsg->Data[HeaderSize + 1] );
					}

					/* Make sure there is enough data in the message to process */
					if (RxMsg->DataSize <= (HeaderSize + 1))
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  Not enough data in SID $6 response message to process\n",
						     GetEcuId(EcuIndex) );
						return(FAIL);
					}

					/* Save the data in the buffer and set the new size */
					if (gOBDList[gOBDListIndex].Protocol == ISO15765)
					{
						/* Determine if there is enough room in the buffer to store the data */
						if ((RxMsg->DataSize - HeaderSize - 1) > sizeof(gOBDResponse[EcuIndex].Sid6Mid) )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $6 response data exceeded buffer size\n",
							     GetEcuId(EcuIndex) );
							return(FAIL);
						}

						memcpy( &gOBDResponse[EcuIndex].Sid6Mid[0],
						        &RxMsg->Data[HeaderSize + 1],
						        (RxMsg->DataSize - HeaderSize - 1));
						gOBDResponse[EcuIndex].Sid6MidSize = (unsigned short)(RxMsg->DataSize - HeaderSize - 1);
					}
					else
					{
						/* If not ISO15765 protocol, make the data look like it */
						/* Check if it is a max or min limit */
						if (RxMsg->Data[HeaderSize + 2] & 0x80)
						{
							/* Move the limit to the minimum location */
							RxMsg->Data[HeaderSize + 7] = RxMsg->Data[HeaderSize + 6];
							RxMsg->Data[HeaderSize + 6] = RxMsg->Data[HeaderSize + 5];

							/* Set the maximum to 0xFFFF */
							RxMsg->Data[HeaderSize + 9] = 0xFF;
							RxMsg->Data[HeaderSize + 8] = 0xFF;
						}
						else
						{
							/* Move the limit to the maximum location */
							RxMsg->Data[HeaderSize + 9] = RxMsg->Data[HeaderSize + 6];
							RxMsg->Data[HeaderSize + 8] = RxMsg->Data[HeaderSize + 5];

							/* Set the minimum to 0x0000 */
							RxMsg->Data[HeaderSize + 7] = 0x00;
							RxMsg->Data[HeaderSize + 6] = 0x00;
						}

						/* Move the value to the new location */
						RxMsg->Data[HeaderSize + 5] = RxMsg->Data[HeaderSize + 4];
						RxMsg->Data[HeaderSize + 4] = RxMsg->Data[HeaderSize + 3];

						/* Set the unit and scaling ID to zero */
						RxMsg->Data[HeaderSize + 3] = 0x00;

						/* Adjust the message size */
						RxMsg->DataSize += 3;

						if ( IsMessageUnique(&gOBDResponse[EcuIndex].Sid6Mid[0],
						                     gOBDResponse[EcuIndex].Sid6MidSize,
						                     &RxMsg->Data[HeaderSize + 1],
						                     (unsigned short)(RxMsg->DataSize - HeaderSize - 1)) == FAIL )
						{
							Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  has already responded\n", GetEcuId(EcuIndex) );
							(*pulNumResponses)--;	// don't count the response
						}
						else
						{
							/* Determine if there is enough room in the buffer to store the data */
							if ((RxMsg->DataSize - HeaderSize - 1) > (sizeof(
								gOBDResponse[EcuIndex].Sid6Mid) - gOBDResponse[EcuIndex].Sid6MidSize))
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  SID $6 response data exceeded buffer size\n",
								     GetEcuId(EcuIndex) );
								return(FAIL);
							}

							memcpy( &gOBDResponse[EcuIndex].Sid6Mid[gOBDResponse[EcuIndex].Sid6MidSize],
							        &RxMsg->Data[HeaderSize + 1],
							        (RxMsg->DataSize - HeaderSize - 1));
							gOBDResponse[EcuIndex].Sid6MidSize += (unsigned short)(RxMsg->DataSize - HeaderSize - 1);
						}
					}
				}
				break;
			}
		}
		break;


		/* SID 7 (Mode 7) */
		case 0x47:
		{
			gOBDResponse[EcuIndex].Sid7Supported = TRUE;

			if (gOBDList[gOBDListIndex].Protocol != ISO15765)
			{
				if ( IsMessageUnique(&gOBDResponse[EcuIndex].Sid7[0],
				                     gOBDResponse[EcuIndex].Sid7Size,
				                     &RxMsg->Data[HeaderSize + 1],
				                     (unsigned short)(RxMsg->DataSize - HeaderSize - 1)) == FAIL )
				{
					Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  has already responded\n", GetEcuId(EcuIndex) );
					(*pulNumResponses)--;	// don't count the response
				}
				else
				{
					/* Determine if there is enough room in the buffer to store the data */
					if ( (RxMsg->DataSize - HeaderSize - 1) >
					     (sizeof(gOBDResponse[EcuIndex].Sid7) - gOBDResponse[EcuIndex].Sid7Size) )
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  SID $7 response data exceeded buffer size\n",
						     GetEcuId(EcuIndex) );
						return(FAIL);
					}

					/* Just copy the DTC bytes */
					memcpy(&gOBDResponse[EcuIndex].Sid7[gOBDResponse[EcuIndex].Sid7Size],
					       &RxMsg->Data[HeaderSize + 1], RxMsg->DataSize - HeaderSize - 1);
					gOBDResponse[EcuIndex].Sid7Size += (unsigned short)(RxMsg->DataSize - HeaderSize - 1);
				}
			}
			else
			{
				// indicate that this ECU has responded
				gOBDResponse[EcuIndex].bResponseReceived = TRUE;

				/* Determine if there is enough room in the buffer to store the data */
				if ( (RxMsg->DataSize - HeaderSize - 1) > sizeof(gOBDResponse[EcuIndex].Sid7) )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					    "ECU %X  SID $7 response data exceeded buffer size\n",
					     GetEcuId(EcuIndex) );
					return(FAIL);
				}

				/* If ISO15765, skip the number of DTCs byte */
				if (RxMsg->DataSize <= (HeaderSize + 2) || (RxMsg->Data[HeaderSize + 1] == 0))
				{
					/* If no DTCs, set to zero */
					memset(&gOBDResponse[EcuIndex].Sid7[0], 0, 2);
					gOBDResponse[EcuIndex].Sid7Size = 2;
				}
				else
				{
					/* Otherwise copy the DTC data */
					memcpy(&gOBDResponse[EcuIndex].Sid7[0],
					       &RxMsg->Data[HeaderSize + 2], (RxMsg->DataSize - HeaderSize - 2));
					gOBDResponse[EcuIndex].Sid7Size = (unsigned short)(RxMsg->DataSize - HeaderSize - 2);
				}

				/* Per J1699 rev 11.6- TC# 6.3 & 7.3 verify that reported number of DTCs
				** matches that of actual DTC count.
				*/
				if ( RxMsg->Data[HeaderSize + 1] != ( (RxMsg->DataSize - HeaderSize - 2) / 2 ) )
				{
					Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
					     "ECU %X  SID $7 DTC Count must match the # of DTC's reported.\n",
					     GetEcuId(EcuIndex) );
					return (FAIL);
				}
			}
		}
		break;


		/* SID 8 (Mode 8) */
		case 0x48:
		{
			// indicate that this ECU has responded
			gOBDResponse[EcuIndex].bResponseReceived = TRUE;

			/* Process message based on PID number */
			switch (RxMsg->Data[HeaderSize + 1])
			{
				case 0x00:
				case 0x20:
				case 0x40:
				case 0x60:
				case 0x80:
				case 0xA0:
				case 0xC0:
				case 0xE0:
					/* Make sure there is enough data in the message to process */
					if (RxMsg->DataSize < (HeaderSize + 1 + sizeof(ID_SUPPORT)))
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  Not enough data in SID $8 support response message to process\n",
						     GetEcuId(EcuIndex) );
						return(FAIL);
					}

					/* Save the data in the buffer and set the new size */
					for ( ulInx = HeaderSize + 1; ulInx < RxMsg->DataSize; ulInx += sizeof(ID_SUPPORT) )
					{
						/* find the element number (for the array of structures) */
						bElementOffset = (unsigned char)(RxMsg->Data[ulInx]) >> 5;

						/* move the data into the element */
						memcpy( &gOBDResponse[EcuIndex].Sid8TidSupport[bElementOffset],
						        &RxMsg->Data[ulInx],
						        sizeof( ID_SUPPORT ));

						/* increase the element count if the element in the array was previously unused */
						if ( (bElementOffset + 1) > gOBDResponse[EcuIndex].Sid8TidSupportSize)
						{
							gOBDResponse[EcuIndex].Sid8TidSupportSize = (bElementOffset + 1);
						}
					}
				/*
				 * FALL THROUGH AND TREAT LIKE OTHER PIDS
				 */

				default:
					/* Check if this is a response to an unsupported TID */
					if (IsSid8TidSupported(EcuIndex, RxMsg->Data[HeaderSize + 1]) == FALSE)
					{
						Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  Unsupported SID $8 TID $%02X detected\n",
						     GetEcuId(EcuIndex),
						     RxMsg->Data[HeaderSize + 1]);
					}

					/* Make sure there is enough data in the message to process */
					if (RxMsg->DataSize <= (HeaderSize + 2))
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  Not enough data in SID $8 response message to process\n",
						     GetEcuId(EcuIndex) );
						return(FAIL);
					}

					if (gOBDList[gOBDListIndex].Protocol == ISO15765)
					{
						/* Determine if there is enough room in the buffer to store the data */
						if ( (RxMsg->DataSize - HeaderSize - 1) > sizeof(gOBDResponse[EcuIndex].Sid8Tid) )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $8 response data exceeded buffer size\n",
							     GetEcuId(EcuIndex) );
							return(FAIL);
						}

						memcpy( &gOBDResponse[EcuIndex].Sid8Tid[0],
						        &RxMsg->Data[HeaderSize + 1], (RxMsg->DataSize - (HeaderSize + 1)) );
						gOBDResponse[EcuIndex].Sid8TidSize = (unsigned short)(RxMsg->DataSize - (HeaderSize + 1));
					}
					else
					{
						/* Determine if there is enough room in the buffer to store the data */
						if ( (RxMsg->DataSize - HeaderSize - 1) >
							 (sizeof(gOBDResponse[EcuIndex].Sid8Tid) - gOBDResponse[EcuIndex].Sid8TidSize) )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $8 response data exceeded buffer size\n",
							     GetEcuId(EcuIndex) );
							return(FAIL);
						}

						memcpy( &gOBDResponse[EcuIndex].Sid8Tid[gOBDResponse[EcuIndex].Sid8TidSize],
						        &RxMsg->Data[HeaderSize + 1], (RxMsg->DataSize - (HeaderSize + 1)) );
						gOBDResponse[EcuIndex].Sid8TidSize += (unsigned short)(RxMsg->DataSize - (HeaderSize + 1));
					}

					break;
			}
		}
		break;


		/* SID 9 (Mode 9) */
		case 0x49:
		{
			if (gOBDList[gOBDListIndex].Protocol == ISO15765)
			{
				// indicate that this ECU has responded
				gOBDResponse[EcuIndex].bResponseReceived = TRUE;
			}

			/* Process message based on PID number */
			switch (RxMsg->Data[HeaderSize + 1])
			{
				case 0x00:
				case 0x20:
				case 0x40:
				case 0x60:
				case 0x80:
				case 0xA0:
				case 0xC0:
				case 0xE0:
				{
					if (gOBDList[gOBDListIndex].Protocol != ISO15765)
					{
						/* Make sure there is enough data in the message to process */
						if (RxMsg->DataSize < (HeaderSize + sizeof(SID9)))
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Not enough data in SID $9 support response message to process.\n", GetEcuId(EcuIndex) );
							return(FAIL);
						}

						/* Save the data in the buffer and set the new size */

						/* find the element number (for the array of structures) */
						bElementOffset = (unsigned char)(RxMsg->Data[HeaderSize + 1]) >> 5;

						/* move the INF into the element */
						gOBDResponse[EcuIndex].Sid9InfSupport[bElementOffset].FirstID = RxMsg->Data[HeaderSize + 1];
						/* move the data into the element ... skipping the message count */
						memcpy( &gOBDResponse[EcuIndex].Sid9InfSupport[bElementOffset].IDBits[0],
						        &RxMsg->Data[HeaderSize + 3], 4);

						/* increase the element count if the element in the array was previously unused */
						if ( (bElementOffset + 1) > gOBDResponse[EcuIndex].Sid9InfSupportSize)
						{
							gOBDResponse[EcuIndex].Sid9InfSupportSize = (bElementOffset + 1);
						}
					}
					else
					{
						ulInx = HeaderSize + 1;

						/* Make sure there is enough data in the message to process */
						if (RxMsg->DataSize < (ulInx + sizeof(ID_SUPPORT)))
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  Not enough data in SID $9 support response message to process.\n", GetEcuId(EcuIndex) );
							return(FAIL);
						}

						/* Save the data in the buffer and set the new size */
						for ( ; ulInx < RxMsg->DataSize; ulInx += sizeof(ID_SUPPORT) )
						{
							/* find the element number (for the array of structures) */
							bElementOffset = (unsigned char)(RxMsg->Data[ulInx]) >> 5;

							/* move the data into the element */
							memcpy( &gOBDResponse[EcuIndex].Sid9InfSupport[bElementOffset],
							        &RxMsg->Data[ulInx],
							        sizeof( ID_SUPPORT ));

							/* increase the element count if the element in the array was previously unused */
							if ( (bElementOffset + 1) > gOBDResponse[EcuIndex].Sid9InfSupportSize)
							{
								gOBDResponse[EcuIndex].Sid9InfSupportSize = (bElementOffset + 1);
							}
						}
					}

				}
				/*
				 * FALL THROUGH AND TREAT LIKE OTHER PIDS
				 */

				default:
				{
					/* Check if this is a response to an unsupported INF */
					if (IsSid9InfSupported(EcuIndex, RxMsg->Data[HeaderSize + 1]) == FALSE)
					{
						Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  Unsupported SID $9 INF $%X detected\n",
						     GetEcuId(EcuIndex),
						     RxMsg->Data[HeaderSize + 1]);
					}

					/* Make sure there is enough data in the message to process */
					if (RxMsg->DataSize <= (HeaderSize + 2))
					{
						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "ECU %X  Not enough data in SID $9 response message to process\n",
						     GetEcuId(EcuIndex) );
						return(FAIL);
					}

					/* Save the data in the buffer and set the new size */
					if (gOBDList[gOBDListIndex].Protocol == ISO15765)
					{
						/* Determine if there is enough room in the buffer to store the data */
						if ( (RxMsg->DataSize - HeaderSize - 1) > sizeof(gOBDResponse[EcuIndex].Sid9Inf)  )
						{
							Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  SID $9 response data exceeded buffer size\n",
							     GetEcuId(EcuIndex) );
							return(FAIL);
						}

						memcpy(&gOBDResponse[EcuIndex].Sid9Inf[0],
						       &RxMsg->Data[HeaderSize + 1], (RxMsg->DataSize - (HeaderSize + 1)));
						gOBDResponse[EcuIndex].Sid9InfSize = (unsigned short)(RxMsg->DataSize - (HeaderSize + 1));

						/* Verify CVNs contain 4 HEX bytes */
						if ( RxMsg->Data[HeaderSize+1] == INF_TYPE_CVN )
						{
							/* HeaderSize is 4; 0x49 0x06 0x(CVN Count) 0x(DATA) ... */
							if ( ( ( RxMsg->DataSize - ( HeaderSize + 3 ) ) % 4 ) != 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  SID $9 INF $6, All CVNs must contain 4 HEX bytes.\n", GetEcuId(EcuIndex) );
							}
						}
						/* Verify CALIDs contain 16 HEX bytes */
						else if ( RxMsg->Data[HeaderSize+1] == INF_TYPE_CALID )
						{
							/* HeaderSize is 4; 0x49 0x04 0x(CALID Count) 0x(DATA) ... */
							if ( ( ( RxMsg->DataSize - ( HeaderSize + 3 ) ) % 16 ) != 0 )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  SID $9 INF $4, All CALIDs must contain 16 HEX bytes.\n", GetEcuId(EcuIndex) );
							}
						}
					}
					else
					{
						if ( IsMessageUnique(&gOBDResponse[EcuIndex].Sid9Inf[0],
						                     gOBDResponse[EcuIndex].Sid9InfSize,
						                     &RxMsg->Data[HeaderSize + 1],
						                     (unsigned short)(RxMsg->DataSize - HeaderSize - 1)) == FAIL )
						{
							Log( WARNING, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
							     "ECU %X  has already responded\n", GetEcuId(EcuIndex) );
							(*pulNumResponses)--;	// don't count the response
						}
						else
						{
							/* Determine if there is enough room in the buffer to store the data */
							if ( (RxMsg->DataSize - HeaderSize - 1) >
								 (sizeof(gOBDResponse[EcuIndex].Sid9Inf) - gOBDResponse[EcuIndex].Sid9InfSize) )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  SID $9 response data exceeded buffer size\n",
								     GetEcuId(EcuIndex) );
								return(FAIL);
							}

							memcpy( &gOBDResponse[EcuIndex].Sid9Inf[gOBDResponse[EcuIndex].Sid9InfSize],
							        &RxMsg->Data[HeaderSize + 1],
							        sizeof(SID9) );
							gOBDResponse[EcuIndex].Sid9InfSize += sizeof(SID9);

							/* Verify CALIDs and CVNs contain 4 HEX bytes */
							if ( ( (RxMsg->Data[HeaderSize+1] == INF_TYPE_CVN) ||
							       (RxMsg->Data[HeaderSize+1] == INF_TYPE_CALID) ) &&
							     ( RxMsg->DataSize   != 0x0A ) )
							{
								Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
								     "ECU %X  SID $9 INF $%X response must contain 4 bytes of HEX data!\n",
								     GetEcuId(EcuIndex),
								     RxMsg->Data[HeaderSize+1]);
							}
						}
					}
				}
				break;
			}
		}
		break;


		/* SID A (Mode A) */
		case 0x4A:
		{
			/*
			 * THE SOFTWARE ASSUMES THAT SID $0A IS ONLY SUPPORTED ON ISO 15765
			 */

			gOBDResponse[EcuIndex].SidASupported = TRUE;

			// indicate that this ECU has responded
			gOBDResponse[EcuIndex].bResponseReceived = TRUE;

			/* Determine if there is enough room in the buffer to store the data */
			if ( (RxMsg->DataSize - HeaderSize - 1) > sizeof(gOBDResponse[EcuIndex].SidA) )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  SID $A response data exceeded buffer size\n",
				     GetEcuId(EcuIndex) );
				return(FAIL);
			}

			/* If ISO15765, skip the number of DTCs byte */
			if (RxMsg->DataSize <= (HeaderSize + 2) || (RxMsg->Data[HeaderSize + 1] == 0))
			{
				/* If no DTCs, set to zero */
				memset(&gOBDResponse[EcuIndex].SidA[0], 0, 2);
				gOBDResponse[EcuIndex].Sid7Size = 2;
			}
			else
			{
				/* Otherwise copy the DTC data */
				memcpy( &gOBDResponse[EcuIndex].SidA[0],
				        &RxMsg->Data[HeaderSize + 2],
				        (RxMsg->DataSize - HeaderSize - 2) );
				gOBDResponse[EcuIndex].SidASize = (unsigned short)(RxMsg->DataSize - HeaderSize - 2);
			}

			/* Verify that reported number of DTCs matches that of actual DTC count. */
			if ( RxMsg->Data[HeaderSize + 1] != ( (RxMsg->DataSize - HeaderSize - 2) / 2 ) )
			{
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  SID $A DTC Count must match the # of DTC's reported.\n",
				     GetEcuId(EcuIndex) );
				return (FAIL);
			}
		}
		break;

		default:
		{
			// indicate that this ECU has responded
			gOBDResponse[EcuIndex].bResponseReceived = TRUE;

			/* Check for a negative response code */
			if (RxMsg->Data[HeaderSize] == NAK)
			{
				Log( INFORMATION, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  Received negative response code\n",
				     GetEcuId(EcuIndex));
				if (RxMsg->Data[HeaderSize + 2] == NAK_RESPONSE_PENDING)
				{
					(*pulNumResponses)--;	// don't count the 0x78 response
				}
			}
			else
			{
				/* Unexpected SID response */
				Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
				     "ECU %X  Unexpected SID $%02X response\n",
				     GetEcuId(EcuIndex),
				     (RxMsg->Data[HeaderSize] - OBD_RESPONSE_BIT));
				return(FAIL);
			}
		}
	}
	return(PASS);
}

/*
********************************************************************************
**
**	FUNCTION    ChkIFRAdjust
**
**	PURPOSE     Account for checksum or IFR in data size.
**
********************************************************************************
*/
void ChkIFRAdjust (PASSTHRU_MSG *RxMsg)
{
	/* If extra data was returned (e.g. checksum, IFR, ...), remove it */
	if ( ( RxMsg->ExtraDataIndex != 0 ) && ( RxMsg->DataSize != RxMsg->ExtraDataIndex ) )
	{
		/* Adjust the data size to ignore the extra data */
		RxMsg->DataSize = RxMsg->ExtraDataIndex;
	}
}

/*
********************************************************************************
**
**	FUNCTION    LookupEcuIndex
**
**	PURPOSE     Return index into gOBDResponse struct for this message
**
********************************************************************************
*/
STATUS LookupEcuIndex (PASSTHRU_MSG *RxMsg, unsigned long *pulEcuIndex)
{
	unsigned long HeaderSize;
	unsigned long EcuIndex;
	unsigned long ByteIndex;
	unsigned long EcuId = 0;

	BOOL bTestFailed = FALSE;

	/* Set the response header size based on the protocol */
	HeaderSize = gOBDList[gOBDListIndex].HeaderSize;

	/* Find the appropriate EcuIndex based on the header information */
	for (EcuIndex = 0; EcuIndex < OBD_MAX_ECUS; EcuIndex++)
	{
		/* Check if we have a header match */
		for (ByteIndex = 0; ByteIndex < HeaderSize; ByteIndex++)
		{
			if (RxMsg->Data[ByteIndex] != gOBDResponse[EcuIndex].Header[ByteIndex])
			{
				/*
				** If ISO14230 protocol,
				** ignore the length bits in the first byte of header
				*/
				if ( (gOBDList[gOBDListIndex].Protocol == ISO14230) &&
				     (ByteIndex == 0))
				{
					if ( (RxMsg->Data[ByteIndex] & 0xC0) !=
					     (gOBDResponse[EcuIndex].Header[ByteIndex] & 0xC0) )
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
		}
		/* If no match, check if EcuIndex is empty */
		if (ByteIndex != HeaderSize)
		{
			if (gOBDResponse[EcuIndex].Header[0] == 0x00 &&
			    gOBDResponse[EcuIndex].Header[1] == 0x00 &&
			    gOBDResponse[EcuIndex].Header[2] == 0x00 &&
			    gOBDResponse[EcuIndex].Header[3] == 0x00)
			{
				/* if not currently in the process of determining the protocol
				 * then check every Rx msg's ID
				 */
				if (gOBDDetermined == TRUE)
				{
					if (VerifyEcuID (&RxMsg->Data[0]) == FAIL)
					{
						/* create the ECU ID for later use */
						for (ByteIndex = 0; ByteIndex < HeaderSize; ByteIndex++)
						{
							EcuId = (EcuId << 8) + (RxMsg->Data[ByteIndex]);
						}

						Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
						     "Response from ECU %X not in initial list\n", EcuId );
						bTestFailed = TRUE;
						break;
					}
				}

				/* If empty, add the new response */
				memcpy(&gOBDResponse[EcuIndex].Header[0], &RxMsg->Data[0], HeaderSize);
				break;
			}
		}
		else
		{
			/* We have a header match */
			break;
		}
	}

	*pulEcuIndex = EcuIndex;

	if ( EcuIndex == OBD_MAX_ECUS )
	{
		Log( FAILURE, SCREENOUTPUTON, LOGOUTPUTON, NO_PROMPT,
		     "Too many OBD ECU responses\n" );
		bTestFailed = TRUE;
	}


	if ( bTestFailed == TRUE )
	{
		return(FAIL);
	}

	return(PASS);
}


/*
********************************************************************************
**
**	FUNCTION    IsMessageUnique
**
**	PURPOSE     Check to see if this is a duplicate message
**
**	RETURNS     PASS - message is unique
**	            FAIL - message is a duplicate
**
********************************************************************************
*/
STATUS IsMessageUnique
          (
            unsigned char *pucBuffer,   /* pointer to buffer */
            unsigned short usSize,      /* size of buffer */
            unsigned char *pucMsg,      /* pointer to message to be checked */
            unsigned short usMsgSize    /* size of message */
          )
{
	unsigned short i;
	unsigned short x;
	BOOL bDone = FALSE;

	for (i = 0, x = 0; (i < usSize) && (x < usMsgSize); i += usMsgSize)
	{
		x = 0;

		while ( (pucBuffer[i + x] == pucMsg[x]) && (x < usMsgSize) && ((i + x) < usSize) )
		{
			x++;
		}

		if (x == usMsgSize)
		{
			bDone = TRUE;
		}
	}

	if (bDone == TRUE)
	{
		return(FAIL);
	}

	return(PASS);
}
