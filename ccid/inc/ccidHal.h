/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __CCID_HAL_H__
#define __CCID_HAL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

    void ccidhalInit(void);

    void ccidHalDeinit(void);

    void ccidHalMemCpy(uint8_t* dst, uint8_t* src, uint16_t length);

    void ccidHalMemSet(uint8_t* dst, uint8_t value, uint16_t length);

    void ccidHalFatalError(void);

#ifdef __cplusplus
}
#endif

#endif /* __CCID_HAL_H__ */
