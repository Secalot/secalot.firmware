/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __ETH_CORE_INT_H__
#define __ETH_CORE_INT_H__

#define ETH_CORE_SW_REMAINING_PIN_TRIES (0x63C0)

#define ETH_CORE_CLA (0x80)

#define ETH_CORE_INS_SETUP (0x20)
#define ETH_CORE_INS_VERIFY_PIN (0x22)
#define ETH_CORE_INS_GET_WALLET_PUBLIC_KEY (0x40)
#define ETH_CORE_INS_GET_INFO (0xC4)
#define ETH_CORE_INS_GET_RANDOM (0xC0)
#define ETH_CORE_INS_WIPEOUT (0xF0)
#define ETH_CORE_INS_HASH_AND_SIGN (0xF2)

#define ETH_CORE_P1P2_SETUP (0x0000)
#define ETH_CORE_P1P2_VERIFY_PIN (0x0000)
#define ETH_CORE_P1P2_VERIFY_PIN_GET_REMAINING_ATTEMPTS (0x8000)
#define ETH_CORE_P1P2_GET_WALLET_PUBLIC_KEY (0x0000)
#define ETH_CORE_P1P2_GET_INFO (0x0000)
#define ETH_CORE_P1P2_GET_RANDOM (0x0000)
#define ETH_CORE_P1P2_WIPEOUT (0x0000)
#define ETH_CORE_P1P2_HASH_AND_SIGN_INIT_TRANSACTION (0x0000)
#define ETH_CORE_P1P2_HASH_AND_SIGN_INIT_MESSAGE (0x0001)
#define ETH_CORE_P1P2_HASH_AND_SIGN_UPDATE (0x0100)
#define ETH_CORE_P1P2_HASH_AND_SIGN_FINAL (0x0200)

#define ETH_CORE_HASHING_STATE_IDLE (0x9999)
#define ETH_CORE_HASHING_STATE_STARTED (0x6666)

#define ETH_CORE_HASHING_STATE_IDLE (0x9999)
#define ETH_CORE_HASHING_STATE_STARTED (0x6666)

#define ETH_CORE_VERSION_MAJOR (0)
#define ETH_CORE_VERSION_MINOR (1)

#define ETH_CORE_GET_INFO_WALLET_OPERATIONAL (0x01)
#define ETH_CORE_GET_INFO_PIN_VERIFIED (0x02)

#define ETH_CORE_GET_INFO_RESPONSE_LENGTH (8)

#define ETH_CORE_GET_INFO_RFU (0x00)

#define ETH_CORE_AID                                         \
    {                                                        \
        0x45, 0x54, 0x48, 0x41, 0x50, 0x50, 0x4C, 0x45, 0x54 \
    }
#define ETH_CORE_AID_LENGTH (0x09)

#endif /* __ETH_CORE_INT_H__ */
