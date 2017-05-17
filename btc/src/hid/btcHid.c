/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <btcGlobal.h>
#include <btcGlobalInt.h>
#include <btcHid.h>
#include <hid/btcHidInt.h>
#include <btcHal.h>

static void btcHidClearState(BTC_HID_HANDLE* hidHandle);

void btcHidInit(BTC_HID_HANDLE* hidHandle, uint8_t dataBuffer[BTC_HID_MAX_DATA_SIZE])
{
    if (hidHandle == NULL)
    {
        btcHalFatalError();
    }

    btcHidClearState(hidHandle);
    hidHandle->dataBuffer = dataBuffer;
}

void btcHidProcessIncomingFrame(BTC_HID_HANDLE* hidHandle, uint8_t incomingFrame[BTC_HID_FRAME_SIZE],
                                uint16_t* requiredPostFrameProcessingAction)
{
    BTC_HID_FRAME* incomingHidFrame = (BTC_HID_FRAME*)incomingFrame;
    uint16_t packetSequenceIndex;

    if ((hidHandle == NULL) || (requiredPostFrameProcessingAction == NULL))
    {
        btcHalFatalError();
    }

    if ((hidHandle->state != BTC_HID_STATE_IDLE) &&
        (hidHandle->state != BTC_HID_STATE_TRANSACTION_IN_PROGRESS_RECEIVING_REQUEST))
    {
        *requiredPostFrameProcessingAction = BTC_HID_ACTION_DO_NOTHING;
        goto END;
    }

    if (BTC_MAKEWORD(incomingHidFrame->channelIDLo, incomingHidFrame->channelIDHi) != BTC_HID_DEFAULT_CHANNEL_ID)
    {
        *requiredPostFrameProcessingAction = BTC_HID_ACTION_DO_NOTHING;
        btcHidClearState(hidHandle);
        goto END;
    }

    if (incomingHidFrame->commandTag != BTC_HID_COMMAND_APDU)
    {
        *requiredPostFrameProcessingAction = BTC_HID_ACTION_DO_NOTHING;
        btcHidClearState(hidHandle);
        goto END;
    }

    packetSequenceIndex =
        BTC_MAKEWORD(incomingHidFrame->packetSequenceIndexLo, incomingHidFrame->packetSequenceIndexHi);

    if (hidHandle->state == BTC_HID_STATE_IDLE)
    {
        uint16_t apduLength;

        if (packetSequenceIndex != 0)
        {
            *requiredPostFrameProcessingAction = BTC_HID_ACTION_DO_NOTHING;
            btcHidClearState(hidHandle);
            goto END;
        }

        apduLength = BTC_MAKEWORD(incomingHidFrame->initialAPDUFrame.apduLengthLo,
                                  incomingHidFrame->initialAPDUFrame.apduLengthHi);

        if (apduLength > BTC_HID_MAX_DATA_SIZE)
        {
            btcHalFatalError();
        }

        if (apduLength > sizeof(incomingHidFrame->initialAPDUFrame.payload))
        {
            btcHalMemCpy(hidHandle->dataBuffer, incomingHidFrame->initialAPDUFrame.payload,
                         sizeof(incomingHidFrame->initialAPDUFrame.payload));

            btcHidClearState(hidHandle);
            hidHandle->currentSequenceNumber = 1;
            hidHandle->incomingDataTotalSize = apduLength;
            hidHandle->incomingDataBytesRemainingToReceive =
                apduLength - sizeof(incomingHidFrame->initialAPDUFrame.payload);
            hidHandle->state = BTC_HID_STATE_TRANSACTION_IN_PROGRESS_RECEIVING_REQUEST;

            *requiredPostFrameProcessingAction = BTC_HID_ACTION_DO_NOTHING;
            goto END;
        }
        else
        {
            btcHalMemCpy(hidHandle->dataBuffer, incomingHidFrame->initialAPDUFrame.payload, apduLength);

            btcHidClearState(hidHandle);
            hidHandle->incomingDataTotalSize = apduLength;
            hidHandle->incomingDataBytesRemainingToReceive = 0;
            hidHandle->state = BTC_HID_STATE_TRANSACTION_IN_PROGRESS_PROCESSING_REQUEST;

            *requiredPostFrameProcessingAction = BTC_HID_ACTION_PROCESS_RECEIVED_COMMAND;
            goto END;
        }
    }
    else if (hidHandle->state == BTC_HID_STATE_TRANSACTION_IN_PROGRESS_RECEIVING_REQUEST)
    {
        if (packetSequenceIndex != hidHandle->currentSequenceNumber)
        {
            *requiredPostFrameProcessingAction = BTC_HID_ACTION_DO_NOTHING;
            btcHidClearState(hidHandle);
            goto END;
        }

        if (hidHandle->incomingDataBytesRemainingToReceive > sizeof(incomingHidFrame->continuationAPDUFrame.payload))
        {
            btcHalMemCpy(
                &hidHandle
                     ->dataBuffer[hidHandle->incomingDataTotalSize - hidHandle->incomingDataBytesRemainingToReceive],
                incomingHidFrame->continuationAPDUFrame.payload,
                sizeof(incomingHidFrame->continuationAPDUFrame.payload));

            hidHandle->incomingDataBytesRemainingToReceive -= sizeof(incomingHidFrame->continuationAPDUFrame.payload);

            hidHandle->currentSequenceNumber++;

            *requiredPostFrameProcessingAction = BTC_HID_ACTION_DO_NOTHING;
            goto END;
        }
        else
        {
            btcHalMemCpy(
                &hidHandle
                     ->dataBuffer[hidHandle->incomingDataTotalSize - hidHandle->incomingDataBytesRemainingToReceive],
                incomingHidFrame->continuationAPDUFrame.payload, hidHandle->incomingDataBytesRemainingToReceive);

            hidHandle->state = BTC_HID_STATE_TRANSACTION_IN_PROGRESS_PROCESSING_REQUEST;

            *requiredPostFrameProcessingAction = BTC_HID_ACTION_PROCESS_RECEIVED_COMMAND;

            goto END;
        }
    }
    else
    {
        btcHalFatalError();
    }

END:;
}

void btcHidGetIncomingDataSize(BTC_HID_HANDLE* hidHandle, uint16_t* incomingDataSize)
{
    if ((hidHandle == NULL) || (incomingDataSize == NULL))
    {
        btcHalFatalError();
    }

    if (hidHandle->state != BTC_HID_STATE_TRANSACTION_IN_PROGRESS_PROCESSING_REQUEST)
    {
        btcHalFatalError();
    }

    *incomingDataSize = hidHandle->incomingDataTotalSize;
}

void btcHidSetOutgoingDataLength(BTC_HID_HANDLE* hidHandle, uint16_t dataLength)
{
    if (hidHandle->state != BTC_HID_STATE_TRANSACTION_IN_PROGRESS_PROCESSING_REQUEST)
    {
        btcHalFatalError();
    }

    if (dataLength > BTC_HID_MAX_DATA_SIZE)
    {
        btcHalFatalError();
    }

    hidHandle->outgoingDataTotalSize = dataLength;
}

void btcHidProcessOutgoingData(BTC_HID_HANDLE* hidHandle, uint8_t outgoingFrame[BTC_HID_FRAME_SIZE],
                               uint16_t* moreFramesAvailable)
{
    BTC_HID_FRAME* outgoingHidFrame = (BTC_HID_FRAME*)outgoingFrame;
    uint16_t outgoingDataTotalSize;

    btcHalMemSet(outgoingFrame, 0x00, BTC_HID_FRAME_SIZE);

    if ((hidHandle == NULL) || (moreFramesAvailable == NULL))
    {
        btcHalFatalError();
    }

    outgoingDataTotalSize = hidHandle->outgoingDataTotalSize;

    if (hidHandle->state == BTC_HID_STATE_TRANSACTION_IN_PROGRESS_PROCESSING_REQUEST)
    {
        outgoingHidFrame->channelIDHi = BTC_HIBYTE(BTC_HID_DEFAULT_CHANNEL_ID);
        outgoingHidFrame->channelIDLo = BTC_LOBYTE(BTC_HID_DEFAULT_CHANNEL_ID);
        outgoingHidFrame->commandTag = BTC_HID_COMMAND_APDU;
        outgoingHidFrame->initialAPDUFrame.apduLengthHi = BTC_HIBYTE(outgoingDataTotalSize);
        outgoingHidFrame->initialAPDUFrame.apduLengthLo = BTC_LOBYTE(outgoingDataTotalSize);

        if (outgoingDataTotalSize > sizeof(outgoingHidFrame->initialAPDUFrame.payload))
        {
            btcHalMemCpy(outgoingHidFrame->initialAPDUFrame.payload, hidHandle->dataBuffer,
                         sizeof(outgoingHidFrame->initialAPDUFrame.payload));

            *moreFramesAvailable = BTC_TRUE;

            btcHidClearState(hidHandle);
            hidHandle->currentSequenceNumber = 1;
            hidHandle->outgoingDataTotalSize = outgoingDataTotalSize;
            hidHandle->outgoingDataBytesRemainingToSend =
                outgoingDataTotalSize - sizeof(outgoingHidFrame->initialAPDUFrame.payload);
            hidHandle->state = BTC_HID_STATE_TRANSACTION_IN_PROGRESS_SENDING_RESPONSE;
        }
        else
        {
            btcHalMemCpy(outgoingHidFrame->initialAPDUFrame.payload, hidHandle->dataBuffer, outgoingDataTotalSize);

            *moreFramesAvailable = BTC_FALSE;

            btcHidClearState(hidHandle);
        }
    }
    else if (hidHandle->state == BTC_HID_STATE_TRANSACTION_IN_PROGRESS_SENDING_RESPONSE)
    {
        outgoingHidFrame->channelIDHi = BTC_HIBYTE(BTC_HID_DEFAULT_CHANNEL_ID);
        outgoingHidFrame->channelIDLo = BTC_LOBYTE(BTC_HID_DEFAULT_CHANNEL_ID);
        outgoingHidFrame->commandTag = BTC_HID_COMMAND_APDU;
        outgoingHidFrame->packetSequenceIndexHi = BTC_HIBYTE(hidHandle->currentSequenceNumber);
        outgoingHidFrame->packetSequenceIndexLo = BTC_LOBYTE(hidHandle->currentSequenceNumber);

        hidHandle->currentSequenceNumber++;

        if (hidHandle->outgoingDataBytesRemainingToSend > sizeof(outgoingHidFrame->continuationAPDUFrame.payload))
        {
            btcHalMemCpy(outgoingHidFrame->continuationAPDUFrame.payload,
                         &hidHandle->dataBuffer[outgoingDataTotalSize - hidHandle->outgoingDataBytesRemainingToSend],
                         sizeof(outgoingHidFrame->continuationAPDUFrame.payload));

            *moreFramesAvailable = BTC_TRUE;

            hidHandle->outgoingDataBytesRemainingToSend -= sizeof(outgoingHidFrame->continuationAPDUFrame.payload);
        }
        else
        {
            btcHalMemCpy(outgoingHidFrame->continuationAPDUFrame.payload,
                         &hidHandle->dataBuffer[outgoingDataTotalSize - hidHandle->outgoingDataBytesRemainingToSend],
                         hidHandle->outgoingDataBytesRemainingToSend);

            *moreFramesAvailable = BTC_FALSE;

            btcHidClearState(hidHandle);
        }
    }
    else
    {
        btcHalFatalError();
    }
}

static void btcHidClearState(BTC_HID_HANDLE* hidHandle)
{
    hidHandle->state = BTC_HID_STATE_IDLE;
    hidHandle->incomingDataTotalSize = 0;
    hidHandle->incomingDataBytesRemainingToReceive = 0;
    hidHandle->currentSequenceNumber = 0;
    hidHandle->outgoingDataTotalSize = 0;
    hidHandle->outgoingDataBytesRemainingToSend = 0;
}
