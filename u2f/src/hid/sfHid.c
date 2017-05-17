/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sfGlobal.h>
#include <sfGlobalInt.h>
#include <sfHid.h>
#include <hid/sfHidInt.h>
#include <sfHal.h>

static void sfHidConstructErrorResponseFrame(uint32_t channelID, SF_HID_FRAME* frame, uint8_t errorCode);
static void sfHidClearState(SF_HID_HANDLE* hidHandle);

void sfHidInit(SF_HID_HANDLE* hidHandle, uint8_t dataBuffer[SF_HID_MAX_DATA_SIZE])
{
    if (hidHandle == NULL)
    {
        sfHalFatalError();
    }

    sfHidClearState(hidHandle);
    hidHandle->dataBuffer = dataBuffer;
}

void sfHidProcessIncomingFrame(SF_HID_HANDLE* hidHandle, uint8_t incomingFrame[SF_HID_FRAME_SIZE],
                               uint8_t immediateOutgoingFrame[SF_HID_FRAME_SIZE],
                               uint16_t* requiredPostFrameProcessingAction, uint16_t* timerAction)
{
    SF_HID_FRAME* incomingHidFrame = (SF_HID_FRAME*)incomingFrame;
    SF_HID_FRAME* immediateOutgoingHidFrame = (SF_HID_FRAME*)immediateOutgoingFrame;

    *timerAction = SF_HID_TIMER_ACTION_DO_NOTHING;

    sfHalMemSet(immediateOutgoingFrame, 0x00, SF_HID_FRAME_SIZE);

    if ((hidHandle == NULL) || (requiredPostFrameProcessingAction == NULL))
    {
        sfHalFatalError();
    }

    if (incomingHidFrame->channelID == SF_HID_CID_RESERVED)
    {
        sfHidConstructErrorResponseFrame(incomingHidFrame->channelID, immediateOutgoingHidFrame,
                                         SF_HID_ERROR_INVALID_CID);

        *requiredPostFrameProcessingAction = SF_HID_ACTION_SEND_IMMEDIATE_OUTGOING_FRAME;

        goto END;
    }

    if (incomingHidFrame->channelID == SF_HID_CID_BROADCAST)
    {
        if (SF_HID_FRAME_COMMAND(incomingHidFrame) != SF_HID_COMMAND_INIT)
        {
            sfHidConstructErrorResponseFrame(incomingHidFrame->channelID, immediateOutgoingHidFrame,
                                             SF_HID_ERROR_INVALID_CID);

            *requiredPostFrameProcessingAction = SF_HID_ACTION_SEND_IMMEDIATE_OUTGOING_FRAME;

            goto END;
        }
    }

    if ((SF_HID_FRAME_TYPE(incomingHidFrame) == SF_HID_FRAME_TYPE_INITIAL) &&
        (SF_HID_FRAME_COMMAND(incomingHidFrame) == SF_HID_COMMAND_INIT))
    {
        uint32_t newChannelID;
        SF_HID_INIT_RESP* responseData = (SF_HID_INIT_RESP*)immediateOutgoingHidFrame->initialFrame.data;

        if (SF_HID_MESSAGE_SIZE(incomingHidFrame) != SF_HID_INIT_NONCE_SIZE)
        {
            sfHidConstructErrorResponseFrame(incomingHidFrame->channelID, immediateOutgoingHidFrame,
                                             SF_HID_ERR_INVALID_MESSAGE_LENGTH);

            *requiredPostFrameProcessingAction = SF_HID_ACTION_SEND_IMMEDIATE_OUTGOING_FRAME;

            goto END;
        }

        if (hidHandle->state != SF_HID_STATE_IDLE)
        {
            if (incomingHidFrame->channelID == hidHandle->currentChannelID)
            {
                *timerAction = SF_HID_TIMER_ACTION_STOP;
                sfHidClearState(hidHandle);
            }
        }

        if (incomingHidFrame->channelID == SF_HID_CID_BROADCAST)
        {
            do
            {
                sfHalGenerateNonSecureRandom((uint8_t*)&newChannelID, sizeof(newChannelID));

            } while ((newChannelID == SF_HID_CID_RESERVED) || (newChannelID == SF_HID_CID_BROADCAST));
        }
        else
        {
            newChannelID = incomingHidFrame->channelID;
        }

        immediateOutgoingHidFrame->channelID = incomingHidFrame->channelID;
        immediateOutgoingHidFrame->initialFrame.command = SF_HID_COMMAND_INIT;
        immediateOutgoingHidFrame->initialFrame.messageSizeHi = 0;
        immediateOutgoingHidFrame->initialFrame.messageSizeLo = SF_HID_INIT_RESP_SIZE;

        sfHalMemCpy(responseData->nonce, incomingHidFrame->initialFrame.data, SF_HID_INIT_NONCE_SIZE);

        responseData->channelID = newChannelID;
        responseData->interfaceVersion = SF_HID_INTERFACE_VERSION;
        responseData->majorVersion = SF_HID_DEVICE_MAJOR_VERSION;
        responseData->minorVersion = SF_HID_DEVICE_MINOR_VERSION;
        responseData->buildVersion = SF_HID_DEVICE_BUILD_VERSION;
        responseData->capabilitiesFlags = 0;

        *requiredPostFrameProcessingAction = SF_HID_ACTION_SEND_IMMEDIATE_OUTGOING_FRAME;

        goto END;
    }

    switch (hidHandle->state)
    {
        case SF_HID_STATE_IDLE:
        {
            if (SF_HID_FRAME_TYPE(incomingHidFrame) != SF_HID_FRAME_TYPE_INITIAL)
            {
                *requiredPostFrameProcessingAction = SF_HID_ACTION_DO_NOTHING;

                goto END;
            }

            switch (SF_HID_FRAME_COMMAND(incomingHidFrame))
            {
                case SF_HID_COMMAND_PING:
                case SF_HID_COMMAND_MSG:
                {
                    uint16_t incomingMessageSize = SF_HID_MESSAGE_SIZE(incomingHidFrame);

                    if (incomingMessageSize > SF_HID_MAX_DATA_SIZE)
                    {
                        sfHidConstructErrorResponseFrame(incomingHidFrame->channelID, immediateOutgoingHidFrame,
                                                         SF_HID_ERR_INVALID_MESSAGE_LENGTH);

                        *requiredPostFrameProcessingAction = SF_HID_ACTION_SEND_IMMEDIATE_OUTGOING_FRAME;

                        goto END;
                    }

                    if (incomingMessageSize > sizeof(incomingHidFrame->initialFrame.data))
                    {
                        sfHalMemCpy(hidHandle->dataBuffer, incomingHidFrame->initialFrame.data,
                                    sizeof(incomingHidFrame->initialFrame.data));

                        sfHidClearState(hidHandle);
                        hidHandle->incomingDataBytesRemainingToReceive =
                            incomingMessageSize - sizeof(incomingHidFrame->initialFrame.data);
                        hidHandle->currentChannelID = incomingHidFrame->channelID;
                        hidHandle->currentSequenceNumber = 0;
                        hidHandle->currentCommandBeingProcessed = SF_HID_FRAME_COMMAND(incomingHidFrame);
                        hidHandle->incomingDataTotalSize = incomingMessageSize;

                        hidHandle->state = SF_HID_STATE_TRANSACTION_IN_PROGRESS_RECEIVING_REQUEST;

                        *timerAction = SF_HID_TIMER_ACTION_START;

                        *requiredPostFrameProcessingAction = SF_HID_ACTION_DO_NOTHING;

                        goto END;
                    }
                    else
                    {
                        sfHalMemCpy(hidHandle->dataBuffer, incomingHidFrame->initialFrame.data, incomingMessageSize);

                        sfHidClearState(hidHandle);
                        hidHandle->currentChannelID = incomingHidFrame->channelID;
                        hidHandle->currentCommandBeingProcessed = SF_HID_FRAME_COMMAND(incomingHidFrame);
                        hidHandle->incomingDataTotalSize = incomingMessageSize;
                        hidHandle->state = SF_HID_STATE_TRANSACTION_IN_PROGRESS_PROCESSING_REQUEST;

                        *requiredPostFrameProcessingAction = SF_HID_ACTION_PROCESS_RECEIVED_COMMAND;

                        goto END;
                    }
                }
                break;
                default:
                {
                    sfHidConstructErrorResponseFrame(incomingHidFrame->channelID, immediateOutgoingHidFrame,
                                                     SF_HID_ERROR_INVALID_COMMAND);

                    *requiredPostFrameProcessingAction = SF_HID_ACTION_SEND_IMMEDIATE_OUTGOING_FRAME;

                    goto END;
                }
                break;
            }
        }
        break;
        case SF_HID_STATE_TRANSACTION_IN_PROGRESS_RECEIVING_REQUEST:
        {
            if (incomingHidFrame->channelID != hidHandle->currentChannelID)
            {
                sfHidConstructErrorResponseFrame(incomingHidFrame->channelID, immediateOutgoingHidFrame,
                                                 SF_HID_ERROR_CHANNEL_BUSY);

                *requiredPostFrameProcessingAction = SF_HID_ACTION_SEND_IMMEDIATE_OUTGOING_FRAME;

                goto END;
            }

            if (SF_HID_FRAME_TYPE(incomingHidFrame) != SF_HID_FRAME_TYPE_CONTINUATION)
            {
                sfHidConstructErrorResponseFrame(incomingHidFrame->channelID, immediateOutgoingHidFrame,
                                                 SF_HID_ERROR_INVALID_SEQUENCE_NUMBER);

                *requiredPostFrameProcessingAction = SF_HID_ACTION_SEND_IMMEDIATE_OUTGOING_FRAME;

                *timerAction = SF_HID_TIMER_ACTION_STOP;
                sfHidClearState(hidHandle);

                goto END;
            }

            if (SF_HID_FRAME_SEQUENCE_NUMBER(incomingHidFrame) != hidHandle->currentSequenceNumber)
            {
                sfHidConstructErrorResponseFrame(incomingHidFrame->channelID, immediateOutgoingHidFrame,
                                                 SF_HID_ERROR_INVALID_SEQUENCE_NUMBER);

                *requiredPostFrameProcessingAction = SF_HID_ACTION_SEND_IMMEDIATE_OUTGOING_FRAME;

                *timerAction = SF_HID_TIMER_ACTION_STOP;
                sfHidClearState(hidHandle);

                goto END;
            }

            if (hidHandle->incomingDataBytesRemainingToReceive > sizeof(incomingHidFrame->continuationFrame.data))
            {
                sfHalMemCpy(&hidHandle->dataBuffer[hidHandle->incomingDataTotalSize -
                                                   hidHandle->incomingDataBytesRemainingToReceive],
                            incomingHidFrame->continuationFrame.data, sizeof(incomingHidFrame->continuationFrame.data));

                hidHandle->incomingDataBytesRemainingToReceive -= sizeof(incomingHidFrame->continuationFrame.data);

                hidHandle->currentSequenceNumber++;

                *requiredPostFrameProcessingAction = SF_HID_ACTION_DO_NOTHING;

                goto END;
            }
            else
            {
                sfHalMemCpy(&hidHandle->dataBuffer[hidHandle->incomingDataTotalSize -
                                                   hidHandle->incomingDataBytesRemainingToReceive],
                            incomingHidFrame->continuationFrame.data, hidHandle->incomingDataBytesRemainingToReceive);

                hidHandle->state = SF_HID_STATE_TRANSACTION_IN_PROGRESS_PROCESSING_REQUEST;

                *requiredPostFrameProcessingAction = SF_HID_ACTION_PROCESS_RECEIVED_COMMAND;

                *timerAction = SF_HID_TIMER_ACTION_STOP;

                goto END;
            }
        }
        break;
        case SF_HID_STATE_TRANSACTION_IN_PROGRESS_PROCESSING_REQUEST:
        {
            sfHidConstructErrorResponseFrame(incomingHidFrame->channelID, immediateOutgoingHidFrame,
                                             SF_HID_ERROR_CHANNEL_BUSY);

            *requiredPostFrameProcessingAction = SF_HID_ACTION_SEND_IMMEDIATE_OUTGOING_FRAME;

            goto END;
        }
        break;
        case SF_HID_STATE_TRANSACTION_IN_PROGRESS_SENDING_RESPONSE:
        {
            sfHidConstructErrorResponseFrame(incomingHidFrame->channelID, immediateOutgoingHidFrame,
                                             SF_HID_ERROR_CHANNEL_BUSY);

            *requiredPostFrameProcessingAction = SF_HID_ACTION_SEND_IMMEDIATE_OUTGOING_FRAME;

            goto END;
        }
        break;
        default:
            sfHalFatalError();
    }

END:;
}

void sfHidSetOutgoingDataLength(SF_HID_HANDLE* hidHandle, uint16_t dataLength)
{
    if (hidHandle->state != SF_HID_STATE_TRANSACTION_IN_PROGRESS_PROCESSING_REQUEST)
    {
        sfHalFatalError();
    }

    if (dataLength > SF_HID_MAX_DATA_SIZE)
    {
        sfHalFatalError();
    }

    hidHandle->outgoingDataTotalSize = dataLength;
}

void sfHidGetIncomingCommandAndDataSize(SF_HID_HANDLE* hidHandle, uint16_t* command, uint16_t* incomingDataSize)
{
    if ((hidHandle == NULL) || (command == NULL) || (incomingDataSize == NULL))
    {
        sfHalFatalError();
    }

    if (hidHandle->state != SF_HID_STATE_TRANSACTION_IN_PROGRESS_PROCESSING_REQUEST)
    {
        sfHalFatalError();
    }

    if (hidHandle->currentCommandBeingProcessed == SF_HID_COMMAND_MSG)
    {
        *command = SF_HID_COMMAND_CODE_APDU;
    }
    else if (hidHandle->currentCommandBeingProcessed == SF_HID_COMMAND_PING)
    {
        *command = SF_HID_COMMAND_CODE_PING;
    }
    else
    {
        sfHalFatalError();
    }

    *incomingDataSize = hidHandle->incomingDataTotalSize;
}

void sfHidProcessOutgoingData(SF_HID_HANDLE* hidHandle, uint8_t outgoingFrame[SF_HID_FRAME_SIZE],
                              uint16_t* moreFramesAvailable)
{
    SF_HID_FRAME* outgoingHidFrame = (SF_HID_FRAME*)outgoingFrame;
    uint16_t outgoingDataTotalSize = hidHandle->outgoingDataTotalSize;

    sfHalMemSet(outgoingFrame, 0x00, SF_HID_FRAME_SIZE);

    if ((hidHandle == NULL) || (moreFramesAvailable == NULL))
    {
        sfHalFatalError();
    }

    if (hidHandle->state == SF_HID_STATE_TRANSACTION_IN_PROGRESS_PROCESSING_REQUEST)
    {
        outgoingHidFrame->channelID = hidHandle->currentChannelID;
        outgoingHidFrame->initialFrame.command = hidHandle->currentCommandBeingProcessed;
        outgoingHidFrame->initialFrame.messageSizeHi = SF_HIBYTE(outgoingDataTotalSize);
        outgoingHidFrame->initialFrame.messageSizeLo = SF_LOBYTE(outgoingDataTotalSize);

        if (outgoingDataTotalSize > sizeof(outgoingHidFrame->initialFrame.data))
        {
            sfHalMemCpy(outgoingHidFrame->initialFrame.data, hidHandle->dataBuffer,
                        sizeof(outgoingHidFrame->initialFrame.data));

            *moreFramesAvailable = SF_TRUE;

            sfHidClearState(hidHandle);
            hidHandle->currentChannelID = outgoingHidFrame->channelID;
            hidHandle->outgoingDataTotalSize = outgoingDataTotalSize;
            hidHandle->outgoingDataBytesRemainingToSend =
                outgoingDataTotalSize - sizeof(outgoingHidFrame->initialFrame.data);
            hidHandle->state = SF_HID_STATE_TRANSACTION_IN_PROGRESS_SENDING_RESPONSE;
        }
        else
        {
            sfHalMemCpy(outgoingHidFrame->initialFrame.data, hidHandle->dataBuffer, outgoingDataTotalSize);

            *moreFramesAvailable = SF_FALSE;

            sfHidClearState(hidHandle);
        }
    }
    else if (hidHandle->state == SF_HID_STATE_TRANSACTION_IN_PROGRESS_SENDING_RESPONSE)
    {
        outgoingHidFrame->channelID = hidHandle->currentChannelID;
        outgoingHidFrame->continuationFrame.sequenceNumber = hidHandle->currentSequenceNumber;
        hidHandle->currentSequenceNumber++;

        if (hidHandle->outgoingDataBytesRemainingToSend > sizeof(outgoingHidFrame->continuationFrame.data))
        {
            sfHalMemCpy(outgoingHidFrame->continuationFrame.data,
                        &hidHandle->dataBuffer[outgoingDataTotalSize - hidHandle->outgoingDataBytesRemainingToSend],
                        sizeof(outgoingHidFrame->continuationFrame.data));

            *moreFramesAvailable = SF_TRUE;

            hidHandle->outgoingDataBytesRemainingToSend -= sizeof(outgoingHidFrame->continuationFrame.data);
        }
        else
        {
            sfHalMemCpy(outgoingHidFrame->continuationFrame.data,
                        &hidHandle->dataBuffer[outgoingDataTotalSize - hidHandle->outgoingDataBytesRemainingToSend],
                        hidHandle->outgoingDataBytesRemainingToSend);

            *moreFramesAvailable = SF_FALSE;

            sfHidClearState(hidHandle);
        }
    }
    else
    {
        sfHalFatalError();
    }
}

void sfHidTimeoutHandler(SF_HID_HANDLE* hidHandle, uint8_t immediateOutgoingFrame[SF_HID_FRAME_SIZE])
{
    sfHidConstructErrorResponseFrame(hidHandle->currentChannelID, (SF_HID_FRAME*)immediateOutgoingFrame,
                                     SF_HID_ERROR_MESSAGE_TIMEOUT);
    sfHidClearState(hidHandle);
}

static void sfHidConstructErrorResponseFrame(uint32_t channelID, SF_HID_FRAME* frame, uint8_t errorCode)
{
    frame->channelID = channelID;
    frame->initialFrame.command = SF_HID_COMMAND_ERROR;
    frame->initialFrame.messageSizeHi = 0;
    frame->initialFrame.messageSizeLo = 0x01;
    frame->initialFrame.data[0] = errorCode;

    return;
}

static void sfHidClearState(SF_HID_HANDLE* hidHandle)
{
    hidHandle->state = SF_HID_STATE_IDLE;
    hidHandle->incomingDataTotalSize = 0;
    hidHandle->incomingDataBytesRemainingToReceive = 0;
    hidHandle->currentChannelID = SF_HID_CID_RESERVED;
    hidHandle->currentSequenceNumber = 0;
    hidHandle->currentCommandBeingProcessed = SF_HID_COMMAND_INVALID;
    hidHandle->outgoingDataTotalSize = 0;
    hidHandle->outgoingDataBytesRemainingToSend = 0;
}
