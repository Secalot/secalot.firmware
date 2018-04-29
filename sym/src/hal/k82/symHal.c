/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <symGlobal.h>
#include <symGlobalInt.h>
#include <symHal.h>
#include <hal/k82/symHalInt.h>

#include "mk82Global.h"
#include "mk82System.h"
#include "mk82KeySafe.h"
#include "mk82Fs.h"

#include "mbedtls/sha256.h"
#include "mbedtls/md.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/ecp.h"

static uint16_t symHalIsMasterKeyInitialized(void);
static void symHalGetMasterKey(uint8_t* masterKey);
static void symHalDerivePrivateKey(uint8_t* privateKey, uint8_t* tweak, uint32_t tweakLength);

void symHalInit(void) {}

void symHalDeinit(void) {}

uint16_t symHalMemCmp(uint8_t* array1, uint8_t* array2, uint16_t length)
{
    uint16_t comparisonResult;

    comparisonResult = mk82SystemMemCmp(array1, array2, length);

    if (comparisonResult != MK82_TRUE)
    {
        return SYM_CMP_NOT_EQUAL;
    }
    else
    {
        return SYM_CMP_EQUAL;
    }
}

void symHalMemSet(uint8_t* dst, uint8_t value, uint16_t length) { mk82SystemMemSet(dst, value, length); }

void symHalMemCpy(uint8_t* dst, uint8_t* src, uint16_t length) { mk82SystemMemCpy(dst, src, length); }

void symHalInitializeWallet(uint8_t* masterKey)
{
    uint8_t encryptedMasterKey[SYM_GLOBAL_MASTER_KEY_SIZE];
    uint8_t nonce[MK82_KEYSAFE_NONCE_LENGTH];
    uint8_t tag[MK82_KEYSAFE_TAG_LENGTH];
    uint16_t trueFalse;
    uint16_t walletState = SYM_GLOBAL_WALLET_STATE_INITIALIZATION;

    if (masterKey == NULL)
    {
        symHalFatalError();
    }

    mk82KeysafeWrapKey(MK82_KEYSAFE_CCR_KEK_ID, masterKey, SYM_GLOBAL_MASTER_KEY_SIZE, encryptedMasterKey, NULL, 0,
                       nonce, tag);

    mk82SystemMemSet(masterKey, 0x00, sizeof(masterKey));

    mk82FsWriteFile(MK82_FS_FILE_ID_SYM_KEYS, offsetof(SYM_HAL_NVM_KEYS, masterKey), encryptedMasterKey,
                    SYM_GLOBAL_MASTER_KEY_SIZE);
    mk82FsWriteFile(MK82_FS_FILE_ID_SYM_KEYS, offsetof(SYM_HAL_NVM_KEYS, masterKeyNonce), nonce,
                    MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_SYM_KEYS, offsetof(SYM_HAL_NVM_KEYS, masterKeyTag), tag, MK82_KEYSAFE_TAG_LENGTH);
    trueFalse = SYM_TRUE;
    mk82FsWriteFile(MK82_FS_FILE_ID_SYM_KEYS, offsetof(SYM_HAL_NVM_KEYS, masterKeyInitialized), (uint8_t*)&trueFalse,
                    sizeof(trueFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_SYM_KEYS);

    walletState = SYM_GLOBAL_WALLET_STATE_OPERATIONAL;
    mk82FsWriteFile(MK82_FS_FILE_ID_SYM_DATA, offsetof(SYM_HAL_NVM_DATA, walletState), (uint8_t*)&walletState,
                    sizeof(walletState));

    mk82FsCommitWrite(MK82_FS_FILE_ID_SYM_DATA);
}

uint16_t symHalGetWalletState(void)
{
    uint16_t walletState = SYM_GLOBAL_WALLET_STATE_INITIALIZATION;

    mk82FsReadFile(MK82_FS_FILE_ID_SYM_DATA, offsetof(SYM_HAL_NVM_DATA, walletState), (uint8_t*)&walletState,
                   sizeof(walletState));

    return walletState;
}

uint16_t symHalIsWipeoutInProgress(void)
{
    uint16_t wipeoutInProgress = SYM_TRUE;

    mk82FsReadFile(MK82_FS_FILE_ID_SYM_DATA, offsetof(SYM_HAL_NVM_DATA, wipeoutInProgress),
                   (uint8_t*)&wipeoutInProgress, sizeof(wipeoutInProgress));

    return wipeoutInProgress;
}

void symHalGetRandom(uint8_t* buffer, uint32_t length)
{
    if (buffer == NULL)
    {
        symHalFatalError();
    }

    mk82SystemGetRandom(buffer, length);
}

static uint16_t symHalIsMasterKeyInitialized(void)
{
    uint16_t masterKeyInitialized = SYM_FALSE;
    uint16_t retVal = SYM_FALSE;

    mk82FsReadFile(MK82_FS_FILE_ID_SYM_KEYS, offsetof(SYM_HAL_NVM_KEYS, masterKeyInitialized),
                   (uint8_t*)&masterKeyInitialized, sizeof(masterKeyInitialized));

    if (masterKeyInitialized != SYM_TRUE)
    {
        retVal = SYM_FALSE;
    }
    else
    {
        retVal = SYM_TRUE;
    }

    return retVal;
}

static void symHalGetMasterKey(uint8_t* masterKey)
{
    uint8_t encryptedMasterKey[SYM_GLOBAL_MASTER_KEY_SIZE];
    uint8_t nonce[MK82_KEYSAFE_NONCE_LENGTH];
    uint8_t tag[MK82_KEYSAFE_TAG_LENGTH];
    uint16_t masterKeyInitialized = SYM_FALSE;

    masterKeyInitialized = symHalIsMasterKeyInitialized();

    if (masterKeyInitialized != SYM_TRUE)
    {
        symHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_SYM_KEYS, offsetof(SYM_HAL_NVM_KEYS, masterKey), encryptedMasterKey,
                   SYM_GLOBAL_MASTER_KEY_SIZE);
    mk82FsReadFile(MK82_FS_FILE_ID_SYM_KEYS, offsetof(SYM_HAL_NVM_KEYS, masterKeyNonce), nonce,
                   MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsReadFile(MK82_FS_FILE_ID_SYM_KEYS, offsetof(SYM_HAL_NVM_KEYS, masterKeyTag), tag, MK82_KEYSAFE_TAG_LENGTH);

    mk82KeysafeUnwrapKey(MK82_KEYSAFE_CCR_KEK_ID, encryptedMasterKey, sizeof(encryptedMasterKey), masterKey, NULL, 0,
                         nonce, tag);
}

static void symHalDerivePrivateKey(uint8_t* privateKey, uint8_t* tweak, uint32_t tweakLength)
{
    uint8_t masterKey[SYM_GLOBAL_MASTER_KEY_SIZE];
    mbedtls_sha256_context hashContext;
    uint8_t hash[SYM_GLOBAL_SHA256_SIZE];
    mbedtls_ecdsa_context ecsdaContext;
    mbedtls_mpi hashMPI;
    int tlsCalleeRetVal = -1;
    uint16_t isZero = SYM_TRUE;

    if ((privateKey == NULL) || (tweak == NULL))
    {
        symHalFatalError();
    }

    if ((tweakLength < SYM_GLOBAL_MINIMAL_TWEAK_LENGTH) || (tweakLength > SYM_GLOBAL_MAXIMAL_TWEAK_LENGTH))
    {
        symHalFatalError();
    }

    mbedtls_ecdsa_init(&ecsdaContext);

    mbedtls_mpi_init(&hashMPI);

    tlsCalleeRetVal = mbedtls_ecp_group_load(&ecsdaContext.grp, MBEDTLS_ECP_DP_SECP256R1);

    if (tlsCalleeRetVal != 0)
    {
        symHalFatalError();
    }

    symHalGetMasterKey(masterKey);

    mbedtls_sha256_init(&hashContext);
    mbedtls_sha256_update(&hashContext, masterKey, sizeof(masterKey));
    mbedtls_sha256_update(&hashContext, tweak, tweakLength);
    mbedtls_sha256_finish(&hashContext, hash);

    mbedtls_sha256(hash, sizeof(hash), hash, 0);

    while (1)
    {
        isZero = SYM_TRUE;

        tlsCalleeRetVal = mbedtls_mpi_read_binary(&hashMPI, hash, sizeof(hash));

        if (tlsCalleeRetVal != 0)
        {
            symHalFatalError();
        }

        tlsCalleeRetVal = mbedtls_mpi_cmp_int(&hashMPI, 0);

        if (tlsCalleeRetVal != 0)
        {
            isZero = SYM_FALSE;
        }

        tlsCalleeRetVal = mbedtls_mpi_cmp_abs(&hashMPI, &(ecsdaContext.grp.N));

        if ((tlsCalleeRetVal == -1) && (isZero == SYM_FALSE))
        {
            break;
        }

        mbedtls_sha256(hash, sizeof(hash), hash, 0);
    }

    mk82SystemMemCpy(privateKey, hash, SYM_GLOBAL_SHA256_SIZE);

    mbedtls_ecdsa_free(&ecsdaContext);
    mbedtls_mpi_free(&hashMPI);
    mk82SystemMemSet(masterKey, 0x00, sizeof(masterKey));
}

void symHalDerivePublicKey(uint8_t* publicKey, uint8_t* tweak, uint32_t tweakLength)
{
    int calleeRetVal;
    uint8_t privateKey[SYM_GLOBAL_PRIVATE_KEY_SIZE];
    uint32_t pointLength;
    mbedtls_ecp_group ecpGroup;
    mbedtls_ecp_point ecpPoint;
    mbedtls_mpi multiplier;

    if ((publicKey == NULL) || (tweak == NULL))
    {
        symHalFatalError();
    }

    if ((tweakLength < SYM_GLOBAL_MINIMAL_TWEAK_LENGTH) || (tweakLength > SYM_GLOBAL_MAXIMAL_TWEAK_LENGTH))
    {
        symHalFatalError();
    }

    mbedtls_ecp_group_init(&ecpGroup);
    mbedtls_ecp_point_init(&ecpPoint);
    mbedtls_mpi_init(&multiplier);

    symHalDerivePrivateKey(privateKey, tweak, tweakLength);

    calleeRetVal = mbedtls_ecp_group_load(&ecpGroup, MBEDTLS_ECP_DP_SECP256R1);
    if (calleeRetVal != 0)
    {
        symHalFatalError();
    }

    calleeRetVal = mbedtls_mpi_read_binary(&multiplier, privateKey, SYM_GLOBAL_PRIVATE_KEY_SIZE);
    if (calleeRetVal != 0)
    {
        symHalFatalError();
    }

    calleeRetVal = mbedtls_ecp_mul(&ecpGroup, &ecpPoint, &multiplier, &(ecpGroup.G), mk82SystemGetRandomForTLS, NULL);
    if (calleeRetVal != 0)
    {
        symHalFatalError();
    }

    pointLength = SYM_GLOBAL_ENCODED_FULL_POINT_SIZE;

    calleeRetVal = mbedtls_ecp_point_write_binary(&ecpGroup, &ecpPoint, MBEDTLS_ECP_PF_UNCOMPRESSED,
                                                  (size_t*)&pointLength, publicKey, SYM_GLOBAL_ENCODED_FULL_POINT_SIZE);

    if (pointLength != SYM_GLOBAL_ENCODED_FULL_POINT_SIZE)
    {
        symHalFatalError();
    }

    if (calleeRetVal != 0)
    {
        symHalFatalError();
    }

    mbedtls_ecp_group_free(&ecpGroup);
    mbedtls_ecp_point_free(&ecpPoint);
    mbedtls_mpi_free(&multiplier);
}

void symHalSignHash(uint8_t* tweak, uint32_t tweakLength, uint8_t* hash, uint8_t* signature)
{
    uint8_t privateKey[SYM_GLOBAL_PRIVATE_KEY_SIZE];
    uint16_t calleeRetVal = SYM_GENERAL_ERROR;
    int tlsCalleeRetVal = -1;
    mbedtls_ecdsa_context ecsdaContext;
    mbedtls_mpi r;
    mbedtls_mpi s;
    int rYSign;

    if ((tweak == NULL) || (hash == NULL) || (signature == NULL))
    {
        symHalFatalError();
    }

    mbedtls_ecdsa_init(&ecsdaContext);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);

    symHalDerivePrivateKey(privateKey, tweak, tweakLength);

    tlsCalleeRetVal = mbedtls_ecp_group_load(&ecsdaContext.grp, MBEDTLS_ECP_DP_SECP256R1);

    if (tlsCalleeRetVal != 0)
    {
        symHalFatalError();
    }

    tlsCalleeRetVal = mbedtls_mpi_read_binary(&ecsdaContext.d, privateKey, SYM_GLOBAL_PRIVATE_KEY_SIZE);

    if (tlsCalleeRetVal != 0)
    {
        symHalFatalError();
    }

    tlsCalleeRetVal = mbedtls_ecdsa_sign(&ecsdaContext.grp, &r, &s, &ecsdaContext.d, hash, SYM_GLOBAL_SHA256_SIZE,
                                         &rYSign, mk82SystemGetRandomForTLS, NULL);

    if (tlsCalleeRetVal != 0)
    {
        symHalFatalError();
    }

    tlsCalleeRetVal = mbedtls_mpi_write_binary(&r, signature, SYM_HAL_R_S_LENGTH);

    if (tlsCalleeRetVal != 0)
    {
        symHalFatalError();
    }

    tlsCalleeRetVal = mbedtls_mpi_write_binary(&s, signature + SYM_HAL_R_S_LENGTH, SYM_HAL_R_S_LENGTH);

    if (tlsCalleeRetVal != 0)
    {
        symHalFatalError();
    }

END:
    mbedtls_ecdsa_free(&ecsdaContext);
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);
    mk82SystemMemSet(privateKey, 0x00, sizeof(privateKey));
}

void symHalWipeout(void)
{
    uint32_t bytesWritten = 0;
    uint8_t wipeoutBuffer[SYM_HAL_WIPEOUT_BUFFER_SIZE];
    uint16_t trueOrFalse;
    uint16_t walletState;

    trueOrFalse = SYM_TRUE;
    mk82FsWriteFile(MK82_FS_FILE_ID_SYM_DATA, offsetof(SYM_HAL_NVM_DATA, wipeoutInProgress), (uint8_t*)&trueOrFalse,
                    sizeof(trueOrFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_SYM_DATA);

    mk82SystemMemSet(wipeoutBuffer, 0x00, sizeof(wipeoutBuffer));

    while ((bytesWritten + sizeof(wipeoutBuffer)) < sizeof(SYM_HAL_NVM_KEYS))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_SYM_KEYS, bytesWritten, wipeoutBuffer, sizeof(wipeoutBuffer));
        bytesWritten += sizeof(wipeoutBuffer);
    }

    if (bytesWritten < sizeof(SYM_HAL_NVM_KEYS))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_SYM_KEYS, bytesWritten, wipeoutBuffer,
                        (sizeof(SYM_HAL_NVM_KEYS) - bytesWritten));
    }

    trueOrFalse = SYM_FALSE;
    mk82FsWriteFile(MK82_FS_FILE_ID_SYM_KEYS, offsetof(SYM_HAL_NVM_KEYS, masterKeyInitialized), (uint8_t*)&trueOrFalse,
                    sizeof(trueOrFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_SYM_KEYS);

    mk82SystemMemSet(wipeoutBuffer, 0x00, sizeof(wipeoutBuffer));
    bytesWritten = 0;

    while ((bytesWritten + sizeof(wipeoutBuffer)) < sizeof(SYM_HAL_NVM_DATA))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_SYM_DATA, bytesWritten, wipeoutBuffer, sizeof(wipeoutBuffer));
        bytesWritten += sizeof(wipeoutBuffer);
    }

    if (bytesWritten < sizeof(SYM_HAL_NVM_DATA))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_SYM_DATA, bytesWritten, wipeoutBuffer,
                        (sizeof(SYM_HAL_NVM_DATA) - bytesWritten));
    }

    walletState = SYM_GLOBAL_WALLET_STATE_INITIALIZATION;
    mk82FsWriteFile(MK82_FS_FILE_ID_SYM_DATA, offsetof(SYM_HAL_NVM_DATA, walletState), (uint8_t*)&walletState,
                    sizeof(walletState));
    trueOrFalse = SYM_FALSE;
    mk82FsWriteFile(MK82_FS_FILE_ID_SYM_DATA, offsetof(SYM_HAL_NVM_DATA, wipeoutInProgress), (uint8_t*)&trueOrFalse,
                    sizeof(trueOrFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_SYM_DATA);
}

void symHalFatalError(void) { mk82SystemFatalError(); }
