/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __MK82_AS_INT_H__
#define __MK82_AS_INT_H__

#include "stdint.h"

#define MK82_AS_NONE_SELECTED (0x00)
#define MK82_AS_OPGP_SELECTED (0x01)
#define MK82_AS_OTP_SELECTED (0x02)
#define MK82_AS_BLDR_SELECTED (0x03)
#define MK82_AS_ETH_SELECTED (0x04)
#define MK82_AS_SSL_SELECTED (0x05)
#define MK82_AS_BTC_SELECTED (0x06)

#define MK82_AS_MAX_AID_LENGTH (32)

#define MK82_AS_OFFSET_LC (0x04)
#define MK82_AS_OFFSET_DATA (0x05)

#define MK82_AS_APPLICATION_SELECT_HEADER \
    {                                     \
        0x00, 0xA4, 0x04, 0x00            \
    }

#define MK82_AS_SW_UNKNOWN (0x6F00)
#define MK82_AS_SW_REF_DATA_NOT_FOUND (0x6A88)
#define MK82_AS_SW_NO_ERROR (0x9000)

#endif /* __MK82_AS_INT_H__ */
