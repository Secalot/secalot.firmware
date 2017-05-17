/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __OPGP_PIN_INT_H__
#define __OPGP_PIN_INT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

typedef struct
{
    uint16_t PW1_81_Verified;
    uint16_t PW1_82_Verified;
    uint16_t PW3Verified;

} OPGP_PIN_VOLATILE_PIN_STATUS;

#ifdef __cplusplus
}
#endif

#endif /* __OPGP_PIN_INT_H__ */
