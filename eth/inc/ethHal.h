/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __ETH_HAL_H__
#define __ETH_HAL_H__

#include "mk82Global.h"
#include "mk82KeySafe.h"

#ifdef __cplusplus
extern "C" {
#endif

ETH_MAKE_PACKED(typedef struct) { uint8_t pinErrorCounter; /* 0 */ }
ETH_HAL_NVM_COUNTERS;

ETH_MAKE_PACKED(typedef struct)
{
    uint16_t masterKeyInitialized; /* ETH_FALSE16 */
    uint8_t masterKey[ETH_GLOBAL_MASTER_KEY_SIZE];
    uint8_t masterKeyNonce[MK82_KEYSAFE_NONCE_LENGTH];
    uint8_t masterKeyTag[MK82_KEYSAFE_TAG_LENGTH];
}
ETH_HAL_NVM_KEYS;

ETH_MAKE_PACKED(typedef struct)
{
    uint16_t walletState; /* ETH_GLOBAL_WALLET_STATE_INITIALIZATION */
    uint8_t pinHash[ETH_GLOBAL_PIN_HASH_LENGTH];
    uint16_t wipeoutInProgress; /* ETH_FALSE16 */
}
ETH_HAL_NVM_DATA;

void ethHalInit(void);
void ethHalDeinit(void);

uint16_t ethHalMemCmp(uint8_t* array1, uint8_t* array2, uint16_t length);
void ethHalMemSet(uint8_t* dst, uint8_t value, uint16_t length);
void ethHalMemCpy(uint8_t* dst, uint8_t* src, uint16_t length);

void ethHalGetPinErrorCounter(uint8_t* errorCounter);
void ethHalSetPinErrorCounter(uint8_t errorCounter);
void ethHalGetPinHash(uint8_t* pinHash);
void ethHalComputePinHash(uint8_t* pin, uint32_t pinLength, uint8_t* pinHash);

void ethHalSetMasterKey(uint8_t* seed, uint32_t seedLength);
void ethHalWriteSetupInfoAndFinalizeSetup(uint8_t* pinHash);

uint16_t ethHalGetWalletState(void);
uint16_t ethHalIsWipeoutInProgress(void);

uint16_t ethHalDerivePublicKey(uint32_t* derivationIndexes, uint32_t numberOfKeyDerivations, uint8_t* publicKey,
                               uint8_t* chainCode);

void ethHalGetRandom(uint8_t* buffer, uint32_t length);

void ethHalHashInit(void);
void ethHalHashUpdate(uint8_t* data, uint32_t dataLength);
void ethHalHashFinal(uint8_t* hash);

uint16_t ethHalSignHash(uint32_t* derivationIndexes, uint32_t numberOfKeyDerivations, uint8_t* hash,
                        uint8_t* signature);

void ethHalWaitForComfirmation(uint16_t* confirmed);

void ethHalWipeout(void);

void ethHalFatalError(void);

#ifdef __cplusplus
}
#endif

#endif /* __ETH_HAL_H__ */
