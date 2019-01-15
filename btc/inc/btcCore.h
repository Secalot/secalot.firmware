/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __BTC_CORE_H__
#define __BTC_CORE_H__

#ifdef __cplusplus
extern "C"
{
#endif

    void btcCoreInit(void);
    void btcCoreDeinit(void);

    void btcCoreGetAID(uint8_t* aid, uint32_t* aidLength);
    void btcCoreProcessAPDU(uint8_t* apduBuffer, uint32_t* apduBufferLength);

#ifdef __cplusplus
}
#endif

#endif /* __BTC_CORE_H__ */
