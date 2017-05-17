/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __OTP_CORE_H__
#define __OTP_CORE_H__

#ifdef __cplusplus
extern "C" {
#endif

void otpCoreInit(void);
void otpCoreDeinit(void);

void otpCoreGetAID(uint8_t* aid, uint32_t* aidLength);
void otpCoreProcessControlAPDU(uint8_t* apdu, uint32_t* apduLength);
uint16_t otpCoreComputeOtp(uint8_t* otp, uint32_t* otpLength);

#ifdef __cplusplus
}
#endif

#endif /* __OTP_CORE_H__ */
