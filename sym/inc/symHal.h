/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SYM_HAL_H__
#define __SYM_HAL_H__

#include "mk82Global.h"
#include "mk82KeySafe.h"

#ifdef __cplusplus
extern "C" {
#endif

SYM_MAKE_PACKED(typedef struct)
{
    uint16_t masterKeyInitialized; /* SYM_FALSE16 */
    uint8_t masterKey[SYM_GLOBAL_MASTER_KEY_SIZE];
    uint8_t masterKeyNonce[MK82_KEYSAFE_NONCE_LENGTH];
    uint8_t masterKeyTag[MK82_KEYSAFE_TAG_LENGTH];
}
SYM_HAL_NVM_KEYS;

SYM_MAKE_PACKED(typedef struct)
{
    uint16_t walletState;       /* SYM_GLOBAL_WALLET_STATE_INITIALIZATION */
    uint16_t wipeoutInProgress; /* SYM_FALSE16 */
}
SYM_HAL_NVM_DATA;

void symHalInit(void);
void symHalDeinit(void);

uint16_t symHalMemCmp(uint8_t* array1, uint8_t* array2, uint16_t length);
void symHalMemSet(uint8_t* dst, uint8_t value, uint16_t length);
void symHalMemCpy(uint8_t* dst, uint8_t* src, uint16_t length);

void symHalInitializeWallet(uint8_t* masterKey);

uint16_t symHalGetWalletState(void);
uint16_t symHalIsWipeoutInProgress(void);

void symHalGetRandom(uint8_t* buffer, uint32_t length);

void symHalDerivePublicKey(uint8_t* publicKey, uint8_t* tweak, uint32_t tweakLength);

void symHalSignHash(uint8_t* tweak, uint32_t tweakLength, uint8_t* hash, uint8_t* signature);

void symHalWipeout(void);

void symHalFatalError(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYM_HAL_H__ */
