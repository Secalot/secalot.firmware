/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "mk82Global.h"
#include "mk82GlobalInt.h"
#include "mk82System.h"
#include "mk82Bip32.h"
#include "mk82Bip32Int.h"

#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/ripemd160.h"
#include "mbedtls/md.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/ecp.h"

static void mk82Bip32PrivateKeyToPublicKey(uint8_t* privateKey, uint8_t* fullPublicKey, uint8_t* compressedPublicKey,
                                           uint16_t computeFull, uint16_t computeCompressed);

static void mk82Bip32PrivateKeyToPublicKey(uint8_t* privateKey, uint8_t* fullPublicKey, uint8_t* compressedPublicKey,
                                           uint16_t computeFull, uint16_t computeCompressed)
{
    int calleeRetVal;
    uint32_t pointLength;
    mbedtls_ecp_group ecpGroup;
    mbedtls_ecp_point ecpPoint;
    mbedtls_mpi multiplier;

    mbedtls_ecp_group_init(&ecpGroup);
    mbedtls_ecp_point_init(&ecpPoint);
    mbedtls_mpi_init(&multiplier);

    calleeRetVal = mbedtls_ecp_group_load(&ecpGroup, MBEDTLS_ECP_DP_SECP256K1);
    if (calleeRetVal != 0)
    {
        mk82SystemFatalError();
    }

    calleeRetVal = mbedtls_mpi_read_binary(&multiplier, privateKey, MK82_BIP32_PRIVATE_KEY_SIZE);
    if (calleeRetVal != 0)
    {
        mk82SystemFatalError();
    }

    calleeRetVal = mbedtls_ecp_mul(&ecpGroup, &ecpPoint, &multiplier, &(ecpGroup.G), mk82SystemGetRandomForTLS, NULL);
    if (calleeRetVal != 0)
    {
        mk82SystemFatalError();
    }

    if (computeCompressed == MK82_TRUE)
    {
        pointLength = MK82_BIP32_ENCODED_COMPRESSED_POINT_SIZE;

        calleeRetVal =
            mbedtls_ecp_point_write_binary(&ecpGroup, &ecpPoint, MBEDTLS_ECP_PF_COMPRESSED, (size_t*)&pointLength,
                                           compressedPublicKey, MK82_BIP32_ENCODED_COMPRESSED_POINT_SIZE);

        if (pointLength != MK82_BIP32_ENCODED_COMPRESSED_POINT_SIZE)
        {
            mk82SystemFatalError();
        }
    }

    if (computeFull == MK82_TRUE)
    {
        pointLength = MK82_BIP32_ENCODED_FULL_POINT_SIZE;

        calleeRetVal =
            mbedtls_ecp_point_write_binary(&ecpGroup, &ecpPoint, MBEDTLS_ECP_PF_UNCOMPRESSED, (size_t*)&pointLength,
                                           fullPublicKey, MK82_BIP32_ENCODED_FULL_POINT_SIZE);

        if (pointLength != MK82_BIP32_ENCODED_FULL_POINT_SIZE)
        {
            mk82SystemFatalError();
        }
    }

    if (calleeRetVal != 0)
    {
        mk82SystemFatalError();
    }

    mbedtls_ecp_group_free(&ecpGroup);
    mbedtls_ecp_point_free(&ecpPoint);
    mbedtls_mpi_free(&multiplier);
}

uint16_t mk82Bip32DerivePrivateKey(uint32_t* derivationIndexes, uint32_t numberOfKeyDerivations, uint8_t* privateKey,
                                   uint8_t* chainCode, MK82_BIP32_GET_MASTER_KEY_CALLBACK callback)
{
    uint16_t retVal = MK82_BIP32_GENERAL_ERROR;
    uint8_t masterKey[MK82_BIP32_MASTER_KEY_SIZE];
    uint32_t i;
    int calleeRetVal;
    mbedtls_md_context_t mdCtx;
    uint8_t serializedDerivationIndex[sizeof(uint32_t)];
    uint8_t hmac[MK82_BIP32_SHA512_SIZE];
    uint8_t intermediateDerivedPrivateKey[MK82_BIP32_PRIVATE_KEY_SIZE];
    uint8_t intermediateDerivedChainCode[MK82_BIP32_CHAIN_CODE_SIZE];
    uint16_t derivationFailed = MK82_FALSE;

    if ((numberOfKeyDerivations < MK82_BIP32_MINIMAL_NUMBER_OF_KEY_DERIVATIONS) ||
        (numberOfKeyDerivations > MK82_BIP32_MAXIMAL_NUMBER_OF_KEY_DERIVATIONS))
    {
        mk82SystemFatalError();
    }

    callback(masterKey);

    mk82SystemMemCpy(intermediateDerivedPrivateKey, &masterKey[MK82_BIP32_MASTER_KEY_PRIVATE_KEY_OFFSET],
                     MK82_BIP32_PRIVATE_KEY_SIZE);
    mk82SystemMemCpy(intermediateDerivedChainCode, &masterKey[MK82_BIP32_MASTER_KEY_CHAIN_CODE_OFFSET],
                     MK82_BIP32_CHAIN_CODE_SIZE);

    mbedtls_md_init(&mdCtx);

    for (i = 0; i < numberOfKeyDerivations; i++)
    {
        serializedDerivationIndex[0] = derivationIndexes[i] >> 24;
        serializedDerivationIndex[1] = derivationIndexes[i] >> 16;
        serializedDerivationIndex[2] = derivationIndexes[i] >> 8;
        serializedDerivationIndex[3] = derivationIndexes[i];

        calleeRetVal = mbedtls_md_setup(&mdCtx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA512), 1);
        if (calleeRetVal != 0)
        {
            mk82SystemFatalError();
        }

        if ((derivationIndexes[i] & MK82_BIP32_HARDENED_KEY_MASK) == MK82_BIP32_HARDENED_KEY_MASK)
        {
            uint8_t zero = 0;

            calleeRetVal = mbedtls_md_hmac_starts(&mdCtx, intermediateDerivedChainCode, MK82_BIP32_CHAIN_CODE_SIZE);
            if (calleeRetVal != 0)
            {
                mk82SystemFatalError();
            }
            calleeRetVal = mbedtls_md_hmac_update(&mdCtx, &zero, sizeof(zero));
            if (calleeRetVal != 0)
            {
                mk82SystemFatalError();
            }
            calleeRetVal = mbedtls_md_hmac_update(&mdCtx, intermediateDerivedPrivateKey, MK82_BIP32_PRIVATE_KEY_SIZE);
            if (calleeRetVal != 0)
            {
                mk82SystemFatalError();
            }
            calleeRetVal = mbedtls_md_hmac_update(&mdCtx, serializedDerivationIndex, sizeof(serializedDerivationIndex));
            if (calleeRetVal != 0)
            {
                mk82SystemFatalError();
            }
            calleeRetVal = mbedtls_md_hmac_finish(&mdCtx, hmac);
            if (calleeRetVal != 0)
            {
                mk82SystemFatalError();
            }
        }
        else
        {
            uint8_t compressedPoint[MK82_BIP32_ENCODED_COMPRESSED_POINT_SIZE];

            mk82Bip32PrivateKeyToPublicKey(intermediateDerivedPrivateKey, NULL, compressedPoint, MK82_FALSE, MK82_TRUE);

            calleeRetVal = mbedtls_md_hmac_starts(&mdCtx, intermediateDerivedChainCode, MK82_BIP32_CHAIN_CODE_SIZE);
            if (calleeRetVal != 0)
            {
                mk82SystemFatalError();
            }
            calleeRetVal = mbedtls_md_hmac_update(&mdCtx, compressedPoint, MK82_BIP32_ENCODED_COMPRESSED_POINT_SIZE);
            if (calleeRetVal != 0)
            {
                mk82SystemFatalError();
            }
            calleeRetVal = mbedtls_md_hmac_update(&mdCtx, serializedDerivationIndex, sizeof(serializedDerivationIndex));
            if (calleeRetVal != 0)
            {
                mk82SystemFatalError();
            }
            calleeRetVal = mbedtls_md_hmac_finish(&mdCtx, hmac);
            if (calleeRetVal != 0)
            {
                mk82SystemFatalError();
            }
        }

        mbedtls_md_free(&mdCtx);

        {
            mbedtls_mpi mpiLeftSideOfHash;
            mbedtls_mpi mpiParentPrivateKey;
            mbedtls_mpi mpiAdditionResult;
            mbedtls_mpi mpiZero;
            mbedtls_ecp_group ecpGroup;
            uint8_t zero = 0;

            mbedtls_ecp_group_init(&ecpGroup);
            mbedtls_mpi_init(&mpiLeftSideOfHash);
            mbedtls_mpi_init(&mpiParentPrivateKey);
            mbedtls_mpi_init(&mpiAdditionResult);
            mbedtls_mpi_init(&mpiZero);

            calleeRetVal = mbedtls_ecp_group_load(&ecpGroup, MBEDTLS_ECP_DP_SECP256K1);
            if (calleeRetVal != 0)
            {
                mk82SystemFatalError();
            }

            calleeRetVal = mbedtls_mpi_read_binary(&mpiLeftSideOfHash, &hmac[MK82_BIP32_SHA512_LEFT_PART_OFFSET],
                                                   MK82_BIP32_PRIVATE_KEY_SIZE);
            if (calleeRetVal != 0)
            {
                mk82SystemFatalError();
            }
            calleeRetVal = mbedtls_mpi_read_binary(&mpiParentPrivateKey, intermediateDerivedPrivateKey,
                                                   MK82_BIP32_PRIVATE_KEY_SIZE);
            if (calleeRetVal != 0)
            {
                mk82SystemFatalError();
            }
            calleeRetVal = mbedtls_mpi_read_binary(&mpiZero, &zero, sizeof(zero));
            if (calleeRetVal != 0)
            {
                mk82SystemFatalError();
            }

            calleeRetVal = mbedtls_mpi_cmp_abs(&mpiLeftSideOfHash, &(ecpGroup.N));

            if (calleeRetVal != -1)
            {
                derivationFailed = MK82_TRUE;
            }

            calleeRetVal = mbedtls_mpi_add_abs(&mpiAdditionResult, &mpiParentPrivateKey, &mpiLeftSideOfHash);
            if (calleeRetVal != 0)
            {
                mk82SystemFatalError();
            }
            calleeRetVal = mbedtls_mpi_mod_mpi(&mpiAdditionResult, &mpiAdditionResult, &(ecpGroup.N));
            if (calleeRetVal != 0)
            {
                mk82SystemFatalError();
            }

            calleeRetVal = mbedtls_mpi_cmp_abs(&mpiAdditionResult, &mpiZero);

            if (calleeRetVal == 0)
            {
                derivationFailed = MK82_TRUE;
            }

            calleeRetVal = mbedtls_mpi_write_binary(&mpiAdditionResult, intermediateDerivedPrivateKey,
                                                    MK82_BIP32_PRIVATE_KEY_SIZE);
            if (calleeRetVal != 0)
            {
                mk82SystemFatalError();
            }

            mk82SystemMemCpy(intermediateDerivedChainCode, &hmac[MK82_BIP32_SHA512_RIGHT_PART_OFFSET],
                             MK82_BIP32_CHAIN_CODE_SIZE);

            mbedtls_ecp_group_free(&ecpGroup);
            mbedtls_mpi_free(&mpiLeftSideOfHash);
            mbedtls_mpi_free(&mpiParentPrivateKey);
            mbedtls_mpi_free(&mpiAdditionResult);
            mbedtls_mpi_free(&mpiZero);
        }

        if (derivationFailed == MK82_TRUE)
        {
            break;
        }
    }

    if (derivationFailed == MK82_TRUE)
    {
        retVal = MK82_BIP32_KEY_DERIVATION_ERROR;
    }
    else
    {
        mk82SystemMemCpy(privateKey, intermediateDerivedPrivateKey, MK82_BIP32_PRIVATE_KEY_SIZE);
        mk82SystemMemCpy(chainCode, intermediateDerivedChainCode, MK82_BIP32_CHAIN_CODE_SIZE);
        retVal = MK82_BIP32_NO_ERROR;
    }

    mk82SystemMemSet(masterKey, 0x00, sizeof(masterKey));
    mk82SystemMemSet(intermediateDerivedPrivateKey, 0x00, sizeof(intermediateDerivedPrivateKey));
    mk82SystemMemSet(intermediateDerivedChainCode, 0x00, sizeof(intermediateDerivedChainCode));
    mk82SystemMemSet(hmac, 0x00, sizeof(hmac));

    return retVal;
}

uint16_t mk82Bip32DerivePublicKey(uint32_t* derivationIndexes, uint32_t numberOfKeyDerivations, uint8_t* fullPublicKey,
                                  uint8_t* compressedPublicKey, uint8_t* chainCode, uint16_t computeFull,
                                  uint16_t computeCompressed, MK82_BIP32_GET_MASTER_KEY_CALLBACK callback)
{
    uint8_t privateKey[MK82_BIP32_PRIVATE_KEY_SIZE];
    uint16_t calleeRetVal = MK82_BIP32_GENERAL_ERROR;
    uint16_t retVal = MK82_BIP32_GENERAL_ERROR;

    if ((derivationIndexes == NULL) || ((computeFull == MK82_TRUE) && (fullPublicKey == NULL)) ||
        ((computeCompressed == MK82_TRUE) && (compressedPublicKey == NULL)))
    {
        mk82SystemFatalError();
    }

    calleeRetVal =
        mk82Bip32DerivePrivateKey(derivationIndexes, numberOfKeyDerivations, privateKey, chainCode, callback);

    if (calleeRetVal != MK82_BIP32_NO_ERROR)
    {
        if (calleeRetVal == MK82_BIP32_KEY_DERIVATION_ERROR)
        {
            retVal = MK82_BIP32_KEY_DERIVATION_ERROR;
            goto END;
        }
        else
        {
            mk82SystemFatalError();
        }
    }

    mk82Bip32PrivateKeyToPublicKey(privateKey, fullPublicKey, compressedPublicKey, computeFull, computeCompressed);

    retVal = MK82_BIP32_NO_ERROR;

END:
    mk82SystemMemSet(privateKey, 0x00, sizeof(privateKey));
    return retVal;
}
