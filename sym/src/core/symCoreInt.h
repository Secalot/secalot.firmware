/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SYM_CORE_INT_H__
#define __SYM_CORE_INT_H__

#define SYM_CORE_SW_REMAINING_PIN_TRIES (0x63C0)

#define SYM_CORE_CLA (0x80)

#define SYM_CORE_INS_SETUP (0x10)
#define SYM_CORE_INS_GET_WALLET_PUBLIC_KEY (0x20)
#define SYM_CORE_INS_GET_INFO (0x30)
#define SYM_CORE_INS_GET_RANDOM (0x40)
#define SYM_CORE_INS_WIPEOUT (0x50)
#define SYM_CORE_INS_SIGN (0x60)

#define SYM_CORE_P1P2_SETUP (0x0000)
#define SYM_CORE_P1P2_GET_WALLET_PUBLIC_KEY (0x0000)
#define SYM_CORE_P1P2_GET_INFO (0x0000)
#define SYM_CORE_P1P2_GET_RANDOM (0x0000)
#define SYM_CORE_P1P2_WIPEOUT (0x0000)
#define SYM_CORE_P1P2_SIGN (0x0000)

#define SYM_CORE_VERSION_MAJOR (0)
#define SYM_CORE_VERSION_MINOR (1)

#define SYM_CORE_GET_INFO_WALLET_OPERATIONAL (0x01)

#define SYM_CORE_GET_INFO_RESPONSE_LENGTH (8)

#define SYM_CORE_GET_INFO_RFU (0x00)

#define SYM_CORE_AID                                         \
    {                                                        \
        0x53, 0x59, 0x4D, 0x41, 0x50, 0x50, 0x4C, 0x45, 0x54 \
    }
#define SYM_CORE_AID_LENGTH (0x09)

#endif /* __SYM_CORE_INT_H__ */
