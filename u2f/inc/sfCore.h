/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SF_CORE_H__
#define __SF_CORE_H__

#ifdef __cplusplus
extern "C"
{
#endif

    void sfCoreInit(void);
    void sfCoreDeinit(void);
    void sfCoreProcessAPDU(uint8_t* apduBuffer, uint32_t* apduBufferLength);

#ifdef __cplusplus
}
#endif

#endif /* __SF_CORE_H__ */
