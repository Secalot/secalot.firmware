/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __OTP_HAL_H__
#define __OTP_HAL_H__

#include "mk82Global.h"
#include "mk82KeySafe.h"

#ifdef __cplusplus
extern "C"
{
#endif

    OTP_MAKE_PACKED(typedef struct) { uint64_t counter; }
    OTP_HAL_NVM_COUNTERS;

    OTP_MAKE_PACKED(typedef struct)
    {
        uint8_t numberOfDigits;
        uint16_t type;
    }
    OTP_HAL_NVM_DATA;

    OTP_MAKE_PACKED(typedef struct)
    {
        uint16_t keyInitialized; /* OTP_FALSE16 */
        uint8_t key[OTP_GLOBAL_MAX_KEYLENGTH];
        uint8_t keyNonce[MK82_KEYSAFE_NONCE_LENGTH];
        uint8_t keyTag[MK82_KEYSAFE_TAG_LENGTH];
        uint32_t keyLength;
    }
    OTP_HAL_NVM_KEYS;

    void otpHalInit(void);
    void otpHalDeinit(void);

    uint16_t otpHalMemCmp(uint8_t* array1, uint8_t* array2, uint16_t length);
    void otpHalMemSet(uint8_t* dst, uint8_t value, uint16_t length);
    void otpHalMemCpy(uint8_t* dst, uint8_t* src, uint16_t length);

    void otpHalSetKeySetTypeAndResetCounter(uint8_t* key, uint32_t keyLength, uint16_t type);
    void otpHalSetOptions(uint8_t numberOfDigits);
    void otpHalGetOptions(uint8_t* numberOfDigits);
    void otpHalIsKeyInitialized(uint16_t* result);
    void otpHalGetKey(uint8_t* key, uint32_t* keyLength);
    void otpHalGetType(uint16_t* type);
    void otpHalGetCounter(uint64_t* counter);
    void otpHalSetCounter(uint64_t counter);
    void otpHalComputeHmac(uint8_t* key, uint32_t keyLength, uint8_t* counter, uint8_t* hmac);
    void otpHalSetExternalTime(uint32_t externalTime);
    uint16_t otpHalGetCurrentTime(uint32_t* time);

    void otpHalWipeout(void);

    void otpHalFatalError(void);

#ifdef __cplusplus
}
#endif

#endif /* __OTP_HAL_H__ */
