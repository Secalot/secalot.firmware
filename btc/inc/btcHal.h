/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __BTC_HAL_H__
#define __BTC_HAL_H__

#include "mk82Global.h"
#include "mk82KeySafe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BTC_HAL_HASH_ID_TRUSTED_INPUT (0x9999)
#define BTC_HAL_HASH_ID_TRANSACTION_SIGNING (0x6666)
#define BTC_HAL_HASH_ID_TRANSACTION_INTEGRITY_CHECK (0xCCCC)
#define BTC_HAL_HASH_ID_MESSAGE_SIGNING (0x3333)

BTC_MAKE_PACKED(typedef struct) { uint8_t pinErrorCounter; /* 0 */ }
BTC_HAL_NVM_COUNTERS;

BTC_MAKE_PACKED(typedef struct)
{
    uint16_t masterKeyInitialized; /* BTC_FALSE16 */
    uint8_t masterKey[BTC_GLOBAL_MASTER_KEY_SIZE];
    uint8_t masterKeyNonce[MK82_KEYSAFE_NONCE_LENGTH];
    uint8_t masterKeyTag[MK82_KEYSAFE_TAG_LENGTH];
    uint16_t trustedInputKeyInitialized; /* BTC_FALSE16 */
    uint8_t trustedInputKey[BTC_GLOBAL_TRUSTED_INPUT_KEY_SIZE];
    uint8_t trustedInputKeyNonce[MK82_KEYSAFE_NONCE_LENGTH];
    uint8_t trustedInputKeyTag[MK82_KEYSAFE_TAG_LENGTH];
}
BTC_HAL_NVM_KEYS;

BTC_MAKE_PACKED(typedef struct)
{
    uint16_t walletState; /* BTC_GLOBAL_WALLET_STATE_INITIALIZATION */
    uint8_t pinHash[BTC_GLOBAL_PIN_HASH_LENGTH];
    uint8_t regularCoinVersion;
    uint8_t p2shCoinVersion;
    uint16_t wipeoutInProgress; /* BTC_FALSE16 */
}
BTC_HAL_NVM_DATA;

void btcHalInit(void);
void btcHalDeinit(void);

void btcHalMemCpy(uint8_t* dst, uint8_t* src, uint16_t length);
void btcHalMemSet(uint8_t* dst, uint8_t value, uint16_t length);
uint16_t btcHalMemCmp(uint8_t* array1, uint8_t* array2, uint16_t length);

void btcHalGetPinErrorCounter(uint8_t* errorCounter);
void btcHalSetPinErrorCounter(uint8_t errorCounter);
void btcHalGetPinHash(uint8_t* pinHash);
void btcHalComputePinHash(uint8_t* pin, uint32_t pinLength, uint8_t* pinHash);

void btcHalGetCoinVersions(uint8_t* regularCoinVersion, uint8_t* p2shCoinVersion);
void btcHalWriteSetupInfoAndFinalizeSetup(uint8_t regularCoinVersion, uint8_t p2shCoinVersion, uint8_t* pinHash);
void btcHalSetMasterKey(uint8_t* seed, uint32_t seedLength);
void btcHalSetRandomTrustedInputKey(void);
uint16_t btcHalGetWalletState(void);
uint16_t btcHalIsWipeoutInProgress(void);

uint16_t btcHalDerivePublicKey(uint32_t* derivationIndexes, uint32_t numberOfKeyDerivations, uint8_t* fullPublicKey,
                               uint8_t* compressedPublicKey, uint8_t* chainCode, uint16_t computeFull,
                               uint16_t computeCompressed);
uint16_t btcHalSignHash(uint32_t* derivationIndexes, uint32_t numberOfKeyDerivations, uint8_t* hash, uint8_t* signature,
                        uint32_t* signatureLength, uint16_t isTransactionSignature);
void btcHalHash160(uint8_t* data, uint32_t dataLength, uint8_t* hash);
void btcHalComputeTrustedInputMAC(uint8_t* blob);
uint16_t btcHalCheckTrustedInputMAC(uint8_t* blob);

void btcHalSha256Start(uint16_t hashID);
void btcHalSha256Update(uint16_t hashID, uint8_t* data, uint32_t dataLength);
void btcHalSha256Finalize(uint16_t hashID, uint8_t* hash);
void btcHalSha256(uint8_t* data, uint32_t dataLength, uint8_t* hash);

void btcHalWaitForComfirmation(uint16_t* confirmed);

void btcHalGenerateNewSeed(uint8_t* seed, uint32_t seedLength);

void btcHalWipeout(void);

void btcHalFatalError(void);

#ifdef __cplusplus
}
#endif

#endif /* __BTC_HAL_H__ */
