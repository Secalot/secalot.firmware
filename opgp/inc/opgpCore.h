/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SF_OPGP_CORE_H__
#define __SF_OPGP_CORE_H__

#ifdef __cplusplus
extern "C"
{
#endif

    void opgpCoreInit(void);
    void opgpCoreProcessAPDU(uint8_t* apdu, uint32_t* apduLength);
    void opgpCoreSelect(uint16_t* sw);
    void opgpCoreGetAID(uint8_t* aid, uint32_t* aidLength);
    void opgpCoreGetHistChars(uint8_t* histChars, uint32_t* histCharsLength);

#ifdef __cplusplus
}
#endif

#endif /* __SF_OPGP_CORE_H__ */
