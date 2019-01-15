/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __MK82_AS_H__
#define __MK82_AS_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define MK82_AS_ALLOW_OPGP_COMMANDS (0x01)
#define MK82_AS_ALLOW_OTP_COMMANDS (0x02)
#define MK82_AS_ALLOW_BLDR_COMMANDS (0x04)
#define MK82_AS_ALLOW_ETH_COMMANDS (0x08)
#define MK82_AS_ALLOW_SSL_COMMANDS (0x10)
#define MK82_AS_ALLOW_BTC_COMMANDS (0x20)
#define MK82_AS_ALLOW_XRP_COMMANDS (0x40)

#define MK82_AS_ALLOW_ALL_COMMANDS (0xFFFFFFFF)

    void mk82AsInit(void);
    void mk82AsProcessAPDU(uint8_t* apdu, uint32_t* apduLength, uint32_t allowedCommands);

#ifdef __cplusplus
}
#endif

#endif /* __MK82_AS_H__ */
