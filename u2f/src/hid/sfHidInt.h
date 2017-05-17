/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SF_HID_INT_H__
#define __SF_HID_INT_H__

#define SF_HID_TIMEOUT_VALUE_IN_MS (3000)

#define SF_HID_DEVICE_MAJOR_VERSION (1)
#define SF_HID_DEVICE_MINOR_VERSION (0)
#define SF_HID_DEVICE_BUILD_VERSION (1)

#define SF_HID_STATE_IDLE (0x9999)
#define SF_HID_STATE_TRANSACTION_IN_PROGRESS_RECEIVING_REQUEST (0x6666)
#define SF_HID_STATE_TRANSACTION_IN_PROGRESS_PROCESSING_REQUEST (0xCCCC)
#define SF_HID_STATE_TRANSACTION_IN_PROGRESS_SENDING_RESPONSE (0x3333)

#define SF_HID_CID_BROADCAST (0xffffffff)
#define SF_HID_CID_RESERVED (0x00000000)

#define SF_HID_FRAME_TYPE_MASK (0x80)
#define SF_HID_FRAME_TYPE_INITIAL (0x80)
#define SF_HID_FRAME_TYPE_CONTINUATION (0x00)

#if (defined(__CC_ARM))
#pragma anon_unions
#endif

SF_MAKE_PACKED(typedef struct)
{
    uint32_t channelID;
    SF_MAKE_PACKED(union)
    {
        uint8_t frameType;
        SF_MAKE_PACKED(struct)
        {
            uint8_t command;
            uint8_t messageSizeHi;
            uint8_t messageSizeLo;
            uint8_t data[SF_HID_FRAME_SIZE - 7];
        }
        initialFrame;
        SF_MAKE_PACKED(struct)
        {
            uint8_t sequenceNumber;
            uint8_t data[SF_HID_FRAME_SIZE - 5];
        }
        continuationFrame;
    };
}
SF_HID_FRAME;

#if (defined(__CC_ARM))
#pragma no_anon_unions
#endif

#define SF_HID_FRAME_TYPE(f) ((f)->frameType & SF_HID_FRAME_TYPE_MASK)
#define SF_HID_FRAME_COMMAND(f) ((f)->initialFrame.command)
#define SF_HID_MESSAGE_SIZE(f) ((f)->initialFrame.messageSizeHi * 256 + (f)->initialFrame.messageSizeLo)
#define SF_HID_FRAME_SEQUENCE_NUMBER(f) ((f)->continuationFrame.sequenceNumber & ~SF_HID_FRAME_TYPE_MASK)

#define SF_HID_INTERFACE_VERSION (2)
#define SF_HID_TRANSMISSION_TIMEOUT (3000)

#define SF_HID_COMMAND_INVALID (SF_HID_FRAME_TYPE_INITIAL | 0x00)

#define SF_HID_COMMAND_PING (SF_HID_FRAME_TYPE_INITIAL | 0x01)
#define SF_HID_COMMAND_MSG (SF_HID_FRAME_TYPE_INITIAL | 0x03)
#define SF_HID_COMMAND_LOCK (SF_HID_FRAME_TYPE_INITIAL | 0x04)
#define SF_HID_COMMAND_INIT (SF_HID_FRAME_TYPE_INITIAL | 0x06)
#define SF_HID_COMMAND_WINK (SF_HID_FRAME_TYPE_INITIAL | 0x08)
#define SF_HID_COMMAND_SYNC (SF_HID_FRAME_TYPE_INITIAL | 0x3c)
#define SF_HID_COMMAND_ERROR (SF_HID_FRAME_TYPE_INITIAL | 0x3f)

#define SF_HID_INIT_NONCE_SIZE (8)
#define SF_HID_SYNC_NONCE_SIZE (1)
#define SF_HID_CAPABILITIES_FLAG_WINK (0x01)

typedef struct
{
    uint8_t nonce[SF_HID_INIT_NONCE_SIZE];
} SF_HID_INIT_REQEST;

typedef struct
{
    uint8_t nonce[SF_HID_INIT_NONCE_SIZE];
    uint32_t channelID;
    uint8_t interfaceVersion;
    uint8_t majorVersion;
    uint8_t minorVersion;
    uint8_t buildVersion;
    uint8_t capabilitiesFlags;
} SF_HID_INIT_RESP;

#define SF_HID_INIT_RESP_SIZE (17)

typedef struct
{
    uint8_t nonce;
} SF_HID_SYNC_REQUEST;

typedef struct
{
    uint8_t nonce;
} SF_HID_SYNC_RESPONCE;

#define SF_HID_ERROR_SUCCESS 0x00
#define SF_HID_ERROR_INVALID_COMMAND 0x01
#define SF_HID_ERROR_INVALID_PARAMETER 0x02
#define SF_HID_ERR_INVALID_MESSAGE_LENGTH 0x03
#define SF_HID_ERROR_INVALID_SEQUENCE_NUMBER 0x04
#define SF_HID_ERROR_MESSAGE_TIMEOUT 0x05
#define SF_HID_ERROR_CHANNEL_BUSY 0x06
#define SF_HID_ERROR_LOCK_REQUIRED 0x0a
#define SF_HID_ERROR_INVALID_CID 0x0b
#define SF_HID_ERROR_OTHER 0x7f

#endif /* __SF_HID_INT_H__ */
