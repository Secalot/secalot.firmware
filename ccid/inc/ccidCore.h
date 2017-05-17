/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SF_CCID_CORE_H__
#define __SF_CCID_CORE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <ccidGlobal.h>

#define CCID_CORE_ACTION_DO_NOTHING (0x9999)
#define CCID_CORE_ACTION_PROCESS_RECEIVED_APDU (0x6666)
#define CCID_CORE_ACTION_SEND_RESPOSE (0xCCCC)

#define CCID_CORE_ATR_MAX_LENGTH (32)

typedef struct
{
    uint16_t state;
    uint8_t currentSequenceNumber;
    uint8_t* dataBuffer;
    uint32_t responseLength;
    uint32_t totalBytesReceived;
    uint8_t atr[CCID_CORE_ATR_MAX_LENGTH];
    uint8_t atrLength;
} CCID_CORE_HANDLE;

void ccidCoreInit(CCID_CORE_HANDLE* ccidHandle, uint8_t dataBuffer[CCID_MAX_MESSAGE_LENGTH], uint8_t* atrHistChars,
                  uint16_t atrHistCharsLength);

void ccidCoreProcessIncomingPacket(CCID_CORE_HANDLE* ccidHandle, uint8_t* incomingPacket, uint16_t incomingPacketLength,
                                   uint16_t* requiredPostProcessingAction);

void ccidCoreGetAPDU(CCID_CORE_HANDLE* ccidHandle, uint8_t** apdu, uint32_t* apduLength);

void ccidCoreGetResponse(CCID_CORE_HANDLE* ccidHandle, uint8_t** response, uint32_t* resposneLength);

void ccidCoreGetWTXRequest(CCID_CORE_HANDLE* ccidHandle, uint8_t** request, uint32_t* requestLength);

void ccidCoreResponseSent(CCID_CORE_HANDLE* ccidHandle);

void ccidCorePrepareResponseAPDU(CCID_CORE_HANDLE* ccidHandle, uint32_t apduLength, uint8_t** response,
                                 uint32_t* resposneLength);

#ifdef __cplusplus
}
#endif

#endif /* __SF_CCID_CORE_H__ */
