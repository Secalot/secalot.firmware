/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "mk82Global.h"
#include "mk82GlobalInt.h"
#include "mk82System.h"
#include "mk82Fs.h"
#include "mk82KeySafe.h"
#include "mk82KeySafeInt.h"
#include "mk82KeySafeReadOnlyKeysInt.h"

#include "fsl_ltc.h"

#include "mbedtls/sha256.h"

static void mk82KeysafeGetKek(uint16_t kekID, uint8_t* kek);
static void mk82KeysafeGenerateAndEncryptKek(uint8_t* nonce, uint8_t* tag, uint8_t* kek, uint8_t* encryptedKek);
static void mk82KeysafeGetInitializationState(uint16_t* dataInitialized);

static void mk82KeysafeGetKek(uint16_t kekID, uint8_t* kek)
{
    uint8_t encryptedKek[MK82_KEYSAFE_KEK_LENGTH];
    uint8_t kekNonce[MK82_KEYSAFE_NONCE_LENGTH];
    uint8_t kekTag[MK82_KEYSAFE_TAG_LENGTH];
    mbedtls_sha256_context shaContext;
    uint8_t hash[MK82_KEYSAFE_SHA256_LENGTH];
    status_t calleeRetVal;
    uint32_t i;
    KEYSAFE_READONLY_KEYS* readonlyKeys = (KEYSAFE_READONLY_KEYS*)MK82_GLOBAL_READONLY_KEYS_STRUCTURE_ADDRESS;

    if ((kekID == MK82_KEYSAFE_SF_KEK_ID) || (kekID == MK82_KEYSAFE_OTP_KEK_ID) || (kekID == MK82_KEYSAFE_BTC_KEK_ID) ||
        (kekID == MK82_KEYSAFE_OPGP_KEK_ID))
    {
        uint32_t encryptedKekOffset;
        uint32_t kekNonceOffset;
        uint32_t kekTagOffset;

        if (kekID == MK82_KEYSAFE_SF_KEK_ID)
        {
            encryptedKekOffset = offsetof(KEYSAFE_NVM_DATA, sfKek);
            kekNonceOffset = offsetof(KEYSAFE_NVM_DATA, sfKekNonce);
            kekTagOffset = offsetof(KEYSAFE_NVM_DATA, sfKekTag);
        }
        else if (kekID == MK82_KEYSAFE_OTP_KEK_ID)
        {
            encryptedKekOffset = offsetof(KEYSAFE_NVM_DATA, otpKek);
            kekNonceOffset = offsetof(KEYSAFE_NVM_DATA, otpKekNonce);
            kekTagOffset = offsetof(KEYSAFE_NVM_DATA, otpKekTag);
        }
        else if (kekID == MK82_KEYSAFE_BTC_KEK_ID)
        {
            encryptedKekOffset = offsetof(KEYSAFE_NVM_DATA, btcKek);
            kekNonceOffset = offsetof(KEYSAFE_NVM_DATA, btcKekNonce);
            kekTagOffset = offsetof(KEYSAFE_NVM_DATA, btcKekTag);
        }
        else if (kekID == MK82_KEYSAFE_OPGP_KEK_ID)
        {
            encryptedKekOffset = offsetof(KEYSAFE_NVM_DATA, opgpKek);
            kekNonceOffset = offsetof(KEYSAFE_NVM_DATA, opgpKekNonce);
            kekTagOffset = offsetof(KEYSAFE_NVM_DATA, opgpKekTag);
        }
        else
        {
            mk82SystemFatalError();
        }

        mk82FsReadFile(MK82_FS_FILE_ID_KEYSAFE_DATA, encryptedKekOffset, encryptedKek, sizeof(encryptedKek));
        mk82FsReadFile(MK82_FS_FILE_ID_KEYSAFE_DATA, kekNonceOffset, kekNonce, sizeof(kekNonce));
        mk82FsReadFile(MK82_FS_FILE_ID_KEYSAFE_DATA, kekTagOffset, kekTag, sizeof(kekTag));

        calleeRetVal =
            LTC_AES_DecryptTagGcm(LTC0, encryptedKek, kek, MK82_KEYSAFE_KEK_LENGTH, kekNonce, sizeof(kekNonce), NULL, 0,
                                  readonlyKeys->masterKey, MK82_KEYSAFE_MASTERKEY_LENGTH, kekTag, sizeof(kekTag));

        if (calleeRetVal != kStatus_Success)
        {
            mk82SystemFatalError();
        }

        mbedtls_sha256_init(&shaContext);
        mbedtls_sha256_starts(&shaContext, false);
        mbedtls_sha256_update(&shaContext, (uint8_t*)&SIM->UIDL, sizeof(uint32_t));
        mbedtls_sha256_update(&shaContext, (uint8_t*)&SIM->UIDML, sizeof(uint32_t));
        mbedtls_sha256_update(&shaContext, (uint8_t*)&SIM->UIDMH, sizeof(uint32_t));
        mbedtls_sha256_update(&shaContext, (uint8_t*)&SIM->UIDH, sizeof(uint32_t));
        mbedtls_sha256_finish(&shaContext, hash);
        mbedtls_sha256_free(&shaContext);

        for (i = 0; i < MK82_KEYSAFE_KEK_LENGTH; i++)
        {
            kek[i] ^= hash[i];
        }

        memset(hash, 0x00, sizeof(hash));
    }
    else
    {
        mk82SystemFatalError();
    }
}

static void mk82KeysafeGenerateAndEncryptKek(uint8_t* nonce, uint8_t* tag, uint8_t* kek, uint8_t* encryptedKek)
{
    status_t calleeRetVal;
    KEYSAFE_READONLY_KEYS* readonlyKeys = (KEYSAFE_READONLY_KEYS*)MK82_GLOBAL_READONLY_KEYS_STRUCTURE_ADDRESS;

    mk82SystemGetRandom(nonce, MK82_KEYSAFE_NONCE_LENGTH);
    mk82SystemGetRandom(kek, MK82_KEYSAFE_KEK_LENGTH);

    calleeRetVal =
        LTC_AES_EncryptTagGcm(LTC0, kek, encryptedKek, MK82_KEYSAFE_KEK_LENGTH, nonce, MK82_KEYSAFE_NONCE_LENGTH, NULL,
                              0, readonlyKeys->masterKey, MK82_KEYSAFE_MASTERKEY_LENGTH, tag, MK82_KEYSAFE_TAG_LENGTH);

    memset(kek, 0x00, MK82_KEYSAFE_KEK_LENGTH);

    if (calleeRetVal != kStatus_Success)
    {
        mk82SystemFatalError();
    }
}

static void mk82KeysafeGetInitializationState(uint16_t* dataInitialized)
{
    mk82FsReadFile(MK82_FS_FILE_ID_KEYSAFE_DATA, offsetof(KEYSAFE_NVM_DATA, dataInitialized), (uint8_t*)dataInitialized,
                   sizeof(uint16_t));
}

void mk82KeysafeInit(void)
{
    uint16_t dataInitialized = MK82_FALSE;

    mk82KeysafeGetInitializationState(&dataInitialized);

    if (dataInitialized != MK82_TRUE)
    {
        uint8_t nonce[MK82_KEYSAFE_NONCE_LENGTH];
        uint8_t tag[MK82_KEYSAFE_TAG_LENGTH];
        uint8_t kek[MK82_KEYSAFE_KEK_LENGTH];
        uint8_t encryptedKek[MK82_KEYSAFE_KEK_LENGTH];

        mk82KeysafeGenerateAndEncryptKek(nonce, tag, kek, encryptedKek);

        mk82FsWriteFile(MK82_FS_FILE_ID_KEYSAFE_DATA, offsetof(KEYSAFE_NVM_DATA, sfKek), encryptedKek,
                        sizeof(encryptedKek));
        mk82FsWriteFile(MK82_FS_FILE_ID_KEYSAFE_DATA, offsetof(KEYSAFE_NVM_DATA, sfKekNonce), nonce, sizeof(nonce));
        mk82FsWriteFile(MK82_FS_FILE_ID_KEYSAFE_DATA, offsetof(KEYSAFE_NVM_DATA, sfKekTag), tag, sizeof(tag));

        mk82KeysafeGenerateAndEncryptKek(nonce, tag, kek, encryptedKek);

        mk82FsWriteFile(MK82_FS_FILE_ID_KEYSAFE_DATA, offsetof(KEYSAFE_NVM_DATA, otpKek), encryptedKek,
                        sizeof(encryptedKek));
        mk82FsWriteFile(MK82_FS_FILE_ID_KEYSAFE_DATA, offsetof(KEYSAFE_NVM_DATA, otpKekNonce), nonce, sizeof(nonce));
        mk82FsWriteFile(MK82_FS_FILE_ID_KEYSAFE_DATA, offsetof(KEYSAFE_NVM_DATA, otpKekTag), tag, sizeof(tag));

        mk82KeysafeGenerateAndEncryptKek(nonce, tag, kek, encryptedKek);

        mk82FsWriteFile(MK82_FS_FILE_ID_KEYSAFE_DATA, offsetof(KEYSAFE_NVM_DATA, btcKek), encryptedKek,
                        sizeof(encryptedKek));
        mk82FsWriteFile(MK82_FS_FILE_ID_KEYSAFE_DATA, offsetof(KEYSAFE_NVM_DATA, btcKekNonce), nonce, sizeof(nonce));
        mk82FsWriteFile(MK82_FS_FILE_ID_KEYSAFE_DATA, offsetof(KEYSAFE_NVM_DATA, btcKekTag), tag, sizeof(tag));

        mk82KeysafeGenerateAndEncryptKek(nonce, tag, kek, encryptedKek);

        mk82FsWriteFile(MK82_FS_FILE_ID_KEYSAFE_DATA, offsetof(KEYSAFE_NVM_DATA, opgpKek), encryptedKek,
                        sizeof(encryptedKek));
        mk82FsWriteFile(MK82_FS_FILE_ID_KEYSAFE_DATA, offsetof(KEYSAFE_NVM_DATA, opgpKekNonce), nonce, sizeof(nonce));
        mk82FsWriteFile(MK82_FS_FILE_ID_KEYSAFE_DATA, offsetof(KEYSAFE_NVM_DATA, opgpKekTag), tag, sizeof(tag));

        dataInitialized = MK82_TRUE;
        mk82FsWriteFile(MK82_FS_FILE_ID_KEYSAFE_DATA, offsetof(KEYSAFE_NVM_DATA, dataInitialized),
                        (uint8_t*)&dataInitialized, sizeof(dataInitialized));
        mk82FsCommitWrite(MK82_FS_FILE_ID_KEYSAFE_DATA);
    }
}

void mk82KeysafeWrapKey(uint16_t kekID, uint8_t* key, uint32_t keyLength, uint8_t* encryptedKey, uint8_t* appData,
                        uint32_t appDataLength, uint8_t* nonce, uint8_t* tag)
{
    uint8_t kek[MK82_KEYSAFE_KEK_LENGTH];
    status_t calleeRetVal;
    uint16_t dataInitialized;

    if ((key == NULL) || (encryptedKey == NULL) || (nonce == NULL) || (tag == NULL))
    {
        mk82SystemFatalError();
    }

    mk82KeysafeGetInitializationState(&dataInitialized);

    if (dataInitialized != MK82_TRUE)
    {
        mk82SystemFatalError();
    }

    mk82KeysafeGetKek(kekID, kek);

    mk82SystemGetRandom(nonce, MK82_KEYSAFE_NONCE_LENGTH);

    calleeRetVal = LTC_AES_EncryptTagGcm(LTC0, key, encryptedKey, keyLength, nonce, MK82_KEYSAFE_NONCE_LENGTH, appData,
                                         appDataLength, kek, sizeof(kek), tag, MK82_KEYSAFE_TAG_LENGTH);

    if (calleeRetVal != kStatus_Success)
    {
        mk82SystemFatalError();
    }

    memset(kek, 0x00, sizeof(kek));
}

uint16_t mk82KeysafeUnwrapKey(uint16_t kekID, uint8_t* encryptedKey, uint32_t keyLength, uint8_t* key, uint8_t* appData,
                              uint32_t appDataLength, uint8_t* nonce, uint8_t* tag)
{
    uint8_t kek[MK82_KEYSAFE_KEK_LENGTH];
    status_t calleeRetVal;
    uint16_t retVal = MK82_WRAPPED_KEY_CORRUPTED_ERROR;
    uint16_t dataInitialized;

    if ((key == NULL) || (encryptedKey == NULL) || (nonce == NULL) || (tag == NULL))
    {
        mk82SystemFatalError();
    }

    mk82KeysafeGetInitializationState(&dataInitialized);

    if (dataInitialized != MK82_TRUE)
    {
        mk82SystemFatalError();
    }

    mk82KeysafeGetKek(kekID, kek);

    calleeRetVal = LTC_AES_DecryptTagGcm(LTC0, encryptedKey, key, keyLength, nonce, MK82_KEYSAFE_NONCE_LENGTH, appData,
                                         appDataLength, kek, sizeof(kek), tag, MK82_KEYSAFE_TAG_LENGTH);

    if (calleeRetVal != kStatus_Success)
    {
        retVal = MK82_WRAPPED_KEY_CORRUPTED_ERROR;
    }
    else
    {
        retVal = MK82_NO_ERROR;
    }

    memset(kek, 0x00, sizeof(kek));

    return retVal;
}

void mk82KeysafeGetSfAttestationKey(uint8_t* key)
{
    KEYSAFE_READONLY_KEYS* readonlyKeys = (KEYSAFE_READONLY_KEYS*)MK82_GLOBAL_READONLY_KEYS_STRUCTURE_ADDRESS;

    if (key == NULL)
    {
        mk82SystemFatalError();
    }

    mk82SystemMemCpy(key, readonlyKeys->sfAttestationPrivateKey, MK82_KEYSAFE_SF_ATTESTATION_PRIVATE_KEY_LENGTH);
}

void mk82KeysafeWipeout(void)
{
    uint32_t bytesWritten = 0;
    uint8_t wipeoutBuffer[MK82_KEYSAFE_WIPEOUT_BUFFER_SIZE];
    uint16_t trueOrFalse;

    mk82SystemMemSet(wipeoutBuffer, 0x00, sizeof(wipeoutBuffer));

    while ((bytesWritten + sizeof(wipeoutBuffer)) < sizeof(KEYSAFE_NVM_DATA))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_KEYSAFE_DATA, bytesWritten, wipeoutBuffer, sizeof(wipeoutBuffer));
        bytesWritten += sizeof(wipeoutBuffer);
    }

    if (bytesWritten < sizeof(KEYSAFE_NVM_DATA))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_KEYSAFE_DATA, bytesWritten, wipeoutBuffer,
                        (sizeof(KEYSAFE_NVM_DATA) - bytesWritten));
    }

    trueOrFalse = MK82_FALSE;
    mk82FsWriteFile(MK82_FS_FILE_ID_KEYSAFE_DATA, offsetof(KEYSAFE_NVM_DATA, dataInitialized), (uint8_t*)&trueOrFalse,
                    sizeof(trueOrFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_KEYSAFE_DATA);
}
