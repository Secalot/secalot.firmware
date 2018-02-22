/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ethGlobal.h>
#include <ethGlobalInt.h>
#include <ethHal.h>
#include <hal/k82/ethHalInt.h>

#include "mk82Global.h"
#include "mk82System.h"
#include "mk82KeySafe.h"
#include "mk82Fs.h"
#include "mk82Bip32.h"
#ifdef USE_BUTTON
#include <mk82Button.h>
#endif
#ifdef USE_TOUCH
#include <mk82Touch.h>
#endif

#include "mbedtls/sha256.h"
#include "mbedtls/md.h"
#include "mbedtls/sha256.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/ecp.h"

#include "sha3.h"

static uint16_t ethHalIsMasterKeyInitialized(void);
static void ethHalGetMasterKey(uint8_t* masterKey);
static void ethHalButtonPressedCallback(void);

static sha3_context ethHalHashContext;

static uint16_t ethHalButtonPressed;

void ethHalInit(void) { ethHalButtonPressed = ETH_FALSE; }

void ethHalDeinit(void) {}

uint16_t ethHalMemCmp(uint8_t* array1, uint8_t* array2, uint16_t length)
{
    uint16_t comparisonResult;

    comparisonResult = mk82SystemMemCmp(array1, array2, length);

    if (comparisonResult != MK82_TRUE)
    {
        return ETH_CMP_NOT_EQUAL;
    }
    else
    {
        return ETH_CMP_EQUAL;
    }
}

void ethHalMemSet(uint8_t* dst, uint8_t value, uint16_t length) { mk82SystemMemSet(dst, value, length); }

void ethHalMemCpy(uint8_t* dst, uint8_t* src, uint16_t length) { mk82SystemMemCpy(dst, src, length); }

void ethHalGetPinErrorCounter(uint8_t* errorCounter)
{
    if (errorCounter == NULL)
    {
        ethHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_ETH_COUNTERS, offsetof(ETH_HAL_NVM_COUNTERS, pinErrorCounter), errorCounter,
                   sizeof(uint8_t));
}

void ethHalSetPinErrorCounter(uint8_t errorCounter)
{
    mk82FsWriteFile(MK82_FS_FILE_ID_ETH_COUNTERS, offsetof(ETH_HAL_NVM_COUNTERS, pinErrorCounter), &errorCounter,
                    sizeof(uint8_t));
    mk82FsCommitWrite(MK82_FS_FILE_ID_ETH_COUNTERS);
}

void ethHalGetPinHash(uint8_t* pinHash)
{
    if (pinHash == NULL)
    {
        ethHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_ETH_DATA, offsetof(ETH_HAL_NVM_DATA, pinHash), pinHash, ETH_GLOBAL_PIN_HASH_LENGTH);
}

void ethHalComputePinHash(uint8_t* pin, uint32_t pinLength, uint8_t* pinHash)
{
    if ((pin == NULL) || (pinHash == NULL))
    {
        ethHalFatalError();
    }

    mbedtls_sha256(pin, pinLength, pinHash, 0);
}

void ethHalSetMasterKey(uint8_t* seed, uint32_t seedLength)
{
    uint8_t masterKey[ETH_GLOBAL_MASTER_KEY_SIZE];
    uint8_t encryptedMasterKey[ETH_GLOBAL_MASTER_KEY_SIZE];
    uint8_t nonce[MK82_KEYSAFE_NONCE_LENGTH];
    uint8_t tag[MK82_KEYSAFE_TAG_LENGTH];
    uint16_t trueFalse;

    if (seed == NULL)
    {
        ethHalFatalError();
    }

    if ((seedLength < ETH_GLOBAL_SEED_MINIMUM_LENGTH) || (seedLength > ETH_GLOBAL_SEED_MAXIMUM_LENGTH))
    {
        ethHalFatalError();
    }

    int calleeRetVal;

    calleeRetVal = mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA512), (uint8_t*)ETH_HAL_HMAC_KEY,
                                   ETH_HAL_HMAC_KEY_LENGTH, seed, seedLength, masterKey);

    if (calleeRetVal != 0)
    {
        ethHalFatalError();
    }

    mk82KeysafeWrapKey(MK82_KEYSAFE_CCR_KEK_ID, masterKey, sizeof(masterKey), encryptedMasterKey, NULL, 0, nonce, tag);

    mk82SystemMemSet(masterKey, 0x00, sizeof(masterKey));

    mk82FsWriteFile(MK82_FS_FILE_ID_ETH_KEYS, offsetof(ETH_HAL_NVM_KEYS, masterKey), encryptedMasterKey,
                    ETH_GLOBAL_MASTER_KEY_SIZE);
    mk82FsWriteFile(MK82_FS_FILE_ID_ETH_KEYS, offsetof(ETH_HAL_NVM_KEYS, masterKeyNonce), nonce,
                    MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_ETH_KEYS, offsetof(ETH_HAL_NVM_KEYS, masterKeyTag), tag, MK82_KEYSAFE_TAG_LENGTH);
    trueFalse = ETH_TRUE;
    mk82FsWriteFile(MK82_FS_FILE_ID_ETH_KEYS, offsetof(ETH_HAL_NVM_KEYS, masterKeyInitialized), (uint8_t*)&trueFalse,
                    sizeof(trueFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_ETH_KEYS);
}

void ethHalWriteSetupInfoAndFinalizeSetup(uint8_t* pinHash)
{
    uint8_t errorCounter = ETH_GLOBAL_PIN_INITIAL_ERROR_COUNTER_VALUE;
    uint16_t walletState = ETH_GLOBAL_WALLET_STATE_INITIALIZATION;

    if (pinHash == NULL)
    {
        ethHalFatalError();
    }

    mk82FsWriteFile(MK82_FS_FILE_ID_ETH_COUNTERS, offsetof(ETH_HAL_NVM_COUNTERS, pinErrorCounter), &errorCounter,
                    sizeof(uint8_t));
    mk82FsCommitWrite(MK82_FS_FILE_ID_ETH_COUNTERS);

    mk82FsWriteFile(MK82_FS_FILE_ID_ETH_DATA, offsetof(ETH_HAL_NVM_DATA, pinHash), pinHash, ETH_GLOBAL_PIN_HASH_LENGTH);
    walletState = ETH_GLOBAL_WALLET_STATE_OPERATIONAL;
    mk82FsWriteFile(MK82_FS_FILE_ID_ETH_DATA, offsetof(ETH_HAL_NVM_DATA, walletState), (uint8_t*)&walletState,
                    sizeof(walletState));

    mk82FsCommitWrite(MK82_FS_FILE_ID_ETH_DATA);
}

uint16_t ethHalGetWalletState(void)
{
    uint16_t walletState = ETH_GLOBAL_WALLET_STATE_INITIALIZATION;

    mk82FsReadFile(MK82_FS_FILE_ID_ETH_DATA, offsetof(ETH_HAL_NVM_DATA, walletState), (uint8_t*)&walletState,
                   sizeof(walletState));

    return walletState;
}

uint16_t ethHalIsWipeoutInProgress(void)
{
    uint16_t wipeoutInProgress = ETH_TRUE;

    mk82FsReadFile(MK82_FS_FILE_ID_ETH_DATA, offsetof(ETH_HAL_NVM_DATA, wipeoutInProgress),
                   (uint8_t*)&wipeoutInProgress, sizeof(wipeoutInProgress));

    return wipeoutInProgress;
}

void ethHalGetRandom(uint8_t* buffer, uint32_t length)
{
    if (buffer == NULL)
    {
        ethHalFatalError();
    }

    mk82SystemGetRandom(buffer, length);
}

static uint16_t ethHalIsMasterKeyInitialized(void)
{
    uint16_t masterKeyInitialized = ETH_FALSE;
    uint16_t retVal = ETH_FALSE;

    mk82FsReadFile(MK82_FS_FILE_ID_ETH_KEYS, offsetof(ETH_HAL_NVM_KEYS, masterKeyInitialized),
                   (uint8_t*)&masterKeyInitialized, sizeof(masterKeyInitialized));

    if (masterKeyInitialized != ETH_TRUE)
    {
        retVal = ETH_FALSE;
    }
    else
    {
        retVal = ETH_TRUE;
    }

    return retVal;
}

static void ethHalGetMasterKey(uint8_t* masterKey)
{
    uint8_t encryptedMasterKey[ETH_GLOBAL_MASTER_KEY_SIZE];
    uint8_t nonce[MK82_KEYSAFE_NONCE_LENGTH];
    uint8_t tag[MK82_KEYSAFE_TAG_LENGTH];
    uint16_t masterKeyInitialized = ETH_FALSE;

    masterKeyInitialized = ethHalIsMasterKeyInitialized();

    if (masterKeyInitialized != ETH_TRUE)
    {
        ethHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_ETH_KEYS, offsetof(ETH_HAL_NVM_KEYS, masterKey), encryptedMasterKey,
                   ETH_GLOBAL_MASTER_KEY_SIZE);
    mk82FsReadFile(MK82_FS_FILE_ID_ETH_KEYS, offsetof(ETH_HAL_NVM_KEYS, masterKeyNonce), nonce,
                   MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsReadFile(MK82_FS_FILE_ID_ETH_KEYS, offsetof(ETH_HAL_NVM_KEYS, masterKeyTag), tag, MK82_KEYSAFE_TAG_LENGTH);

    mk82KeysafeUnwrapKey(MK82_KEYSAFE_CCR_KEK_ID, encryptedMasterKey, sizeof(encryptedMasterKey), masterKey, NULL, 0,
                         nonce, tag);
}

uint16_t ethHalDerivePublicKey(uint32_t* derivationIndexes, uint32_t numberOfKeyDerivations, uint8_t* publicKey,
                               uint8_t* chainCode)
{
    uint16_t retVal = MK82_BIP32_GENERAL_ERROR;

    retVal = mk82Bip32DerivePublicKey(derivationIndexes, numberOfKeyDerivations, publicKey, NULL, chainCode, MK82_TRUE,
                                      MK82_FALSE, ethHalGetMasterKey);

    if (retVal != MK82_BIP32_NO_ERROR)
    {
        if (retVal == MK82_BIP32_KEY_DERIVATION_ERROR)
        {
            retVal = ETH_KEY_DERIVATION_ERROR;
        }
        else
        {
            retVal = ETH_GENERAL_ERROR;
        }
    }
    else
    {
        retVal = ETH_NO_ERROR;
    }

    return retVal;
}

void ethHalHashInit(void) { sha3_Init256(&ethHalHashContext); }

void ethHalHashUpdate(uint8_t* data, uint32_t dataLength)
{
    if (data == NULL)
    {
        ethHalFatalError();
    }

    sha3_Update(&ethHalHashContext, data, dataLength);
}

void ethHalHashFinal(uint8_t* hash)
{
    uint8_t* hashInternal;

    if (hash == NULL)
    {
        ethHalFatalError();
    }

    hashInternal = (uint8_t*)sha3_Finalize(&ethHalHashContext);

    mk82SystemMemCpy(hash, hashInternal, ETH_GLOBAL_KECCAK_256_HASH_SIZE);
}

uint16_t ethHalSignHash(uint32_t* derivationIndexes, uint32_t numberOfKeyDerivations, uint8_t* hash, uint8_t* signature)
{
    uint8_t privateKey[ETH_GLOBAL_PRIVATE_KEY_SIZE];
    uint8_t chainCode[ETH_GLOBAL_CHAIN_CODE_SIZE];
    uint16_t calleeRetVal = ETH_GENERAL_ERROR;
    uint16_t retVal = ETH_GENERAL_ERROR;
    int tlsCalleeRetVal = -1;
    mbedtls_ecdsa_context ecsdaContext;
    mbedtls_mpi r;
    mbedtls_mpi s;
    mbedtls_mpi nDivBy2;
    uint8_t nDivBy2Array[] = {0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                              0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5D, 0x57, 0x6E, 0x73, 0x57, 0xA4,
                              0x50, 0x1D, 0xDF, 0xE9, 0x2F, 0x46, 0x68, 0x1B, 0x20, 0xA0};
    int rYSign;

    if ((derivationIndexes == NULL) || (hash == NULL) || (signature == NULL))
    {
        ethHalFatalError();
    }

    mbedtls_ecdsa_init(&ecsdaContext);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);
    mbedtls_mpi_init(&nDivBy2);

    calleeRetVal =
        mk82Bip32DerivePrivateKey(derivationIndexes, numberOfKeyDerivations, privateKey, chainCode, ethHalGetMasterKey);

    if (calleeRetVal != MK82_BIP32_NO_ERROR)
    {
        if (calleeRetVal == MK82_BIP32_KEY_DERIVATION_ERROR)
        {
            retVal = ETH_KEY_DERIVATION_ERROR;
            goto END;
        }
        else
        {
            ethHalFatalError();
        }
    }

    tlsCalleeRetVal = mbedtls_ecp_group_load(&ecsdaContext.grp, MBEDTLS_ECP_DP_SECP256K1);

    if (tlsCalleeRetVal != 0)
    {
        ethHalFatalError();
    }

    tlsCalleeRetVal = mbedtls_mpi_read_binary(&ecsdaContext.d, privateKey, ETH_GLOBAL_PRIVATE_KEY_SIZE);

    if (tlsCalleeRetVal != 0)
    {
        ethHalFatalError();
    }

    tlsCalleeRetVal = mbedtls_ecdsa_sign(&ecsdaContext.grp, &r, &s, &ecsdaContext.d, hash,
                                         ETH_GLOBAL_KECCAK_256_HASH_SIZE, &rYSign, mk82SystemGetRandomForTLS, NULL);

    if (tlsCalleeRetVal != 0)
    {
        ethHalFatalError();
    }

    if (rYSign != 0)
    {
        signature[0] = 0x01;
    }
    else
    {
        signature[0] = 0x00;
    }

    tlsCalleeRetVal = mbedtls_mpi_read_binary(&nDivBy2, nDivBy2Array, sizeof(nDivBy2Array));

    if (tlsCalleeRetVal != 0)
    {
        ethHalFatalError();
    }

    tlsCalleeRetVal = mbedtls_mpi_cmp_abs(&s, &nDivBy2);

    if (tlsCalleeRetVal == 1)
    {
        tlsCalleeRetVal = mbedtls_mpi_sub_abs(&s, &(ecsdaContext.grp.N), &s);

        if (tlsCalleeRetVal != 0)
        {
            ethHalFatalError();
        }

        if (signature[0] == 0x01)
        {
            signature[0] = 0x00;
        }
        else
        {
            signature[0] = 0x01;
        }
    }

    tlsCalleeRetVal = mbedtls_mpi_write_binary(&r, signature + ETH_HAL_V_LENGTH, ETH_HAL_R_S_LENGTH);

    if (tlsCalleeRetVal != 0)
    {
        ethHalFatalError();
    }

    tlsCalleeRetVal =
        mbedtls_mpi_write_binary(&s, signature + ETH_HAL_V_LENGTH + ETH_HAL_R_S_LENGTH, ETH_HAL_R_S_LENGTH);

    if (tlsCalleeRetVal != 0)
    {
        ethHalFatalError();
    }

    retVal = ETH_NO_ERROR;

END:
    mbedtls_ecdsa_free(&ecsdaContext);
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);
    mbedtls_mpi_free(&nDivBy2);
    mk82SystemMemSet(privateKey, 0x00, sizeof(privateKey));
    return retVal;
}

static void ethHalButtonPressedCallback(void) { ethHalButtonPressed = ETH_TRUE; }

void ethHalWaitForComfirmation(uint16_t* confirmed)
{
    uint64_t initialTime;
    uint64_t currentTime;

    ethHalButtonPressed = ETH_FALSE;

#ifdef USE_BUTTON
    mk82ButtonRegisterButtonDoubleClickedCallback(ethHalButtonPressedCallback);
#endif

#ifdef USE_TOUCH
    mk82TouchRegisterButton2PressedCallback(ethHalButtonPressedCallback);
#endif

#ifdef USE_TOUCH
    mk82TouchEnable();
#endif

    mk82SystemTickerGetMsPassed(&initialTime);

    while (1)
    {
#ifdef USE_TOUCH
        mk82TouchTask();
#endif

        if (ethHalButtonPressed == ETH_TRUE)
        {
            *confirmed = ETH_TRUE;
            break;
        }

        mk82SystemTickerGetMsPassed(&currentTime);

        if ((currentTime - initialTime) > ETH_HAL_CONFIRMATION_TIMEOUT_IN_MS)
        {
            *confirmed = ETH_FALSE;
            break;
        }
    }

#ifdef USE_TOUCH
    mk82TouchDisable();
#endif

#ifdef USE_BUTTON
    mk82ButtonDeregisterButtonDoubleClickedCallback();
#endif

#ifdef USE_TOUCH
    mk82TouchDeregisterButton2PressedCallback();
#endif
}

void ethHalWipeout(void)
{
    uint32_t bytesWritten = 0;
    uint8_t wipeoutBuffer[ETH_HAL_WIPEOUT_BUFFER_SIZE];
    uint16_t trueOrFalse;
    uint16_t walletState;
    ETH_HAL_NVM_COUNTERS counters = {ETH_GLOBAL_PIN_BLOCKED_ERROR_COUNTER_VALUE};

    trueOrFalse = ETH_TRUE;
    mk82FsWriteFile(MK82_FS_FILE_ID_ETH_DATA, offsetof(ETH_HAL_NVM_DATA, wipeoutInProgress), (uint8_t*)&trueOrFalse,
                    sizeof(trueOrFalse));

    mk82SystemMemSet(wipeoutBuffer, 0x00, sizeof(wipeoutBuffer));

    while ((bytesWritten + sizeof(wipeoutBuffer)) < sizeof(ETH_HAL_NVM_KEYS))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_ETH_KEYS, bytesWritten, wipeoutBuffer, sizeof(wipeoutBuffer));
        bytesWritten += sizeof(wipeoutBuffer);
    }

    if (bytesWritten < sizeof(ETH_HAL_NVM_KEYS))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_ETH_KEYS, bytesWritten, wipeoutBuffer,
                        (sizeof(ETH_HAL_NVM_KEYS) - bytesWritten));
    }

    trueOrFalse = ETH_FALSE;
    mk82FsWriteFile(MK82_FS_FILE_ID_ETH_KEYS, offsetof(ETH_HAL_NVM_KEYS, masterKeyInitialized), (uint8_t*)&trueOrFalse,
                    sizeof(trueOrFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_ETH_KEYS);

    mk82FsWriteFile(MK82_FS_FILE_ID_ETH_COUNTERS, 0, (uint8_t*)&counters, sizeof(counters));
    mk82FsCommitWrite(MK82_FS_FILE_ID_ETH_COUNTERS);

    mk82SystemMemSet(wipeoutBuffer, 0x00, sizeof(wipeoutBuffer));
    bytesWritten = 0;

    while ((bytesWritten + sizeof(wipeoutBuffer)) < sizeof(ETH_HAL_NVM_DATA))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_ETH_DATA, bytesWritten, wipeoutBuffer, sizeof(wipeoutBuffer));
        bytesWritten += sizeof(wipeoutBuffer);
    }

    if (bytesWritten < sizeof(ETH_HAL_NVM_DATA))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_ETH_DATA, bytesWritten, wipeoutBuffer,
                        (sizeof(ETH_HAL_NVM_DATA) - bytesWritten));
    }

    walletState = ETH_GLOBAL_WALLET_STATE_INITIALIZATION;
    mk82FsWriteFile(MK82_FS_FILE_ID_ETH_DATA, offsetof(ETH_HAL_NVM_DATA, walletState), (uint8_t*)&walletState,
                    sizeof(walletState));
    trueOrFalse = ETH_FALSE;
    mk82FsWriteFile(MK82_FS_FILE_ID_ETH_DATA, offsetof(ETH_HAL_NVM_DATA, wipeoutInProgress), (uint8_t*)&trueOrFalse,
                    sizeof(trueOrFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_ETH_DATA);
}

void ethHalFatalError(void) { mk82SystemFatalError(); }
