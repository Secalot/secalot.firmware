/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <btcGlobal.h>
#include <btcGlobalInt.h>
#include <btcHal.h>
#include <hal/k82/btcHalInt.h>

#include <mk82Global.h>
#include <mk82System.h>
#include <mk82Fs.h>
#include <mk82Bip32.h>
#ifdef USE_BUTTON
#include <mk82Button.h>
#endif
#ifdef USE_TOUCH
#include <mk82Touch.h>
#endif
#include <mk82SecApdu.h>
#include <mk82As.h>

#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/ripemd160.h"
#include "mbedtls/md.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/ecp.h"

static void btcHalGetMasterKey(uint8_t* masterKey);
static void btcHalGetTrustedInputKey(uint8_t* trustedInputKey);
static uint16_t btcHalIsMasterKeyInitialized(void);
static uint16_t btcHalIsTrustedInputKeyInitialized(void);
static void btcHalButtonPressedCallback(void);

static mbedtls_sha256_context btcHalTrustedInputHashContext;
static mbedtls_sha256_context btcHalTransactionSigningHashContext;
static mbedtls_sha256_context btcHalTransactionIntegrityCheckHashContext;
static mbedtls_sha256_context btcHalMessageSigningHashContext;

static mbedtls_sha256_context btcHalHashSegWitPrevoutsContext;
static mbedtls_sha256_context btcHalHashSegWitSequenceContext;
static mbedtls_sha256_context btcHalHashSegWitOutputsContext;

static uint16_t btcHalButtonPressed;

static uint16_t btcHalConfirmationTimeOngoing;
static uint64_t btcHalInitialConfirmationTime;

void btcHalInit(void)
{
    mbedtls_sha256_init(&btcHalTrustedInputHashContext);
    mbedtls_sha256_init(&btcHalTransactionSigningHashContext);
    mbedtls_sha256_init(&btcHalTransactionIntegrityCheckHashContext);
    mbedtls_sha256_init(&btcHalMessageSigningHashContext);

    mbedtls_sha256_init(&btcHalHashSegWitPrevoutsContext);
    mbedtls_sha256_init(&btcHalHashSegWitSequenceContext);
    mbedtls_sha256_init(&btcHalHashSegWitOutputsContext);

    btcHalButtonPressed = BTC_FALSE;

    btcHalConfirmationTimeOngoing = BTC_FALSE;
    btcHalInitialConfirmationTime = 0;
}

void btcHalDeinit() {}

void btcHalMemCpy(uint8_t* dst, uint8_t* src, uint16_t length) { mk82SystemMemCpy(dst, src, length); }

void btcHalMemSet(uint8_t* dst, uint8_t value, uint16_t length) { mk82SystemMemSet(dst, value, length); }

uint16_t btcHalMemCmp(uint8_t* array1, uint8_t* array2, uint16_t length)
{
    uint16_t comparisonResult;

    comparisonResult = mk82SystemMemCmp(array1, array2, length);

    if (comparisonResult != MK82_TRUE)
    {
        return BTC_CMP_NOT_EQUAL;
    }
    else
    {
        return BTC_CMP_EQUAL;
    }
}

void btcHalGetPinErrorCounter(uint8_t* errorCounter)
{
    if (errorCounter == NULL)
    {
        btcHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_BTC_COUNTERS, offsetof(BTC_HAL_NVM_COUNTERS, pinErrorCounter), errorCounter,
                   sizeof(uint8_t));
}

void btcHalSetPinErrorCounter(uint8_t errorCounter)
{
    mk82FsWriteFile(MK82_FS_FILE_ID_BTC_COUNTERS, offsetof(BTC_HAL_NVM_COUNTERS, pinErrorCounter), &errorCounter,
                    sizeof(uint8_t));
    mk82FsCommitWrite(MK82_FS_FILE_ID_BTC_COUNTERS);
}

void btcHalGetPinHash(uint8_t* pinHash)
{
    if (pinHash == NULL)
    {
        btcHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_BTC_DATA, offsetof(BTC_HAL_NVM_DATA, pinHash), pinHash, BTC_GLOBAL_PIN_HASH_LENGTH);
}

void btcHalComputePinHash(uint8_t* pin, uint32_t pinLength, uint8_t* pinHash)
{
    if ((pin == NULL) || (pinHash == NULL))
    {
        btcHalFatalError();
    }

    mbedtls_sha256(pin, pinLength, pinHash, 0);
}

void btcHalGetCoinVersions(uint8_t* regularCoinVersion, uint8_t* p2shCoinVersion)
{
    if ((regularCoinVersion == NULL) || (p2shCoinVersion == NULL))
    {
        btcHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_BTC_DATA, offsetof(BTC_HAL_NVM_DATA, regularCoinVersion), regularCoinVersion,
                   sizeof(uint8_t));
    mk82FsReadFile(MK82_FS_FILE_ID_BTC_DATA, offsetof(BTC_HAL_NVM_DATA, p2shCoinVersion), p2shCoinVersion,
                   sizeof(uint8_t));
}

static uint16_t btcHalIsMasterKeyInitialized(void)
{
    uint16_t masterKeyInitialized = BTC_FALSE;
    uint16_t retVal = BTC_FALSE;

    mk82FsReadFile(MK82_FS_FILE_ID_BTC_KEYS, offsetof(BTC_HAL_NVM_KEYS, masterKeyInitialized),
                   (uint8_t*)&masterKeyInitialized, sizeof(masterKeyInitialized));

    if (masterKeyInitialized != BTC_TRUE)
    {
        retVal = BTC_FALSE;
    }
    else
    {
        retVal = BTC_TRUE;
    }

    return retVal;
}

static uint16_t btcHalIsTrustedInputKeyInitialized(void)
{
    uint16_t trustedInputKeyInitialized = BTC_FALSE;
    uint16_t retVal = BTC_FALSE;

    mk82FsReadFile(MK82_FS_FILE_ID_BTC_KEYS, offsetof(BTC_HAL_NVM_KEYS, trustedInputKeyInitialized),
                   (uint8_t*)&trustedInputKeyInitialized, sizeof(trustedInputKeyInitialized));

    if (trustedInputKeyInitialized != BTC_TRUE)
    {
        retVal = BTC_FALSE;
    }
    else
    {
        retVal = BTC_TRUE;
    }

    return retVal;
}

void btcHalSetMasterKey(uint8_t* seed, uint32_t seedLength)
{
    uint8_t masterKey[BTC_GLOBAL_MASTER_KEY_SIZE];
    uint8_t encryptedMasterKey[BTC_GLOBAL_MASTER_KEY_SIZE];
    uint8_t nonce[MK82_KEYSAFE_NONCE_LENGTH];
    uint8_t tag[MK82_KEYSAFE_TAG_LENGTH];
    uint16_t trueFalse;

    if (seed == NULL)
    {
        btcHalFatalError();
    }

    if ((seedLength < BTC_GLOBAL_SEED_MINIMUM_LENGTH) || (seedLength > BTC_GLOBAL_SEED_MAXIMUM_LENGTH))
    {
        btcHalFatalError();
    }

    int calleeRetVal;

    calleeRetVal = mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA512), (uint8_t*)BTC_HAL_HMAC_KEY,
                                   BTC_HAL_HMAC_KEY_LENGTH, seed, seedLength, masterKey);

    if (calleeRetVal != 0)
    {
        btcHalFatalError();
    }

    mk82KeysafeWrapKey(MK82_KEYSAFE_CCR_KEK_ID, masterKey, sizeof(masterKey), encryptedMasterKey, NULL, 0, nonce, tag);

    mk82SystemMemSet(masterKey, 0x00, sizeof(masterKey));

    mk82FsWriteFile(MK82_FS_FILE_ID_BTC_KEYS, offsetof(BTC_HAL_NVM_KEYS, masterKey), encryptedMasterKey,
                    BTC_GLOBAL_MASTER_KEY_SIZE);
    mk82FsWriteFile(MK82_FS_FILE_ID_BTC_KEYS, offsetof(BTC_HAL_NVM_KEYS, masterKeyNonce), nonce,
                    MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_BTC_KEYS, offsetof(BTC_HAL_NVM_KEYS, masterKeyTag), tag, MK82_KEYSAFE_TAG_LENGTH);
    trueFalse = BTC_TRUE;
    mk82FsWriteFile(MK82_FS_FILE_ID_BTC_KEYS, offsetof(BTC_HAL_NVM_KEYS, masterKeyInitialized), (uint8_t*)&trueFalse,
                    sizeof(trueFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_BTC_KEYS);
}

void btcHalSetRandomTrustedInputKey(void)
{
    uint8_t trustedInputKey[BTC_GLOBAL_TRUSTED_INPUT_KEY_SIZE];
    uint8_t encryptedtrustedInputKey[BTC_GLOBAL_TRUSTED_INPUT_KEY_SIZE];
    uint8_t nonce[MK82_KEYSAFE_NONCE_LENGTH];
    uint8_t tag[MK82_KEYSAFE_TAG_LENGTH];
    uint16_t trueFalse;

    mk82SystemGetRandom(trustedInputKey, BTC_GLOBAL_TRUSTED_INPUT_KEY_SIZE);

    mk82KeysafeWrapKey(MK82_KEYSAFE_CCR_KEK_ID, trustedInputKey, sizeof(trustedInputKey), encryptedtrustedInputKey,
                       NULL, 0, nonce, tag);

    mk82SystemMemSet(trustedInputKey, 0x00, sizeof(trustedInputKey));

    mk82FsWriteFile(MK82_FS_FILE_ID_BTC_KEYS, offsetof(BTC_HAL_NVM_KEYS, trustedInputKey), encryptedtrustedInputKey,
                    BTC_GLOBAL_TRUSTED_INPUT_KEY_SIZE);
    mk82FsWriteFile(MK82_FS_FILE_ID_BTC_KEYS, offsetof(BTC_HAL_NVM_KEYS, trustedInputKeyNonce), nonce,
                    MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_BTC_KEYS, offsetof(BTC_HAL_NVM_KEYS, trustedInputKeyTag), tag,
                    MK82_KEYSAFE_TAG_LENGTH);
    trueFalse = BTC_TRUE;
    mk82FsWriteFile(MK82_FS_FILE_ID_BTC_KEYS, offsetof(BTC_HAL_NVM_KEYS, trustedInputKeyInitialized),
                    (uint8_t*)&trueFalse, sizeof(trueFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_BTC_KEYS);
}

void btcHalWriteSetupInfoAndFinalizeSetup(uint8_t regularCoinVersion, uint8_t p2shCoinVersion, uint8_t* pinHash)
{
    uint8_t errorCounter = BTC_GLOBAL_PIN_INITIAL_ERROR_COUNTER_VALUE;
    uint16_t walletState = BTC_GLOBAL_WALLET_STATE_INITIALIZATION;

    if (pinHash == NULL)
    {
        btcHalFatalError();
    }

    mk82FsWriteFile(MK82_FS_FILE_ID_BTC_COUNTERS, offsetof(BTC_HAL_NVM_COUNTERS, pinErrorCounter), &errorCounter,
                    sizeof(uint8_t));
    mk82FsCommitWrite(MK82_FS_FILE_ID_BTC_COUNTERS);

    mk82FsWriteFile(MK82_FS_FILE_ID_BTC_DATA, offsetof(BTC_HAL_NVM_DATA, regularCoinVersion), &regularCoinVersion,
                    sizeof(uint8_t));
    mk82FsWriteFile(MK82_FS_FILE_ID_BTC_DATA, offsetof(BTC_HAL_NVM_DATA, p2shCoinVersion), &p2shCoinVersion,
                    sizeof(uint8_t));
    mk82FsWriteFile(MK82_FS_FILE_ID_BTC_DATA, offsetof(BTC_HAL_NVM_DATA, pinHash), pinHash, BTC_GLOBAL_PIN_HASH_LENGTH);
    walletState = BTC_GLOBAL_WALLET_STATE_OPERATIONAL;
    mk82FsWriteFile(MK82_FS_FILE_ID_BTC_DATA, offsetof(BTC_HAL_NVM_DATA, walletState), (uint8_t*)&walletState,
                    sizeof(walletState));

    mk82FsCommitWrite(MK82_FS_FILE_ID_BTC_DATA);
}

uint16_t btcHalGetWalletState(void)
{
    uint16_t walletState = BTC_GLOBAL_WALLET_STATE_INITIALIZATION;

    mk82FsReadFile(MK82_FS_FILE_ID_BTC_DATA, offsetof(BTC_HAL_NVM_DATA, walletState), (uint8_t*)&walletState,
                   sizeof(walletState));

    return walletState;
}

uint16_t btcHalIsWipeoutInProgress(void)
{
    uint16_t wipeoutInProgress = BTC_TRUE;

    mk82FsReadFile(MK82_FS_FILE_ID_BTC_DATA, offsetof(BTC_HAL_NVM_DATA, wipeoutInProgress),
                   (uint8_t*)&wipeoutInProgress, sizeof(wipeoutInProgress));

    return wipeoutInProgress;
}

static void btcHalGetMasterKey(uint8_t* masterKey)
{
    uint8_t encryptedMasterKey[BTC_GLOBAL_MASTER_KEY_SIZE];
    uint8_t nonce[MK82_KEYSAFE_NONCE_LENGTH];
    uint8_t tag[MK82_KEYSAFE_TAG_LENGTH];
    uint16_t masterKeyInitialized = BTC_FALSE;

    masterKeyInitialized = btcHalIsMasterKeyInitialized();

    if (masterKeyInitialized != BTC_TRUE)
    {
        btcHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_BTC_KEYS, offsetof(BTC_HAL_NVM_KEYS, masterKey), encryptedMasterKey,
                   BTC_GLOBAL_MASTER_KEY_SIZE);
    mk82FsReadFile(MK82_FS_FILE_ID_BTC_KEYS, offsetof(BTC_HAL_NVM_KEYS, masterKeyNonce), nonce,
                   MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsReadFile(MK82_FS_FILE_ID_BTC_KEYS, offsetof(BTC_HAL_NVM_KEYS, masterKeyTag), tag, MK82_KEYSAFE_TAG_LENGTH);

    mk82KeysafeUnwrapKey(MK82_KEYSAFE_CCR_KEK_ID, encryptedMasterKey, sizeof(encryptedMasterKey), masterKey, NULL, 0,
                         nonce, tag);
}

static void btcHalGetTrustedInputKey(uint8_t* trustedInputKey)
{
    uint8_t encryptedTrustedInputKey[BTC_GLOBAL_TRUSTED_INPUT_KEY_SIZE];
    uint8_t nonce[MK82_KEYSAFE_NONCE_LENGTH];
    uint8_t tag[MK82_KEYSAFE_TAG_LENGTH];
    uint16_t trustedInputKeyInitialized = BTC_FALSE;

    trustedInputKeyInitialized = btcHalIsTrustedInputKeyInitialized();

    if (trustedInputKeyInitialized != BTC_TRUE)
    {
        btcHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_BTC_KEYS, offsetof(BTC_HAL_NVM_KEYS, trustedInputKey), encryptedTrustedInputKey,
                   BTC_GLOBAL_TRUSTED_INPUT_KEY_SIZE);
    mk82FsReadFile(MK82_FS_FILE_ID_BTC_KEYS, offsetof(BTC_HAL_NVM_KEYS, trustedInputKeyNonce), nonce,
                   MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsReadFile(MK82_FS_FILE_ID_BTC_KEYS, offsetof(BTC_HAL_NVM_KEYS, trustedInputKeyTag), tag,
                   MK82_KEYSAFE_TAG_LENGTH);

    mk82KeysafeUnwrapKey(MK82_KEYSAFE_CCR_KEK_ID, encryptedTrustedInputKey, sizeof(encryptedTrustedInputKey),
                         trustedInputKey, NULL, 0, nonce, tag);
}

uint16_t btcHalDerivePublicKey(uint32_t* derivationIndexes, uint32_t numberOfKeyDerivations, uint8_t* fullPublicKey,
                               uint8_t* compressedPublicKey, uint8_t* chainCode, uint16_t computeFull,
                               uint16_t computeCompressed)
{
    uint16_t retVal = MK82_BIP32_GENERAL_ERROR;

    if (computeFull == BTC_TRUE)
    {
        computeFull = MK82_TRUE;
    }
    else
    {
        computeFull = MK82_FALSE;
    }

    if (computeCompressed == BTC_TRUE)
    {
        computeCompressed = MK82_TRUE;
    }
    else
    {
        computeCompressed = MK82_FALSE;
    }

    retVal = mk82Bip32DerivePublicKey(derivationIndexes, numberOfKeyDerivations, fullPublicKey, compressedPublicKey,
                                      chainCode, computeFull, computeCompressed, btcHalGetMasterKey);

    if (retVal != MK82_BIP32_NO_ERROR)
    {
        if (retVal == MK82_BIP32_KEY_DERIVATION_ERROR)
        {
            retVal = BTC_KEY_DERIVATION_ERROR;
        }
        else
        {
            retVal = BTC_GENERAL_ERROR;
        }
    }
    else
    {
        retVal = BTC_NO_ERROR;
    }

    return retVal;
}

uint16_t btcHalSignHash(uint32_t* derivationIndexes, uint32_t numberOfKeyDerivations, uint8_t* hash, uint8_t* signature,
                        uint32_t* signatureLength, uint16_t isTransactionSignature)
{
    uint8_t privateKey[BTC_GLOBAL_PRIVATE_KEY_SIZE];
    uint8_t chainCode[BTC_GLOBAL_CHAIN_CODE_SIZE];
    uint16_t calleeRetVal = BTC_GENERAL_ERROR;
    uint16_t retVal = BTC_GENERAL_ERROR;
    int tlsCalleeRetVal = -1;
    mbedtls_ecdsa_context ecsdaContext;
    mbedtls_mpi r;
    mbedtls_mpi s;
    mbedtls_mpi nDivBy2;
    uint8_t nDivBy2Array[] = {0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                              0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5D, 0x57, 0x6E, 0x73, 0x57, 0xA4,
                              0x50, 0x1D, 0xDF, 0xE9, 0x2F, 0x46, 0x68, 0x1B, 0x20, 0xA0};
    uint8_t signatureInternal[MBEDTLS_ECDSA_MAX_LEN];
    uint32_t signatureLengthInternal;
    int rYSign;

    if ((derivationIndexes == NULL) || (hash == NULL) || (signature == NULL) || (signatureLength == NULL))
    {
        btcHalFatalError();
    }

    mbedtls_ecdsa_init(&ecsdaContext);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);
    mbedtls_mpi_init(&nDivBy2);

    calleeRetVal =
        mk82Bip32DerivePrivateKey(derivationIndexes, numberOfKeyDerivations, privateKey, chainCode, btcHalGetMasterKey);

    if (calleeRetVal != MK82_BIP32_NO_ERROR)
    {
        if (calleeRetVal == MK82_BIP32_KEY_DERIVATION_ERROR)
        {
            retVal = BTC_KEY_DERIVATION_ERROR;
            goto END;
        }
        else
        {
            btcHalFatalError();
        }
    }

    tlsCalleeRetVal = mbedtls_ecp_group_load(&ecsdaContext.grp, MBEDTLS_ECP_DP_SECP256K1);

    if (tlsCalleeRetVal != 0)
    {
        btcHalFatalError();
    }

    tlsCalleeRetVal = mbedtls_mpi_read_binary(&ecsdaContext.d, privateKey, BTC_GLOBAL_PRIVATE_KEY_SIZE);

    if (tlsCalleeRetVal != 0)
    {
        btcHalFatalError();
    }

    *signatureLength = MBEDTLS_ECDSA_MAX_LEN;

    tlsCalleeRetVal = mbedtls_ecdsa_sign_det(&ecsdaContext.grp, &r, &s, &ecsdaContext.d, hash, &rYSign,
                                             BTC_GLOBAL_SHA256_SIZE, MBEDTLS_MD_SHA256);

    if (tlsCalleeRetVal != 0)
    {
        btcHalFatalError();
    }

    if (isTransactionSignature == BTC_TRUE)
    {
        tlsCalleeRetVal = mbedtls_mpi_read_binary(&nDivBy2, nDivBy2Array, sizeof(nDivBy2Array));

        if (tlsCalleeRetVal != 0)
        {
            btcHalFatalError();
        }

        tlsCalleeRetVal = mbedtls_mpi_cmp_abs(&s, &nDivBy2);

        if (tlsCalleeRetVal == 1)
        {
            tlsCalleeRetVal = mbedtls_mpi_sub_abs(&s, &(ecsdaContext.grp.N), &s);

            if (tlsCalleeRetVal != 0)
            {
                btcHalFatalError();
            }
        }
    }

    tlsCalleeRetVal = ecdsa_signature_to_asn1(&r, &s, signatureInternal, (size_t*)&signatureLengthInternal);

    if (tlsCalleeRetVal != 0)
    {
        btcHalFatalError();
    }

    if (isTransactionSignature == BTC_FALSE)
    {
        signatureInternal[0] |= rYSign;
    }

    if (signatureLengthInternal > BTC_GLOBAL_MAXIMAL_SIGNATURE_LENGTH)
    {
        btcHalFatalError();
    }

    *signatureLength = signatureLengthInternal;
    mk82SystemMemCpy(signature, signatureInternal, signatureLengthInternal);

    retVal = BTC_NO_ERROR;

END:
    mbedtls_ecdsa_free(&ecsdaContext);
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);
    mbedtls_mpi_free(&nDivBy2);
    mk82SystemMemSet(privateKey, 0x00, sizeof(privateKey));
    return retVal;
}

void btcHalComputeTrustedInputMAC(uint8_t* blob)
{
    uint8_t trustedInputKey[BTC_GLOBAL_TRUSTED_INPUT_KEY_SIZE];
    uint8_t hmac[BTC_GLOBAL_SHA256_SIZE];
    int calleeRetVal;

    if (blob == NULL)
    {
        btcHalFatalError();
    }

    btcHalGetTrustedInputKey(trustedInputKey);

    calleeRetVal =
        mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), trustedInputKey,
                        BTC_GLOBAL_TRUSTED_INPUT_KEY_SIZE, blob, BTC_GLOBAL_TRUSTED_INPUT_BLOB_MAC_OFFSET, hmac);

    mk82SystemMemSet(trustedInputKey, 0x00, sizeof(trustedInputKey));

    if (calleeRetVal != 0)
    {
        btcHalFatalError();
    }

    mk82SystemMemCpy(&blob[BTC_GLOBAL_TRUSTED_INPUT_BLOB_MAC_OFFSET], hmac, BTC_GLOBAL_TRUSTED_INPUT_BLOB_MAC_LENGTH);
}

uint16_t btcHalCheckTrustedInputMAC(uint8_t* blob)
{
    uint8_t trustedInputKey[BTC_GLOBAL_TRUSTED_INPUT_KEY_SIZE];
    uint8_t hmac[BTC_GLOBAL_SHA256_SIZE];
    uint16_t retVal = BTC_GENERAL_ERROR;
    int calleeRetVal = -1;
    uint16_t comparisonResult;

    if (blob == NULL)
    {
        btcHalFatalError();
    }

    btcHalGetTrustedInputKey(trustedInputKey);

    calleeRetVal =
        mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), trustedInputKey,
                        BTC_GLOBAL_TRUSTED_INPUT_KEY_SIZE, blob, BTC_GLOBAL_TRUSTED_INPUT_BLOB_MAC_OFFSET, hmac);

    mk82SystemMemSet(trustedInputKey, 0x00, sizeof(trustedInputKey));

    if (calleeRetVal != 0)
    {
        btcHalFatalError();
    }

    comparisonResult = mk82SystemMemCmp(hmac, blob + BTC_GLOBAL_TRUSTED_INPUT_BLOB_MAC_OFFSET,
                                        BTC_GLOBAL_TRUSTED_INPUT_BLOB_MAC_LENGTH);

    if (comparisonResult != MK82_TRUE)
    {
        retVal = BTC_INVALID_MAC_ERROR;
        goto END;
    }

    retVal = BTC_NO_ERROR;

END:
    return retVal;
}

void btcHalGenerateNewSeed(uint8_t* seed, uint32_t seedLength)
{
    if (seed == NULL)
    {
        btcHalFatalError();
    }

    if ((seedLength > BTC_GLOBAL_SEED_MAXIMUM_LENGTH) || (seedLength < BTC_GLOBAL_SEED_MINIMUM_LENGTH))
    {
        btcHalFatalError();
    }

    mk82SystemGetRandom(seed, seedLength);
}

void btcHalHash160(uint8_t* data, uint32_t dataLength, uint8_t* hash)
{
    uint8_t internalHashBuffer[BTC_GLOBAL_SHA256_SIZE];

    if ((data == NULL) || (hash == NULL))
    {
        btcHalFatalError();
    }

    mbedtls_sha256(data, dataLength, internalHashBuffer, 0);
    mbedtls_ripemd160(internalHashBuffer, sizeof(internalHashBuffer), hash);
}

void btcHalSha256Start(uint16_t hashID)
{
    mbedtls_sha256_context* hashContext;

    if (hashID == BTC_HAL_HASH_ID_TRUSTED_INPUT)
    {
        hashContext = &btcHalTrustedInputHashContext;
    }
    else if (hashID == BTC_HAL_HASH_ID_TRANSACTION_SIGNING)
    {
        hashContext = &btcHalTransactionSigningHashContext;
    }
    else if (hashID == BTC_HAL_HASH_ID_TRANSACTION_INTEGRITY_CHECK)
    {
        hashContext = &btcHalTransactionIntegrityCheckHashContext;
    }
    else if (hashID == BTC_HAL_HASH_ID_MESSAGE_SIGNING)
    {
        hashContext = &btcHalMessageSigningHashContext;
    }
    else if (hashID == BTC_HAL_HASH_ID_SEGWIT_PREVOUTS)
    {
        hashContext = &btcHalHashSegWitPrevoutsContext;
    }
    else if (hashID == BTC_HAL_HASH_ID_SEGWIT_SEQUENCE)
    {
        hashContext = &btcHalHashSegWitSequenceContext;
    }
    else if (hashID == BTC_HAL_HASH_ID_SEGWIT_OUTPUTS)
    {
        hashContext = &btcHalHashSegWitOutputsContext;
    }
    else
    {
        btcHalFatalError();
    }

    mbedtls_sha256_starts(hashContext, 0);
}

void btcHalSha256Update(uint16_t hashID, uint8_t* data, uint32_t dataLength)
{
    mbedtls_sha256_context* hashContext;

    if (data == NULL)
    {
        btcHalFatalError();
    }

    if (hashID == BTC_HAL_HASH_ID_TRUSTED_INPUT)
    {
        hashContext = &btcHalTrustedInputHashContext;
    }
    else if (hashID == BTC_HAL_HASH_ID_TRANSACTION_SIGNING)
    {
        hashContext = &btcHalTransactionSigningHashContext;
    }
    else if (hashID == BTC_HAL_HASH_ID_TRANSACTION_INTEGRITY_CHECK)
    {
        hashContext = &btcHalTransactionIntegrityCheckHashContext;
    }
    else if (hashID == BTC_HAL_HASH_ID_MESSAGE_SIGNING)
    {
        hashContext = &btcHalMessageSigningHashContext;
    }
    else if (hashID == BTC_HAL_HASH_ID_SEGWIT_PREVOUTS)
    {
        hashContext = &btcHalHashSegWitPrevoutsContext;
    }
    else if (hashID == BTC_HAL_HASH_ID_SEGWIT_SEQUENCE)
    {
        hashContext = &btcHalHashSegWitSequenceContext;
    }
    else if (hashID == BTC_HAL_HASH_ID_SEGWIT_OUTPUTS)
    {
        hashContext = &btcHalHashSegWitOutputsContext;
    }
    else
    {
        btcHalFatalError();
    }

    mbedtls_sha256_update(hashContext, data, dataLength);
}

void btcHalSha256Finalize(uint16_t hashID, uint8_t* hash)
{
    mbedtls_sha256_context* hashContext;

    if (hash == NULL)
    {
        btcHalFatalError();
    }

    if (hashID == BTC_HAL_HASH_ID_TRUSTED_INPUT)
    {
        hashContext = &btcHalTrustedInputHashContext;
    }
    else if (hashID == BTC_HAL_HASH_ID_TRANSACTION_SIGNING)
    {
        hashContext = &btcHalTransactionSigningHashContext;
    }
    else if (hashID == BTC_HAL_HASH_ID_TRANSACTION_INTEGRITY_CHECK)
    {
        hashContext = &btcHalTransactionIntegrityCheckHashContext;
    }
    else if (hashID == BTC_HAL_HASH_ID_MESSAGE_SIGNING)
    {
        hashContext = &btcHalMessageSigningHashContext;
    }
    else if (hashID == BTC_HAL_HASH_ID_SEGWIT_PREVOUTS)
    {
        hashContext = &btcHalHashSegWitPrevoutsContext;
    }
    else if (hashID == BTC_HAL_HASH_ID_SEGWIT_SEQUENCE)
    {
        hashContext = &btcHalHashSegWitSequenceContext;
    }
    else if (hashID == BTC_HAL_HASH_ID_SEGWIT_OUTPUTS)
    {
        hashContext = &btcHalHashSegWitOutputsContext;
    }
    else
    {
        btcHalFatalError();
    }

    mbedtls_sha256_finish(hashContext, hash);
}

void btcHalSha256(uint8_t* data, uint32_t dataLength, uint8_t* hash)
{
    if ((data == NULL) || (hash == NULL))
    {
        btcHalFatalError();
    }

    mbedtls_sha256(data, dataLength, hash, 0);
}

static void btcHalButtonPressedCallback(void) { btcHalButtonPressed = BTC_TRUE; }

void btcHalGetRandom(uint8_t* buffer, uint32_t length)
{
    if (buffer == NULL)
    {
        btcHalFatalError();
    }

    mk82SystemGetRandom(buffer, length);
}

void btcHalWaitForComfirmation(uint16_t allowCcidApdus, uint16_t* confirmed)
{
    uint64_t currentTime;
    uint16_t currentDataType;

    btcHalButtonPressed = BTC_FALSE;

    currentDataType = mk82SecApduGetPrimaryDataType();

    btcHalConfirmationTimeOngoing = BTC_TRUE;

#ifdef USE_BUTTON
    mk82ButtonRegisterButtonDoubleClickedCallback(btcHalButtonPressedCallback);
#endif

#ifdef USE_TOUCH
    mk82TouchRegisterButton2PressedCallback(btcHalButtonPressedCallback);
#endif

#ifdef USE_TOUCH
    mk82TouchEnable();
#endif

    mk82SystemTickerGetMsPassed(&btcHalInitialConfirmationTime);

    while (1)
    {
#ifdef USE_TOUCH
        mk82TouchTask();
#endif

        if (btcHalButtonPressed == BTC_TRUE)
        {
            *confirmed = BTC_TRUE;
            break;
        }

        mk82SystemTickerGetMsPassed(&currentTime);

        if ((currentTime - btcHalInitialConfirmationTime) > BTC_HAL_CONFIRMATION_TIMEOUT_IN_MS)
        {
            *confirmed = BTC_FALSE;
            break;
        }

        if (allowCcidApdus == BTC_TRUE)
        {
            if (currentDataType == MK82_GLOBAL_DATATYPE_BTC_MESSAGE)
            {
                mk82SecApduProcessCommandIfAvailable(MK82_GLOBAL_PROCESS_CCID_APDU,
                                                     MK82_AS_ALLOW_BTC_COMMANDS | MK82_AS_ALLOW_SSL_COMMANDS);
            }
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

uint64_t btcHalGetRemainingConfirmationTime(void)
{
    uint64_t currentTime;

    if (btcHalConfirmationTimeOngoing != BTC_TRUE)
    {
        btcHalFatalError();
    }

    mk82SystemTickerGetMsPassed(&currentTime);

    return (BTC_HAL_CONFIRMATION_TIMEOUT_IN_MS - (currentTime - btcHalInitialConfirmationTime));
}

void btcHalWipeout(void)
{
    uint32_t bytesWritten = 0;
    uint8_t wipeoutBuffer[BTC_HAL_WIPEOUT_BUFFER_SIZE];
    uint16_t trueOrFalse;
    uint16_t walletState;
    BTC_HAL_NVM_COUNTERS counters = {BTC_GLOBAL_PIN_BLOCKED_ERROR_COUNTER_VALUE};

    trueOrFalse = BTC_TRUE;
    mk82FsWriteFile(MK82_FS_FILE_ID_BTC_DATA, offsetof(BTC_HAL_NVM_DATA, wipeoutInProgress), (uint8_t*)&trueOrFalse,
                    sizeof(trueOrFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_BTC_DATA);

    mk82SystemMemSet(wipeoutBuffer, 0x00, sizeof(wipeoutBuffer));

    while ((bytesWritten + sizeof(wipeoutBuffer)) < sizeof(BTC_HAL_NVM_KEYS))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_BTC_KEYS, bytesWritten, wipeoutBuffer, sizeof(wipeoutBuffer));
        bytesWritten += sizeof(wipeoutBuffer);
    }

    if (bytesWritten < sizeof(BTC_HAL_NVM_KEYS))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_BTC_KEYS, bytesWritten, wipeoutBuffer,
                        (sizeof(BTC_HAL_NVM_KEYS) - bytesWritten));
    }

    trueOrFalse = BTC_FALSE;
    mk82FsWriteFile(MK82_FS_FILE_ID_BTC_KEYS, offsetof(BTC_HAL_NVM_KEYS, masterKeyInitialized), (uint8_t*)&trueOrFalse,
                    sizeof(trueOrFalse));
    mk82FsWriteFile(MK82_FS_FILE_ID_BTC_KEYS, offsetof(BTC_HAL_NVM_KEYS, trustedInputKeyInitialized),
                    (uint8_t*)&trueOrFalse, sizeof(trueOrFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_BTC_KEYS);

    mk82FsWriteFile(MK82_FS_FILE_ID_BTC_COUNTERS, 0, (uint8_t*)&counters, sizeof(counters));
    mk82FsCommitWrite(MK82_FS_FILE_ID_BTC_COUNTERS);

    mk82SystemMemSet(wipeoutBuffer, 0x00, sizeof(wipeoutBuffer));
    bytesWritten = 0;

    while ((bytesWritten + sizeof(wipeoutBuffer)) < sizeof(BTC_HAL_NVM_DATA))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_BTC_DATA, bytesWritten, wipeoutBuffer, sizeof(wipeoutBuffer));
        bytesWritten += sizeof(wipeoutBuffer);
    }

    if (bytesWritten < sizeof(BTC_HAL_NVM_DATA))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_BTC_DATA, bytesWritten, wipeoutBuffer,
                        (sizeof(BTC_HAL_NVM_DATA) - bytesWritten));
    }

    walletState = BTC_GLOBAL_WALLET_STATE_INITIALIZATION;
    mk82FsWriteFile(MK82_FS_FILE_ID_BTC_DATA, offsetof(BTC_HAL_NVM_DATA, walletState), (uint8_t*)&walletState,
                    sizeof(walletState));
    trueOrFalse = BTC_FALSE;
    mk82FsWriteFile(MK82_FS_FILE_ID_BTC_DATA, offsetof(BTC_HAL_NVM_DATA, wipeoutInProgress), (uint8_t*)&trueOrFalse,
                    sizeof(trueOrFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_BTC_DATA);
}

void btcHalFatalError(void) { mk82SystemFatalError(); }
