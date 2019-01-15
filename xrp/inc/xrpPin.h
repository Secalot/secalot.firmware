/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __XRP_PIN_H__
#define __XRP_PIN_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"

    void xrpPinInit(void);
    void xrpPinDeinit(void);

    uint16_t xrpPinIsPinVerified(void);
    uint16_t xrpPinIsPinBlocked(void);

    uint16_t xrpPinVerifyPin(uint8_t* input, uint32_t inputLength);
    uint16_t xrpPinCheckNewPinAndGetPinHash(uint8_t* input, uint32_t inputLength, uint8_t* pinHash);

#ifdef __cplusplus
}
#endif

#endif /* __XRP_PIN_H__ */
