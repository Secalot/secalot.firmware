/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __XRP_HAL_H__
#define __XRP_HAL_H__

#include "mk82Global.h"
#include "mk82KeySafe.h"

#ifdef __cplusplus
extern "C" {
#endif

XRP_MAKE_PACKED(typedef struct) { uint8_t pinErrorCounter; /* 0 */ }
XRP_HAL_NVM_COUNTERS;

XRP_MAKE_PACKED(typedef struct)
{
    uint16_t privateKeyInitialized; /* XRP_FALSE16 */
    uint8_t privateKey[XRP_GLOBAL_PRIVATE_KEY_SIZE];
    uint8_t privateKeyNonce[MK82_KEYSAFE_NONCE_LENGTH];
    uint8_t privateKeyTag[MK82_KEYSAFE_TAG_LENGTH];
}
XRP_HAL_NVM_KEYS;

XRP_MAKE_PACKED(typedef struct)
{
    uint16_t walletState; /* XRP_GLOBAL_WALLET_STATE_INITIALIZATION */
    uint8_t pinHash[XRP_GLOBAL_PIN_HASH_LENGTH];
    uint16_t wipeoutInProgress; /* XRP_FALSE16 */
}
XRP_HAL_NVM_DATA;

void xrpHalInit(void);
void xrpHalDeinit(void);

uint16_t xrpHalMemCmp(uint8_t* array1, uint8_t* array2, uint16_t length);
void xrpHalMemSet(uint8_t* dst, uint8_t value, uint16_t length);
void xrpHalMemCpy(uint8_t* dst, uint8_t* src, uint16_t length);

void xrpHalGetPinErrorCounter(uint8_t* errorCounter);
void xrpHalSetPinErrorCounter(uint8_t errorCounter);
void xrpHalGetPinHash(uint8_t* pinHash);
void xrpHalComputePinHash(uint8_t* pin, uint32_t pinLength, uint8_t* pinHash);

void xrpHalSetPrivateKey(uint8_t* privateKey);
void xrpHalWriteSetupInfoAndFinalizeSetup(uint8_t* pinHash);

uint16_t xrpHalGetWalletState(void);
uint16_t xrpHalIsWipeoutInProgress(void);

void xrpHalDerivePublicKey(uint8_t* publicKey);

void xrpHalGetRandom(uint8_t* buffer, uint32_t length);

void xrpHalHashInit(void);
void xrpHalHashUpdate(uint8_t* data, uint32_t dataLength);
void xrpHalHashFinal(uint8_t* hash);

void xrpHalSignHash(uint8_t* hash, uint8_t* signature, uint16_t* signatureLength);

void xrpHalWaitForComfirmation(uint16_t* confirmed);
uint64_t xrpHalGetRemainingConfirmationTime(void);

void xrpHalWipeout(void);

void xrpHalFatalError(void);

#ifdef __cplusplus
}
#endif

#endif /* __XRP_HAL_H__ */
