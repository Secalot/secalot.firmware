/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __BLDR_CORE_H__
#define __BLDR_CORE_H__

#ifdef __cplusplus
extern "C" {
#endif

void bldrCoreInit(void);
void bldrCoreDeinit(void);

#ifdef FIRMWARE
void bldrCoreGetAID(uint8_t* aid, uint32_t* aidLength);
#endif /* FIRMWARE */

void bldrCoreProcessAPDU(uint8_t* apdu, uint32_t* apduLength);

#ifdef __cplusplus
}
#endif

#endif /* __BLDR_CORE_H__ */
