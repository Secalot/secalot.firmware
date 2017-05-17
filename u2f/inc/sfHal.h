/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SF_HAL_H__
#define __SF_HAL_H__

#ifdef __cplusplus
extern "C" {
#endif

SF_MAKE_PACKED(typedef struct) { uint32_t signatureCounter; }
SF_HAL_NVM_COUNTERS;

void sfhalInit(void);

void sfHalDeinit(void);

void sfhalCheckKeyPresence(uint8_t* keyHandle, uint8_t keyLength, uint8_t* applicationId, uint16_t* keyFound);

void sfHalComputeAuthenticationSignature(uint8_t* keyHandle, uint8_t* applicationId, uint8_t userPresenceByte,
                                         uint8_t* counter, uint8_t* challenge, uint8_t* signature,
                                         uint16_t* signatureLength);

void sfHalComputeRegistrationSignature(uint8_t hashID, uint8_t* applicationId, uint8_t* challenge, uint8_t* keyHandle,
                                       uint8_t* publicKey, uint8_t* signature, uint16_t* signatureLength);

void sfHalGenerateKeyPair(uint8_t* publicKey, uint8_t* applicationId, uint8_t* keyHandle);

void sfhalCheckUserPresence(uint16_t* userPresent);

void sfHalDiscardUserPresence(void);

void sfHalGetAndIncrementACounter(uint32_t* counterValue);

void sfHalGetAttestationCertificate(uint8_t* certificate, uint16_t* certificateLength);

void sfHalMemCpy(uint8_t* dst, uint8_t* src, uint16_t length);

void sfHalMemSet(uint8_t* dst, uint8_t value, uint16_t length);

void sfHalGenerateNonSecureRandom(uint8_t* data, uint16_t length);

void sfHalWipeout(void);

void sfHalFatalError(void);

#ifdef __cplusplus
}
#endif

#endif /* __SF_HAL_H__ */
