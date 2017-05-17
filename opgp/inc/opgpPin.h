/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __OPGP_PIN_H__
#define __OPGP_PIN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

void opgpPinInit(void);

void opgpPinResetVolatilePinStatus(void);
void opgpPinResetPW1_81_Status(void);

uint16_t opgpPinIsPW1_81_Verified(void);
uint16_t opgpPinIsPW1_82_Verified(void);
uint16_t opgpPinIsPW3Verified(void);

uint16_t opgpPinVerifyPin(uint8_t pinID, uint8_t* input, uint32_t inputLength);
uint16_t opgpPinChangePin(uint8_t pinID, uint8_t* input, uint32_t inputLength);
uint16_t opgpPinUnblockPW1WithRC(uint8_t* input, uint32_t inputLength);
uint16_t opgpPinUnblockPW1(uint8_t* input, uint32_t inputLength);
uint16_t opgpPinSetResettingCode(uint8_t* input, uint32_t inputLength);

#ifdef __cplusplus
}
#endif

#endif /* __OPGP_PIN_H__ */
