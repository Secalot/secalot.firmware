/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdint.h>

#include "ccidCore.h"
#include "ccidCoreInt.h"
#include "ccidGlobal.h"
#include "ccidGlobalInt.h"
#include "ccidClassDescriptor.h"
#include "ccidHal.h"

static const uint8_t ccidCoreAtrBase[] = CCID_CORE_ATR_BASE;

static void ccidCoreConstructErrorMessageAndSetState(CCID_CORE_HANDLE* ccidHandle, uint8_t error);

static void ccidCoreConstructHeaderAndSetState(CCID_CORE_HANDLE* ccidHandle, uint8_t messageType, uint32_t length,
                                               uint8_t messageSpecificByte3);

static void ccidCoreClearState(CCID_CORE_HANDLE* ccidHandle);

void ccidCoreInit(CCID_CORE_HANDLE* ccidHandle, uint8_t dataBuffer[CCID_MAX_MESSAGE_LENGTH], uint8_t* atrHistChars,
                  uint16_t atrHistCharsLength)
{
    uint8_t atrChecksum = 0x00;
    uint16_t i;

    if ((ccidHandle == NULL) || (atrHistChars == NULL))
    {
        ccidHalFatalError();
    }

    ccidhalInit();

    ccidCoreClearState(ccidHandle);

    ccidHandle->dataBuffer = dataBuffer;

    if (atrHistCharsLength > CCID_CORE_ATR_MAX_HISTCHAR_LENGTH)
    {
        atrHistCharsLength = 0;
    }

    if ((atrHistCharsLength + sizeof(ccidCoreAtrBase) + 1) > CCID_CORE_ATR_MAX_LENGTH)
    {
        atrHistCharsLength = 0;
    }

    ccidHalMemCpy(ccidHandle->atr, (void*)ccidCoreAtrBase, sizeof(ccidCoreAtrBase));

    ccidHalMemCpy(&ccidHandle->atr[sizeof(ccidCoreAtrBase)], atrHistChars, atrHistCharsLength);

    ccidHandle->atr[0x01] |= atrHistCharsLength;

    for (i = 1; i < (sizeof(ccidCoreAtrBase) + atrHistCharsLength); i++)
    {
        atrChecksum ^= ccidHandle->atr[i];
    }

    ccidHandle->atr[(sizeof(ccidCoreAtrBase) + atrHistCharsLength)] |= atrChecksum;

    ccidHandle->atrLength = atrHistCharsLength + sizeof(ccidCoreAtrBase) + 1;
}

void ccidCoreProcessIncomingPacket(CCID_CORE_HANDLE* ccidHandle, uint8_t* incomingPacket, uint16_t incomingPacketLength,
                                   uint16_t* requiredPostProcessingAction)
{
    CCID_CORE_MESSAGE* message = (CCID_CORE_MESSAGE*)ccidHandle->dataBuffer;
    uint32_t messageLength;

    if ((ccidHandle == NULL) || (requiredPostProcessingAction == NULL))
    {
        ccidHalFatalError();
    }

    if (incomingPacketLength > CCID_MAX_PACKET_SIZE)
    {
        ccidHalFatalError();
    }

    *requiredPostProcessingAction = CCID_CORE_ACTION_DO_NOTHING;

    if (ccidHandle->state == CCID_CORE_STATE_RECEIVING_COMMAND)
    {
        if ((ccidHandle->totalBytesReceived == 0) && (incomingPacketLength < CCID_MESSAGE_HEADER_SIZE))
        {
            // Ignore command.
            goto END;
        }

        if ((ccidHandle->totalBytesReceived + incomingPacketLength) <= CCID_MAX_MESSAGE_LENGTH)
        {
            ccidHalMemCpy(&ccidHandle->dataBuffer[ccidHandle->totalBytesReceived], incomingPacket,
                          incomingPacketLength);
        }
        else
        {
            if (ccidHandle->totalBytesReceived == 0)
            {
                ccidHalMemCpy(ccidHandle->dataBuffer, incomingPacket, CCID_MESSAGE_HEADER_SIZE);
            }
        }

        messageLength = CCID_CORE_MESSAGE_LENGTH(message);

        ccidHandle->totalBytesReceived += incomingPacketLength;

        if ((incomingPacketLength < CCID_MAX_PACKET_SIZE) ||
            (messageLength == (ccidHandle->totalBytesReceived - CCID_MESSAGE_HEADER_SIZE)))
        {
            if (ccidHandle->totalBytesReceived > CCID_MAX_MESSAGE_LENGTH)
            {
                ccidCoreConstructErrorMessageAndSetState(ccidHandle, CCID_CORE_ERROR_INVALID_LENGTH);

                *requiredPostProcessingAction = CCID_CORE_ACTION_SEND_RESPOSE;

                goto END;
            }
            else if ((messageLength + CCID_MESSAGE_HEADER_SIZE) != ccidHandle->totalBytesReceived)
            {
                ccidCoreConstructErrorMessageAndSetState(ccidHandle, CCID_CORE_ERROR_INVALID_LENGTH);

                *requiredPostProcessingAction = CCID_CORE_ACTION_SEND_RESPOSE;

                goto END;
            }
            else
            {
                if (message->slot != CCID_CORE_SLOT_NUMBER)
                {
                    ccidCoreConstructErrorMessageAndSetState(ccidHandle, CCID_CORE_ERROR_INVALID_LENGTH);

                    *requiredPostProcessingAction = CCID_CORE_ACTION_SEND_RESPOSE;

                    goto END;
                }

                switch (message->messageType)
                {
                    case CCID_CORE_COMMAND_ICC_POWER_ON:
                    {
                        // Do not check the length
                        // Do not check bPowerSelect

                        ccidHalMemCpy(&ccidHandle->dataBuffer[CCID_MESSAGE_HEADER_SIZE], ccidHandle->atr,
                                      ccidHandle->atrLength);

                        ccidCoreConstructHeaderAndSetState(ccidHandle, CCID_CORE_RESPONSE_DATABLOCK,
                                                           ccidHandle->atrLength,
                                                           CCID_CORE_CHAINING_RESPONSE_BEGINS_AND_ENDS_HERE);

                        *requiredPostProcessingAction = CCID_CORE_ACTION_SEND_RESPOSE;

                        goto END;
                    }
                    break;
                    case CCID_CORE_COMMAND_ICC_POWER_OFF:
                    {
                        ccidCoreConstructHeaderAndSetState(ccidHandle, CCID_CORE_RESPONSE_SLOTSTATUS, 0x00,
                                                           CCID_CORE_CLOCK_STATUS_CLOCK_RUNNING);

                        *requiredPostProcessingAction = CCID_CORE_ACTION_SEND_RESPOSE;

                        goto END;
                    }
                    break;
                    case CCID_CORE_COMMAND_ICC_GET_PARAMETERS:
                    {
                        // Do not check the length

                        ccidHandle->dataBuffer[CCID_MESSAGE_HEADER_SIZE] = CCID_CORE_T1_FIDI;
                        ccidHandle->dataBuffer[CCID_MESSAGE_HEADER_SIZE + 1] = CCID_CORE_T1_TCCKS;
                        ccidHandle->dataBuffer[CCID_MESSAGE_HEADER_SIZE + 2] = CCID_CORE_T1_GUARD_TIME;
                        ccidHandle->dataBuffer[CCID_MESSAGE_HEADER_SIZE + 3] = CCID_CORE_T1_WAITING_INTEGERS;
                        ccidHandle->dataBuffer[CCID_MESSAGE_HEADER_SIZE + 4] = CCID_CORE_T1_CLOCK_STOP;
                        ccidHandle->dataBuffer[CCID_MESSAGE_HEADER_SIZE + 5] = CCID_CORE_T1_IFSC;
                        ccidHandle->dataBuffer[CCID_MESSAGE_HEADER_SIZE + 6] = CCID_CORE_T1_NAD;

                        ccidCoreConstructHeaderAndSetState(ccidHandle, CCID_CORE_RESPONSE_PARAMETERS,
                                                           CCID_CORE_T1_PARAMATERS_LENGTH, CCID_CORE_PROTOCOL_TYPE_T1);

                        *requiredPostProcessingAction = CCID_CORE_ACTION_SEND_RESPOSE;

                        goto END;
                    }
                    break;
                    case CCID_CORE_COMMAND_ICC_GET_SLOT_STATUS:
                    {
                        ccidCoreConstructHeaderAndSetState(ccidHandle, CCID_CORE_RESPONSE_SLOTSTATUS, 0x00,
                                                           CCID_CORE_CLOCK_STATUS_CLOCK_RUNNING);

                        *requiredPostProcessingAction = CCID_CORE_ACTION_SEND_RESPOSE;

                        goto END;
                    }
                    break;
                    case CCID_CORE_COMMAND_XFR_BLOCK:
                    {
                        uint16_t levelParameter;

                        // Do not check bBWI

                        levelParameter = CCID_MAKEWORD(message->messageSpecific3, message->messageSpecific2OrError);

                        if (levelParameter != CCID_CORE_LEVEL_APDU_BEGINS_AND_ENDS_HERE)
                        {
                            ccidCoreConstructErrorMessageAndSetState(ccidHandle, CCID_CORE_ERROR_BAD_LEVEL_PARAMETER);
                        }

                        ccidHandle->state = CCID_CORE_STATE_PROCESSING_RECEIVED_APDU;

                        *requiredPostProcessingAction = CCID_CORE_ACTION_PROCESS_RECEIVED_APDU;

                        goto END;
                    }
                    break;
                    default:
                    {
                        ccidCoreConstructErrorMessageAndSetState(ccidHandle, CCID_CORE_ERROR_INVALID_COMMAND);

                        *requiredPostProcessingAction = CCID_CORE_ACTION_SEND_RESPOSE;

                        goto END;
                    }
                    break;
                }
            }
        }
    }
    else
    {
        // Ignore packet.
        goto END;
    }

END:;
}

void ccidCoreGetAPDU(CCID_CORE_HANDLE* ccidHandle, uint8_t** apdu, uint32_t* apduLength)
{
    if ((ccidHandle == NULL) || (apdu == NULL) || (apduLength == NULL))
    {
        ccidHalFatalError();
    }

    if (ccidHandle->state != CCID_CORE_STATE_PROCESSING_RECEIVED_APDU)
    {
        ccidHalFatalError();
    }

    if (ccidHandle->totalBytesReceived < CCID_MESSAGE_HEADER_SIZE)
    {
        ccidHalFatalError();
    }

    *apdu = &ccidHandle->dataBuffer[CCID_MESSAGE_HEADER_SIZE];
    *apduLength = ccidHandle->totalBytesReceived - CCID_MESSAGE_HEADER_SIZE;
}

void ccidCoreGetResponse(CCID_CORE_HANDLE* ccidHandle, uint8_t** response, uint32_t* resposneLength)
{
    if ((ccidHandle == NULL) || (response == NULL) || (resposneLength == NULL))
    {
        ccidHalFatalError();
    }

    if (ccidHandle->state != CCID_CORE_STATE_SENDING_RESPONSE)
    {
        ccidHalFatalError();
    }

    *response = ccidHandle->dataBuffer;
    *resposneLength = ccidHandle->responseLength;
}

void ccidCoreResponseSent(CCID_CORE_HANDLE* ccidHandle)
{
    if (ccidHandle == NULL)
    {
        ccidHalFatalError();
    }

    if (ccidHandle->state != CCID_CORE_STATE_SENDING_RESPONSE)
    {
        ccidHalFatalError();
    }

    ccidCoreClearState(ccidHandle);
}

void ccidCorePrepareResponseAPDU(CCID_CORE_HANDLE* ccidHandle, uint32_t apduLength, uint8_t** response,
                                 uint32_t* responseLength)
{
    CCID_CORE_MESSAGE* message = (CCID_CORE_MESSAGE*)(ccidHandle->dataBuffer);

    if ((ccidHandle == NULL) || (response == NULL) || (responseLength == NULL))
    {
        ccidHalFatalError();
    }

    if (ccidHandle->state != CCID_CORE_STATE_PROCESSING_RECEIVED_APDU)
    {
        ccidHalFatalError();
    }

    if (apduLength > CCID_MAX_APDU_SIZE)
    {
        ccidHalFatalError();
    }

    ccidCoreConstructHeaderAndSetState(ccidHandle, CCID_CORE_RESPONSE_SLOTSTATUS, 0x00,
                                       CCID_CORE_CLOCK_STATUS_CLOCK_RUNNING);

    message->messageType = CCID_CORE_RESPONSE_DATABLOCK;
    message->lengthLL = CCID_LOBYTE(CCID_LOWORD(apduLength));
    message->lengthHL = CCID_HIBYTE(CCID_LOWORD(apduLength));
    message->lengthLH = CCID_LOBYTE(CCID_HIWORD(apduLength));
    message->lengthHH = CCID_HIBYTE(CCID_HIWORD(apduLength));
    // Slot stays the same
    // Sequence number stays the same
    message->messageSpecific1OrStatus = CCID_CORE_STATUS_ICC_PRESENT_AND_ACTIVE | CCID_CORE_STATUS_COMMAND_SUCCEEDED;
    message->messageSpecific2OrError = CCID_CORE_ERROR_NO_ERROR;
    message->messageSpecific3 = CCID_CORE_CHAINING_RESPONSE_BEGINS_AND_ENDS_HERE;

    *response = ccidHandle->dataBuffer;
    *responseLength = (apduLength + CCID_MESSAGE_HEADER_SIZE);
}

void ccidCoreGetWTXRequest(CCID_CORE_HANDLE* ccidHandle, uint8_t** request, uint32_t* requestLength)
{
    CCID_CORE_MESSAGE* message = (CCID_CORE_MESSAGE*)(ccidHandle->dataBuffer);

    if ((ccidHandle == NULL) || (request == NULL) || (requestLength == NULL))
    {
        ccidHalFatalError();
    }

    if (ccidHandle->state != CCID_CORE_STATE_PROCESSING_RECEIVED_APDU)
    {
        ccidHalFatalError();
    }

    message->messageType = CCID_CORE_RESPONSE_DATABLOCK;
    message->lengthLL = 0x00;
    message->lengthLH = 0x00;
    message->lengthHL = 0x00;
    message->lengthHH = 0x00;
    // Slot stays the same
    // Sequence number stays the same
    message->messageSpecific1OrStatus =
        CCID_CORE_STATUS_ICC_PRESENT_AND_ACTIVE | CCID_CORE_STATUS_COMMAND_WTX_REQUESTED;
    message->messageSpecific2OrError = CCID_CORE_ERROR_SET_BWT_MULTIPLIER_TO_ONE;
    message->messageSpecific3 = CCID_CORE_CHAINING_RESPONSE_BEGINS_AND_ENDS_HERE;

    *request = ccidHandle->dataBuffer;
    *requestLength = CCID_MESSAGE_HEADER_SIZE;
}

static void ccidCoreConstructErrorMessageAndSetState(CCID_CORE_HANDLE* ccidHandle, uint8_t error)
{
    CCID_CORE_MESSAGE* message = (CCID_CORE_MESSAGE*)(ccidHandle->dataBuffer);

    // Message type stays the same
    message->lengthLL = 0x00;
    message->lengthLH = 0x00;
    message->lengthHL = 0x00;
    message->lengthHH = 0x00;
    // Slot stays the same
    // Sequence number stays the same
    message->messageSpecific1OrStatus = CCID_CORE_STATUS_ICC_PRESENT_AND_ACTIVE | CCID_CORE_STATUS_COMMAND_FAILED;
    message->messageSpecific2OrError = error;
    message->messageSpecific3 = 0x00;

    ccidHandle->responseLength = CCID_MESSAGE_HEADER_SIZE;
    ccidHandle->state = CCID_CORE_STATE_SENDING_RESPONSE;
}

static void ccidCoreConstructHeaderAndSetState(CCID_CORE_HANDLE* ccidHandle, uint8_t messageType, uint32_t length,
                                               uint8_t messageSpecificByte3)
{
    CCID_CORE_MESSAGE* message = (CCID_CORE_MESSAGE*)(ccidHandle->dataBuffer);

    message->messageType = messageType;
    message->lengthLL = CCID_LOBYTE(CCID_LOWORD(length));
    message->lengthHL = CCID_HIBYTE(CCID_LOWORD(length));
    message->lengthLH = CCID_LOBYTE(CCID_HIWORD(length));
    message->lengthHH = CCID_HIBYTE(CCID_HIWORD(length));
    // Slot stays the same
    // Sequence number stays the same
    message->messageSpecific1OrStatus = CCID_CORE_STATUS_ICC_PRESENT_AND_ACTIVE | CCID_CORE_STATUS_COMMAND_SUCCEEDED;
    message->messageSpecific2OrError = CCID_CORE_ERROR_NO_ERROR;
    message->messageSpecific3 = messageSpecificByte3;

    ccidHandle->responseLength = length + CCID_MESSAGE_HEADER_SIZE;
    ccidHandle->state = CCID_CORE_STATE_SENDING_RESPONSE;
}

static void ccidCoreClearState(CCID_CORE_HANDLE* ccidHandle)
{
    ccidHandle->state = CCID_CORE_STATE_RECEIVING_COMMAND;
    ccidHandle->currentSequenceNumber = 0x00;
    ccidHandle->totalBytesReceived = 0x00;
    ccidHandle->responseLength = 0x00;
}
