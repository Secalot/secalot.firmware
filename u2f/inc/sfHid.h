/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SF_HID_H__
#define __SF_HID_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SF_HID_MAX_DATA_SIZE (7609)
#define SF_HID_FRAME_SIZE (64)

#define SF_HID_ACTION_DO_NOTHING (0x9999)
#define SF_HID_ACTION_PROCESS_RECEIVED_COMMAND (0x6666)
#define SF_HID_ACTION_SEND_IMMEDIATE_OUTGOING_FRAME (0xCCCC)

#define SF_HID_COMMAND_CODE_APDU (0x9999)
#define SF_HID_COMMAND_CODE_PING (0x6666)

#define SF_HID_TIMER_ACTION_START (0x9999)
#define SF_HID_TIMER_ACTION_STOP (0x6666)
#define SF_HID_TIMER_ACTION_DO_NOTHING (0xCCCC)

#define SF_HID_TIMER_TIMEOUT_IN_MS (800)

typedef struct
{
    uint16_t state;
    uint16_t incomingDataTotalSize;
    uint16_t incomingDataBytesRemainingToReceive;
    uint32_t currentChannelID;
    uint8_t currentSequenceNumber;
    uint8_t currentCommandBeingProcessed;
    uint16_t outgoingDataTotalSize;
    uint16_t outgoingDataBytesRemainingToSend;
    uint8_t* dataBuffer;
} SF_HID_HANDLE;

void sfHidInit(SF_HID_HANDLE* hidHandle, uint8_t dataBuffer[SF_HID_MAX_DATA_SIZE]);

void sfHidProcessIncomingFrame(SF_HID_HANDLE* hidHandle, uint8_t incomingFrame[SF_HID_FRAME_SIZE],
                               uint8_t immediateOutgoingFrame[SF_HID_FRAME_SIZE],
                               uint16_t* requiredPostFrameProcessingAction, uint16_t* timerAction);

void sfHidGetIncomingCommandAndDataSize(SF_HID_HANDLE* hidHandle, uint16_t* command, uint16_t* incomingDataSize);

void sfHidSetOutgoingDataLength(SF_HID_HANDLE* hidHandle, uint16_t dataLength);

void sfHidProcessOutgoingData(SF_HID_HANDLE* hidHandle, uint8_t outgoingFrame[SF_HID_FRAME_SIZE],
                              uint16_t* moreFramesAvailable);

void sfHidTimeoutHandler(SF_HID_HANDLE* hidHandle, uint8_t immediateOutgoingFrame[SF_HID_FRAME_SIZE]);

#ifdef __cplusplus
}
#endif

#endif /* __SF_HID_H__ */
