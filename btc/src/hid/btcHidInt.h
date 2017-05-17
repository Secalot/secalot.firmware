/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __BTC_HID_INT_H__
#define __BTC_HID_INT_H__

#define BTC_HID_STATE_IDLE (0x9999)
#define BTC_HID_STATE_TRANSACTION_IN_PROGRESS_RECEIVING_REQUEST (0x6666)
#define BTC_HID_STATE_TRANSACTION_IN_PROGRESS_PROCESSING_REQUEST (0xCCCC)
#define BTC_HID_STATE_TRANSACTION_IN_PROGRESS_SENDING_RESPONSE (0x3333)

BTC_MAKE_PACKED(typedef struct)
{
    uint8_t channelIDHi;
    uint8_t channelIDLo;
    uint8_t commandTag;
    uint8_t packetSequenceIndexHi;
    uint8_t packetSequenceIndexLo;

#if (defined(__CC_ARM))
#pragma anon_unions
#endif

    BTC_MAKE_PACKED(union)
    {
        BTC_MAKE_PACKED(struct)
        {
            uint8_t apduLengthHi;
            uint8_t apduLengthLo;
            uint8_t payload[BTC_HID_FRAME_SIZE - 7];
        }
        initialAPDUFrame;
        BTC_MAKE_PACKED(struct) { uint8_t payload[BTC_HID_FRAME_SIZE - 5]; }
        continuationAPDUFrame;
    };
}
BTC_HID_FRAME;

#if (defined(__CC_ARM))
#pragma no_anon_unions
#endif

#define BTC_HID_COMMAND_INVALID (0x00)
#define BTC_HID_COMMAND_APDU (0x05)

#define BTC_HID_DEFAULT_CHANNEL_ID (0x0101)

#endif /* __BTC_HID_INT_H__ */
