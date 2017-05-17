/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __OTP_CORE_INT_H__
#define __OTP_CORE_INT_H__

#define OTP_CORE_SET_OPTONS_NUMBER_OF_DIGITS_OFFSET (0)
#define OTP_CORE_SET_OPTIONS_NUMBER_OF_DIGITS_LENGTH (1)

#define OTP_CORE_SET_EXTERNAL_TIME_TIME_LEMGTH (4)

#define OTP_CORE_CLA (0x80)

#define OTP_CORE_INS_SET_OPTIONS (0x00)
#define OTP_CORE_INS_SET_KEY_AND_TYPE (0x01)
#define OTP_CORE_INS_GET_OPTIONS_AND_TYPE (0x02)
#define OTP_CORE_INS_SET_EXTERNAL_TIME (0x03)

#define OTP_CORE_P1P2_SET_OPTIONS (0x0000)
#define OTP_CORE_P1P2_SET_KEY_AND_TYPE (0x0000)
#define OTP_CORE_P1P2_GET_OPTIONS_AND_TYPE (0x0000)
#define OTP_CORE_P1P2_SET_EXTERNAL_TIME (0x0000)

#define OTP_CORE_AID                                         \
    {                                                        \
        0x4F, 0x54, 0x50, 0x41, 0x50, 0x50, 0x4C, 0x45, 0x54 \
    }
#define OTP_CORE_AID_LENGTH (0x09)

#define OTP_CORE_TYPE_HOTP (0x1)
#define OTP_CORE_TYPE_TOTP (0x2)

#define OTP_CORE_TI (30)

#endif /* __OTP_CORE_INT_H__ */
