/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __XRP_CORE_INT_H__
#define __XRP_CORE_INT_H__

#define XRP_CORE_MAX_VIEWABLE_TRANSACTION_SIZE (8192)

#define XRP_CORE_TRANSACTION_READ_CHUNK_SIZE (128)
#define XRP_CORE_TRANSACTION_READ_NUMBER_OF_CHUNKS \
    (XRP_CORE_MAX_VIEWABLE_TRANSACTION_SIZE / XRP_CORE_TRANSACTION_READ_CHUNK_SIZE)

typedef struct
{
    uint16_t finalized;
    uint16_t transactionTooBigToDisplay;
    uint8_t transaction[XRP_CORE_MAX_VIEWABLE_TRANSACTION_SIZE];
    uint16_t currentOffset;

} XRP_CORE_TRANSACTION_TO_DISPLAY;

#define XRP_CORE_SW_REMAINING_PIN_TRIES (0x63C0)

#define XRP_CORE_CLA (0x80)

#define XRP_CORE_INS_SETUP (0x20)
#define XRP_CORE_INS_VERIFY_PIN (0x22)
#define XRP_CORE_INS_GET_WALLET_PUBLIC_KEY (0x40)
#define XRP_CORE_INS_GET_INFO (0xC4)
#define XRP_CORE_INS_GET_RANDOM (0xC0)
#define XRP_CORE_INS_WIPEOUT (0xF0)
#define XRP_CORE_INS_HASH_AND_SIGN (0xF2)

#define XRP_CORE_INS_READ_TRANSACTION (0xE0)

#define XRP_CORE_P1P2_SETUP_WITH_PRIVATE_KEY (0x0000)
#define XRP_CORE_P1P2_SETUP_WITH_SECRET (0x0001)
#define XRP_CORE_P1P2_VERIFY_PIN (0x0000)
#define XRP_CORE_P1P2_VERIFY_PIN_GET_REMAINING_ATTEMPTS (0x8000)
#define XRP_CORE_P1P2_GET_WALLET_PUBLIC_KEY (0x0000)
#define XRP_CORE_P1P2_GET_INFO (0x0000)
#define XRP_CORE_P1P2_GET_RANDOM (0x0000)
#define XRP_CORE_P1P2_WIPEOUT (0x0000)
#define XRP_CORE_P1P2_HASH_AND_SIGN_INIT (0x0000)
#define XRP_CORE_P1P2_HASH_AND_SIGN_UPDATE (0x0100)
#define XRP_CORE_P1P2_HASH_AND_SIGN_FINAL (0x0200)
#define XRP_CORE_P1P2_READ_TRANSACTION_INFO (0x0000)
#define XRP_CORE_P1P2_READ_TRANSACTION_DATA (0x0100)

#define XRP_CORE_HASHING_STATE_IDLE (0x9999)
#define XRP_CORE_HASHING_STATE_STARTED (0x6666)

#define XRP_CORE_VERSION_MAJOR (0)
#define XRP_CORE_VERSION_MINOR (1)

#define XRP_CORE_GET_INFO_WALLET_OPERATIONAL (0x01)
#define XRP_CORE_GET_INFO_PIN_VERIFIED (0x02)

#define XRP_CORE_GET_INFO_RESPONSE_LENGTH (8)

#define XRP_CORE_GET_INFO_RFU (0x00)

#define XRP_CORE_AID                                         \
    {                                                        \
        0x58, 0x52, 0x50, 0x41, 0x50, 0x50, 0x4C, 0x45, 0x54 \
    }
#define XRP_CORE_AID_LENGTH (0x09)

#define XRP_CORE_TRANSACTION_HEADER_SIZE (0x04)

#endif /* __XRP_CORE_INT_H__ */
