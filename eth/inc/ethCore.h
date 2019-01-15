/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __ETH_CORE_H__
#define __ETH_CORE_H__

#ifdef __cplusplus
extern "C"
{
#endif

    void ethCoreInit(void);
    void ethCoreDeinit(void);

    void ethCoreGetAID(uint8_t* aid, uint32_t* aidLength);
    void ethCoreProcessAPDU(uint8_t* apdu, uint32_t* apduLength);

#ifdef __cplusplus
}
#endif

#endif /* __ETH_CORE_H__ */
