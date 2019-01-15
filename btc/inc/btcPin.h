/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __BTC_PIN_H__
#define __BTC_PIN_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"

    void btcPinInit(void);
    void btcPinDeinit(void);

    uint16_t btcPinIsPinVerified(void);
    uint16_t btcPinIsPinBlocked(void);

    uint16_t btcPinVerifyPin(uint8_t* input, uint32_t inputLength);
    uint16_t btcPinCheckNewPinAndGetPinHash(uint8_t* input, uint32_t inputLength, uint8_t* pinHash);

#ifdef __cplusplus
}
#endif

#endif /* __BTC_PIN_H__ */
