/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __BTC_HID_H__
#define __BTC_HID_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define BTC_HID_MAX_DATA_SIZE (261)
#define BTC_HID_FRAME_SIZE (64)

#define BTC_HID_ACTION_DO_NOTHING (0x9999)
#define BTC_HID_ACTION_PROCESS_RECEIVED_COMMAND (0x6666)

    typedef struct
    {
        uint16_t state;
        uint16_t incomingDataTotalSize;
        uint16_t incomingDataBytesRemainingToReceive;
        uint8_t currentSequenceNumber;
        uint16_t outgoingDataTotalSize;
        uint16_t outgoingDataBytesRemainingToSend;
        uint8_t* dataBuffer;
    } BTC_HID_HANDLE;

    void btcHidInit(BTC_HID_HANDLE* hidHandle, uint8_t dataBuffer[BTC_HID_MAX_DATA_SIZE]);

    void btcHidProcessIncomingFrame(BTC_HID_HANDLE* hidHandle, uint8_t incomingFrame[BTC_HID_FRAME_SIZE],
                                    uint16_t* requiredPostFrameProcessingAction);

    void btcHidGetIncomingDataSize(BTC_HID_HANDLE* hidHandle, uint16_t* incomingDataSize);

    void btcHidSetOutgoingDataLength(BTC_HID_HANDLE* hidHandle, uint16_t dataLength);

    void btcHidProcessOutgoingData(BTC_HID_HANDLE* hidHandle, uint8_t outgoingFrame[BTC_HID_FRAME_SIZE],
                                   uint16_t* moreFramesAvailable);

#ifdef __cplusplus
}
#endif

#endif /* __BTC_HID_H__ */
