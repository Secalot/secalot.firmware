/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "opgpGlobal.h"
#include "opgpGlobalInt.h"
#include "opgpHal.h"
#include "opgpCore.h"

#include "mk82Global.h"
#include "mk82Fs.h"
#include "mk82System.h"

#include "fsl_common.h"

#include "mbedtls/rsa.h"
#include "mbedtls/bignum.h"
#include "mbedtls/sha256.h"

#include <string.h>
#include <stddef.h>

static void opgpHalGetRsaKeyFromFile(uint16_t keyType, mbedtls_rsa_context* rsaKey);
static void opgpHalGetKeyInfo(uint16_t keyType, uint32_t* keyOffset);
static void opgpHalGetDoWithConstantLengthInfo(uint16_t tag, uint32_t* dataOffset, uint32_t* length);
static void opgpHalGetDoWithVariableLengthInfo(uint16_t tag, uint32_t* dataOffset, uint32_t* lengthOffset,
                                               uint32_t* maximalLength);
static void opgpHalGetPinOffsets(uint8_t pinID, uint32_t* dataOffset, uint32_t* lengthOffset,
                                 uint32_t* errorCounterOffset);

static uint8_t opgpHalTempBuffer[OPGP_GLOBAL_MODULUS_LENGTH];

static void opgpHalGetRsaKeyFromFile(uint16_t keyType, mbedtls_rsa_context* rsaKey)
{
    int calleeRetVal;
    uint16_t mk82CalleeRetVal;
    uint32_t keyOffset;
    uint8_t tag[MK82_KEYSAFE_TAG_LENGTH];
    uint8_t nonce[MK82_KEYSAFE_NONCE_LENGTH];

    opgpHalGetKeyInfo(keyType, &keyOffset);

    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, n), (uint8_t*)&opgpHalTempBuffer,
                   OPGP_GLOBAL_MODULUS_LENGTH);
    calleeRetVal = mbedtls_mpi_read_binary(&rsaKey->N, opgpHalTempBuffer, OPGP_GLOBAL_MODULUS_LENGTH);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }
    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, e), (uint8_t*)&opgpHalTempBuffer,
                   OPGP_GLOBAL_PUBLIC_EXPONENT_LENGTH);
    calleeRetVal = mbedtls_mpi_read_binary(&rsaKey->E, opgpHalTempBuffer, OPGP_GLOBAL_PUBLIC_EXPONENT_LENGTH);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, p), (uint8_t*)&opgpHalTempBuffer,
                   OPGP_GLOBAL_PRIME_LENGTH);
    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, pNonce), (uint8_t*)&nonce,
                   MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, pTag), (uint8_t*)&tag,
                   MK82_KEYSAFE_TAG_LENGTH);
    mk82CalleeRetVal = mk82KeysafeUnwrapKey(MK82_KEYSAFE_OPGP_KEK_ID, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH,
                                            opgpHalTempBuffer, NULL, 0, nonce, tag);
    if (mk82CalleeRetVal != MK82_TRUE)
    {
        opgpHalFatalError();
    }
    calleeRetVal = mbedtls_mpi_read_binary(&rsaKey->P, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, q), (uint8_t*)&opgpHalTempBuffer,
                   OPGP_GLOBAL_PRIME_LENGTH);
    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, qNonce), (uint8_t*)&nonce,
                   MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, qTag), (uint8_t*)&tag,
                   MK82_KEYSAFE_TAG_LENGTH);
    mk82CalleeRetVal = mk82KeysafeUnwrapKey(MK82_KEYSAFE_OPGP_KEK_ID, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH,
                                            opgpHalTempBuffer, NULL, 0, nonce, tag);
    if (mk82CalleeRetVal != MK82_TRUE)
    {
        opgpHalFatalError();
    }
    calleeRetVal |= mbedtls_mpi_read_binary(&rsaKey->Q, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, dp1), (uint8_t*)&opgpHalTempBuffer,
                   OPGP_GLOBAL_PRIME_LENGTH);
    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, dp1Nonce), (uint8_t*)&nonce,
                   MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, dp1Tag), (uint8_t*)&tag,
                   MK82_KEYSAFE_TAG_LENGTH);
    mk82CalleeRetVal = mk82KeysafeUnwrapKey(MK82_KEYSAFE_OPGP_KEK_ID, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH,
                                            opgpHalTempBuffer, NULL, 0, nonce, tag);
    if (mk82CalleeRetVal != MK82_TRUE)
    {
        opgpHalFatalError();
    }
    calleeRetVal |= mbedtls_mpi_read_binary(&rsaKey->DP, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, dq1), (uint8_t*)&opgpHalTempBuffer,
                   OPGP_GLOBAL_PRIME_LENGTH);
    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, dq1Nonce), (uint8_t*)&nonce,
                   MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, dq1Tag), (uint8_t*)&tag,
                   MK82_KEYSAFE_TAG_LENGTH);
    mk82CalleeRetVal = mk82KeysafeUnwrapKey(MK82_KEYSAFE_OPGP_KEK_ID, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH,
                                            opgpHalTempBuffer, NULL, 0, nonce, tag);
    if (mk82CalleeRetVal != MK82_TRUE)
    {
        opgpHalFatalError();
    }
    calleeRetVal |= mbedtls_mpi_read_binary(&rsaKey->DQ, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, pq), (uint8_t*)&opgpHalTempBuffer,
                   OPGP_GLOBAL_PRIME_LENGTH);
    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, pqNonce), (uint8_t*)&nonce,
                   MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, pqTag), (uint8_t*)&tag,
                   MK82_KEYSAFE_TAG_LENGTH);
    mk82CalleeRetVal = mk82KeysafeUnwrapKey(MK82_KEYSAFE_OPGP_KEK_ID, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH,
                                            opgpHalTempBuffer, NULL, 0, nonce, tag);
    if (mk82CalleeRetVal != MK82_TRUE)
    {
        opgpHalFatalError();
    }
    calleeRetVal |= mbedtls_mpi_read_binary(&rsaKey->QP, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }

    rsaKey->len = OPGP_GLOBAL_MODULUS_LENGTH;

    opgpHalWipeoutBuffer(opgpHalTempBuffer, sizeof(opgpHalTempBuffer));
}

static void opgpHalGetKeyInfo(uint16_t keyType, uint32_t* keyOffset)
{
    uint32_t keyOffsetInternal;

    if (keyType == OPGP_GLOBAL_KEY_TYPE_SIGNATURE)
    {
        keyOffsetInternal = offsetof(OPGP_HAL_NVM_KEYS, signatureKey);
    }
    else if (keyType == OPGP_GLOBAL_KEY_TYPE_CONFIDENTIALITY)
    {
        keyOffsetInternal = offsetof(OPGP_HAL_NVM_KEYS, decryptionKey);
    }
    else if (keyType == OPGP_GLOBAL_KEY_TYPE_AUTHENTICATION)
    {
        keyOffsetInternal = offsetof(OPGP_HAL_NVM_KEYS, authenticationKey);
    }
    else
    {
        opgpHalFatalError();
    }

    if (keyOffset != NULL)
    {
        *keyOffset = keyOffsetInternal;
    }
}

static void opgpHalGetDoWithConstantLengthInfo(uint16_t tag, uint32_t* dataOffset, uint32_t* length)
{
    uint16_t tags[] = {OPGP_GLOBAL_TAG_SEX,
                       OPGP_GLOBAL_TAG_PW_STATUS_BYTES,
                       OPGP_GLOBAL_TAG_SIGNATURE_FINGERPRINT,
                       OPGP_GLOBAL_TAG_DECRYPTION_FINGERPRINT,
                       OPGP_GLOBAL_TAG_AUTHENTICATION_FINGERPRINT,
                       OPGP_GLOBAL_TAG_1ST_CA_FINGERPRINT,
                       OPGP_GLOBAL_TAG_2ND_CA_FINGERPRINT,
                       OPGP_GLOBAL_TAG_3RD_CA_FINGERPRINT,
                       OPGP_GLOBAL_TAG_GENERATION_DATE_TIME_SIGNATURE_KEY,
                       OPGP_GLOBAL_TAG_GENERATION_DATE_TIME_DECRYPTION_KEY,
                       OPGP_GLOBAL_TAG_GENERATION_DATE_TIME_AUTHENTICATION_KEY};

    uint32_t dataOffsets[] = {offsetof(OPGP_HAL_NVM_DATA, doSex),
                              offsetof(OPGP_HAL_NVM_DATA, doPW1_81_Status),
                              offsetof(OPGP_HAL_NVM_DATA, doSignatureFingerprint),
                              offsetof(OPGP_HAL_NVM_DATA, doDecryptionFingerprint),
                              offsetof(OPGP_HAL_NVM_DATA, doAuthenticationFingerprint),
                              offsetof(OPGP_HAL_NVM_DATA, do1stCaFingerprint),
                              offsetof(OPGP_HAL_NVM_DATA, do2ndCaFingerprint),
                              offsetof(OPGP_HAL_NVM_DATA, do3rdCaFingerprint),
                              offsetof(OPGP_HAL_NVM_DATA, doGenerationDateTimeSignatureKey),
                              offsetof(OPGP_HAL_NVM_DATA, doGenerationDateTimeDecryptionKey),
                              offsetof(OPGP_HAL_NVM_DATA, doGenerationDateTimeAuthenticationKey)};

    uint32_t lengths[] = {OPGP_GLOBAL_DO_SEX_LENGTH,
                          OPGP_GLOBAL_DO_PW_STATUS_BYTE_LENGTH,
                          OPGP_GLOBAL_DO_FINGERPRINT_LENGTH,
                          OPGP_GLOBAL_DO_FINGERPRINT_LENGTH,
                          OPGP_GLOBAL_DO_FINGERPRINT_LENGTH,
                          OPGP_GLOBAL_DO_CA_FINGERPRINT_LENGTH,
                          OPGP_GLOBAL_DO_CA_FINGERPRINT_LENGTH,
                          OPGP_GLOBAL_DO_CA_FINGERPRINT_LENGTH,
                          OPGP_GLOBAL_DO_GENERATION_DATE_TIME_LENGTH,
                          OPGP_GLOBAL_DO_GENERATION_DATE_TIME_LENGTH,
                          OPGP_GLOBAL_DO_GENERATION_DATE_TIME_LENGTH};

    uint32_t numberOfItems = sizeof(lengths) / sizeof(lengths[0]);

    uint32_t i = 0;
    uint16_t itemFound = OPGP_FALSE;

    for (i = 0; i < numberOfItems; i++)
    {
        if (tag == tags[i])
        {
            itemFound = OPGP_TRUE;
            break;
        }
    }

    if (itemFound == OPGP_FALSE)
    {
        opgpHalFatalError();
    }

    if (dataOffset != NULL)
    {
        *dataOffset = dataOffsets[i];
    }

    if (length != NULL)
    {
        *length = lengths[i];
    }
}

static void opgpHalGetDoWithVariableLengthInfo(uint16_t tag, uint32_t* dataOffset, uint32_t* lengthOffset,
                                               uint32_t* maximalLength)
{
    uint16_t tags[] = {OPGP_GLOBAL_TAG_PRIVATE_USE_1, OPGP_GLOBAL_TAG_PRIVATE_USE_2,      OPGP_GLOBAL_TAG_PRIVATE_USE_3,
                       OPGP_GLOBAL_TAG_PRIVATE_USE_4, OPGP_GLOBAL_TAG_LOGIN_DATA,         OPGP_GLOBAL_TAG_URL,
                       OPGP_GLOBAL_TAG_NAME,          OPGP_GLOBAL_TAG_LANGUAGE_PREFERENCE};

    uint32_t dataOffsets[] = {
        offsetof(OPGP_HAL_NVM_DATA, doPrivateUse1), offsetof(OPGP_HAL_NVM_DATA, doPrivateUse2),
        offsetof(OPGP_HAL_NVM_DATA, doPrivateUse3), offsetof(OPGP_HAL_NVM_DATA, doPrivateUse4),
        offsetof(OPGP_HAL_NVM_DATA, doLoginData),   offsetof(OPGP_HAL_NVM_DATA, doUrl),
        offsetof(OPGP_HAL_NVM_DATA, doName),        offsetof(OPGP_HAL_NVM_DATA, doLanguagePreference)};

    uint32_t lengthOffsets[] = {
        offsetof(OPGP_HAL_NVM_DATA, doPrivateUse1Length), offsetof(OPGP_HAL_NVM_DATA, doPrivateUse2Length),
        offsetof(OPGP_HAL_NVM_DATA, doPrivateUse3Length), offsetof(OPGP_HAL_NVM_DATA, doPrivateUse4Length),
        offsetof(OPGP_HAL_NVM_DATA, doLoginDataLength),   offsetof(OPGP_HAL_NVM_DATA, doUrlLength),
        offsetof(OPGP_HAL_NVM_DATA, doNameLength),        offsetof(OPGP_HAL_NVM_DATA, doLanguagePreferenceLength)};

    uint32_t maximalLengths[] = {
        OPGP_GLOBAL_DO_PRIVATE_USE_1_MAX_LENGTH, OPGP_GLOBAL_DO_PRIVATE_USE_2_MAX_LENGTH,
        OPGP_GLOBAL_DO_PRIVATE_USE_3_MAX_LENGTH, OPGP_GLOBAL_DO_PRIVATE_USE_4_MAX_LENGTH,
        OPGP_GLOBAL_DO_LOGIN_DATA_MAX_LENGTH,    OPGP_GLOBAL_DO_URL_MAX_LENGTH,
        OPGP_GLOBAL_DO_NAME_MAX_LENGTH,          OPGP_GLOBAL_DO_LANGUAGE_PREFERENCE_MAX_LENGTH};

    uint32_t numberOfItems = sizeof(maximalLengths) / sizeof(maximalLengths[0]);

    uint32_t i = 0;
    uint16_t itemFound = OPGP_FALSE;

    for (i = 0; i < numberOfItems; i++)
    {
        if (tag == tags[i])
        {
            itemFound = OPGP_TRUE;
            break;
        }
    }

    if (itemFound == OPGP_FALSE)
    {
        opgpHalFatalError();
    }

    if (dataOffset != NULL)
    {
        *dataOffset = dataOffsets[i];
    }

    if (lengthOffset != NULL)
    {
        *lengthOffset = lengthOffsets[i];
    }

    if (maximalLength != NULL)
    {
        *maximalLength = maximalLengths[i];
    }
}

static void opgpHalGetPinOffsets(uint8_t pinID, uint32_t* dataOffset, uint32_t* lengthOffset,
                                 uint32_t* errorCounterOffset)
{
    uint32_t lengthOffsetInternal;
    uint32_t dataOffsetInternal;
    uint32_t errorCounterOffsetInternal;

    if (pinID == OPGP_GLOBAL_PIN_ID_PW1_81)
    {
        lengthOffsetInternal = offsetof(OPGP_HAL_NVM_DATA, pw1Length);
        dataOffsetInternal = offsetof(OPGP_HAL_NVM_DATA, pw1);
        errorCounterOffsetInternal = offsetof(OPGP_HAL_NVM_COUNTERS, pw1ErrorCounter);
    }
    else if (pinID == OPGP_GLOBAL_PIN_ID_PW1_82)
    {
        lengthOffsetInternal = offsetof(OPGP_HAL_NVM_DATA, pw1Length);
        dataOffsetInternal = offsetof(OPGP_HAL_NVM_DATA, pw1);
        errorCounterOffsetInternal = offsetof(OPGP_HAL_NVM_COUNTERS, pw1ErrorCounter);
    }
    else if (pinID == OPGP_GLOBAL_PIN_ID_PW3)
    {
        lengthOffsetInternal = offsetof(OPGP_HAL_NVM_DATA, pw3Length);
        dataOffsetInternal = offsetof(OPGP_HAL_NVM_DATA, pw3);
        errorCounterOffsetInternal = offsetof(OPGP_HAL_NVM_COUNTERS, pw3ErrorCounter);
    }
    else if (pinID == OPGP_GLOBAL_PIN_ID_RC)
    {
        lengthOffsetInternal = offsetof(OPGP_HAL_NVM_DATA, rcLength);
        dataOffsetInternal = offsetof(OPGP_HAL_NVM_DATA, rc);
        errorCounterOffsetInternal = offsetof(OPGP_HAL_NVM_COUNTERS, rcErrorCounter);
    }
    else
    {
        opgpHalFatalError();
    }

    if (dataOffset != NULL)
    {
        *dataOffset = dataOffsetInternal;
    }

    if (lengthOffset != NULL)
    {
        *lengthOffset = lengthOffsetInternal;
    }

    if (errorCounterOffset != NULL)
    {
        *errorCounterOffset = errorCounterOffsetInternal;
    }
}

void opgpHalInit(void) {}

void opgpHalDeinit(void) {}

void opgpHalMemCpy(uint8_t* dst, uint8_t* src, uint16_t length) { mk82SystemMemCpy(dst, src, length); }

void opgpHalMemSet(uint8_t* dst, uint8_t value, uint16_t length) { mk82SystemMemSet(dst, value, length); }

uint16_t opgpHalMemCmp(uint8_t* array1, uint8_t* array2, uint16_t length)
{
    uint16_t comparisonResult;

    comparisonResult = mk82SystemMemCmp(array1, array2, length);

    if (comparisonResult != MK82_TRUE)
    {
        return OPGP_CMP_NOT_EQUAL;
    }
    else
    {
        return OPGP_CMP_EQUAL;
    }
}

void opgpHalWipeoutBuffer(uint8_t* buffer, uint16_t length)
{
    if (buffer == NULL)
    {
        opgpHalFatalError();
    }

    mk82SystemMemSet((void*)buffer, 0x00, length);
}

void opgpHalGetPinErrorCounter(uint8_t pinID, uint8_t* errorCounter)
{
    uint32_t offset;

    if (errorCounter == NULL)
    {
        opgpHalFatalError();
    }

    opgpHalGetPinOffsets(pinID, NULL, NULL, &offset);

    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_COUNTERS, offset, errorCounter, sizeof(uint8_t));
}

void opgpHalSetPinErrorCounter(uint8_t pinID, uint8_t errorCounter)
{
    uint32_t offset;

    opgpHalGetPinOffsets(pinID, NULL, NULL, &offset);

    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_COUNTERS, offset, &errorCounter, sizeof(uint8_t));
    mk82FsCommitWrite(MK82_FS_FILE_ID_OPGP_COUNTERS);
}

void opgpHalGetPinHashAndLength(uint8_t pinID, uint8_t* pinHash, uint32_t* pinLength)
{
    uint32_t lengthOffset;
    uint32_t dataOffset;

    if ((pinHash == NULL) || (pinLength == NULL))
    {
        opgpHalFatalError();
    }

    opgpHalGetPinOffsets(pinID, &dataOffset, &lengthOffset, NULL);

    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_DATA, lengthOffset, (uint8_t*)pinLength, sizeof(uint32_t));
    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_DATA, dataOffset, pinHash, OPGP_GLOBAL_PIN_HASH_LENGTH);
}

void opgpHalGetPinLength(uint8_t pinID, uint32_t* pinLength)
{
    uint32_t lengthOffset;

    if (pinLength == NULL)
    {
        opgpHalFatalError();
    }

    opgpHalGetPinOffsets(pinID, NULL, &lengthOffset, NULL);

    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_DATA, lengthOffset, (uint8_t*)pinLength, sizeof(uint32_t));
}

void opgpHalSetPinHashAndLength(uint8_t pinID, uint8_t* pinHash, uint32_t pinLength)
{
    uint32_t lengthOffset;
    uint32_t dataOffset;

    if (pinHash == NULL)
    {
        opgpHalFatalError();
    }

    opgpHalGetPinOffsets(pinID, &dataOffset, &lengthOffset, NULL);

    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, lengthOffset, (uint8_t*)&pinLength, sizeof(uint8_t));
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, dataOffset, pinHash, OPGP_GLOBAL_PIN_HASH_LENGTH);
    mk82FsCommitWrite(MK82_FS_FILE_ID_OPGP_DATA);
}

void opgpHalSetPW1HashAndLengthAndUnblock(uint8_t* pinHash, uint32_t pinLength)
{
    uint32_t lengthOffset;
    uint32_t dataOffset;
    uint32_t counterValueOffset;
    uint8_t counterValue = OPGP_GLOBAL_PIN_INITIAL_ERROR_COUNTER_VALUE;

    if (pinHash == NULL)
    {
        opgpHalFatalError();
    }

    lengthOffset = offsetof(OPGP_HAL_NVM_DATA, pw1Length);
    dataOffset = offsetof(OPGP_HAL_NVM_DATA, pw1);
    counterValueOffset = offsetof(OPGP_HAL_NVM_COUNTERS, pw1ErrorCounter);

    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, lengthOffset, (uint8_t*)&pinLength, sizeof(uint8_t));
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, dataOffset, pinHash, OPGP_GLOBAL_PIN_HASH_LENGTH);
    mk82FsCommitWrite(MK82_FS_FILE_ID_OPGP_DATA);

    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_COUNTERS, counterValueOffset, (uint8_t*)&counterValue, sizeof(uint8_t));
    mk82FsCommitWrite(MK82_FS_FILE_ID_OPGP_COUNTERS);
}

void opgpHalSetRCHashAndLengthAndSetErrorCounter(uint8_t* pinHash, uint32_t pinLength, uint8_t errorCounter)
{
    uint32_t lengthOffset;
    uint32_t dataOffset;
    uint32_t counterValueOffset;

    if (pinHash == NULL)
    {
        opgpHalFatalError();
    }

    lengthOffset = offsetof(OPGP_HAL_NVM_DATA, rcLength);
    dataOffset = offsetof(OPGP_HAL_NVM_DATA, rc);
    counterValueOffset = offsetof(OPGP_HAL_NVM_COUNTERS, rcErrorCounter);

    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, lengthOffset, (uint8_t*)&pinLength, sizeof(uint8_t));
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, dataOffset, pinHash, OPGP_GLOBAL_PIN_HASH_LENGTH);
    mk82FsCommitWrite(MK82_FS_FILE_ID_OPGP_DATA);

    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_COUNTERS, counterValueOffset, (uint8_t*)&errorCounter, sizeof(uint8_t));
    mk82FsCommitWrite(MK82_FS_FILE_ID_OPGP_COUNTERS);
}

void opgpHalCalculatePinHash(uint8_t* pinValue, uint32_t pinLength, uint8_t* pinHash)
{
    if ((pinValue == NULL) || (pinHash == NULL))
    {
        opgpHalFatalError();
    }

    mbedtls_sha256(pinValue, pinLength, pinHash, 0);
}

void opgpHalGetDataObject(uint16_t tag, uint8_t* data)
{
    uint32_t offset;
    uint32_t length;

    if (data == NULL)
    {
        opgpHalFatalError();
    }

    opgpHalGetDoWithConstantLengthInfo(tag, &offset, &length);

    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_DATA, offset, data, length);
}

void opgpHalGetDataObjectWithLength(uint16_t tag, uint8_t* data, uint32_t* length)
{
    uint32_t dataOffset;
    uint32_t lengthOffset;

    if ((data == NULL) || (length == NULL))
    {
        opgpHalFatalError();
    }

    if (tag == OPGP_GLOBAL_TAG_CARD_HOLDER_CERTIFICATE)
    {
        dataOffset = offsetof(OPGP_HAL_NVM_CERTIFICATES, doCertificate);
        lengthOffset = offsetof(OPGP_HAL_NVM_CERTIFICATES, doCertificateLength);

        mk82FsReadFile(MK82_FS_FILE_ID_OPGP_CERTIFICATES, lengthOffset, (uint8_t*)length, sizeof(uint32_t));
        mk82FsReadFile(MK82_FS_FILE_ID_OPGP_CERTIFICATES, dataOffset, data, *length);
    }
    else
    {
        opgpHalGetDoWithVariableLengthInfo(tag, &dataOffset, &lengthOffset, NULL);

        mk82FsReadFile(MK82_FS_FILE_ID_OPGP_DATA, lengthOffset, (uint8_t*)length, sizeof(uint32_t));
        mk82FsReadFile(MK82_FS_FILE_ID_OPGP_DATA, dataOffset, data, *length);
    }
}

void opgpHalSetDataObject(uint16_t tag, uint8_t* data)
{
    uint32_t offset;
    uint32_t length;

    if (data == NULL)
    {
        opgpHalFatalError();
    }

    opgpHalGetDoWithConstantLengthInfo(tag, &offset, &length);

    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, offset, data, length);
    mk82FsCommitWrite(MK82_FS_FILE_ID_OPGP_DATA);
}

void opgpHalSetDataObjectWithLength(uint16_t tag, uint8_t* data, uint32_t length)
{
    uint32_t dataOffset;
    uint32_t lengthOffset;
    uint32_t maximalLength;

    if (data == NULL)
    {
        opgpHalFatalError();
    }

    if (tag == OPGP_GLOBAL_TAG_CARD_HOLDER_CERTIFICATE)
    {
        dataOffset = offsetof(OPGP_HAL_NVM_CERTIFICATES, doCertificate);
        lengthOffset = offsetof(OPGP_HAL_NVM_CERTIFICATES, doCertificateLength);
        maximalLength = OPGP_GLOBAL_DO_CERTIFICATE_MAX_LENGTH;

        if (length > maximalLength)
        {
            opgpHalFatalError();
        }

        mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_CERTIFICATES, lengthOffset, (uint8_t*)&length, sizeof(uint32_t));
        mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_CERTIFICATES, dataOffset, data, length);
        mk82FsCommitWrite(MK82_FS_FILE_ID_OPGP_CERTIFICATES);
    }
    else
    {
        opgpHalGetDoWithVariableLengthInfo(tag, &dataOffset, &lengthOffset, &maximalLength);

        if (length > maximalLength)
        {
            opgpHalFatalError();
        }

        mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, lengthOffset, (uint8_t*)&length, sizeof(uint32_t));
        mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, dataOffset, data, length);
        mk82FsCommitWrite(MK82_FS_FILE_ID_OPGP_DATA);
    }
}

void opgpHalGetSerialNumber(uint8_t* serialNumber)
{
    if (serialNumber == NULL)
    {
        opgpHalFatalError();
    }

    mk82SystemGetSerialNumber((uint32_t*)serialNumber);
}

void opgpHalGetCardState(uint8_t* cardState)
{
    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_DATA, offsetof(OPGP_HAL_NVM_DATA, cardState), (uint8_t*)cardState,
                   sizeof(uint8_t));
}

void opgpHalSetCardState(uint8_t cardState)
{
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, offsetof(OPGP_HAL_NVM_DATA, cardState), (uint8_t*)&cardState,
                    sizeof(uint8_t));
    mk82FsCommitWrite(MK82_FS_FILE_ID_OPGP_DATA);
}

void opgpHalIncrementSignatureCounter(void)
{
    uint32_t counterValue;

    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_COUNTERS, offsetof(OPGP_HAL_NVM_COUNTERS, signatureCounter),
                   (uint8_t*)&counterValue, sizeof(uint32_t));

    counterValue = (counterValue + 1) % 0x01000000;

    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_COUNTERS, offsetof(OPGP_HAL_NVM_COUNTERS, signatureCounter),
                    (uint8_t*)&counterValue, sizeof(uint32_t));
    mk82FsCommitWrite(MK82_FS_FILE_ID_OPGP_COUNTERS);
}

void opgpHalGetSignatureCounter(uint32_t* signatureCounter)
{
    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_COUNTERS, offsetof(OPGP_HAL_NVM_COUNTERS, signatureCounter),
                   (uint8_t*)signatureCounter, sizeof(uint32_t));
}

void opgpHalResetSignatureCounter(void)
{
    uint32_t counterValue = 0;

    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_COUNTERS, offsetof(OPGP_HAL_NVM_COUNTERS, signatureCounter),
                    (uint8_t*)&counterValue, sizeof(uint32_t));
    mk82FsCommitWrite(MK82_FS_FILE_ID_OPGP_COUNTERS);
}

uint16_t opgpHalImportKey(uint16_t keyType, uint8_t* e, uint8_t* p, uint8_t* q)
{
    int retVal = OPGP_INVALID_CRYPTO_DATA_ERROR;
    int calleeRetVal;
    mbedtls_rsa_context importedKey;
    mbedtls_mpi p1;
    mbedtls_mpi q1;
    mbedtls_mpi h;
    mbedtls_mpi d;
    uint32_t keyOffset;
    uint16_t trueFalse;
    uint8_t tag[MK82_KEYSAFE_TAG_LENGTH];
    uint8_t nonce[MK82_KEYSAFE_NONCE_LENGTH];

    if ((e == NULL) || (p == NULL) || (q == NULL))
    {
        opgpHalFatalError();
    }

    opgpHalGetKeyInfo(keyType, &keyOffset);

    mbedtls_mpi_init(&p1);
    mbedtls_mpi_init(&q1);
    mbedtls_mpi_init(&h);
    mbedtls_mpi_init(&d);

    mbedtls_rsa_init(&importedKey, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);

    calleeRetVal = 0x00;

    calleeRetVal |= mbedtls_mpi_read_binary(&importedKey.E, e, OPGP_GLOBAL_PUBLIC_EXPONENT_LENGTH);
    calleeRetVal |= mbedtls_mpi_read_binary(&importedKey.P, p, OPGP_GLOBAL_PRIME_LENGTH);
    calleeRetVal |= mbedtls_mpi_read_binary(&importedKey.Q, q, OPGP_GLOBAL_PRIME_LENGTH);

    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }

    calleeRetVal = mbedtls_mpi_mul_mpi(&importedKey.N, &importedKey.P, &importedKey.Q);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }

    calleeRetVal = mbedtls_mpi_sub_int(&p1, &importedKey.P, 1);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }

    calleeRetVal = mbedtls_mpi_sub_int(&q1, &importedKey.Q, 1);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }

    calleeRetVal = mbedtls_mpi_mul_mpi(&h, &p1, &q1);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }

    calleeRetVal = mbedtls_mpi_inv_mod(&importedKey.D, &importedKey.E, &h);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }

    calleeRetVal = mbedtls_mpi_mod_mpi(&importedKey.DP, &importedKey.D, &p1);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }

    calleeRetVal = mbedtls_mpi_mod_mpi(&importedKey.DQ, &importedKey.D, &q1);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }

    calleeRetVal = mbedtls_mpi_inv_mod(&importedKey.QP, &importedKey.Q, &importedKey.P);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }

    calleeRetVal = mbedtls_rsa_check_privkey(&importedKey);

    if (calleeRetVal != 0)
    {
        retVal = OPGP_INVALID_CRYPTO_DATA_ERROR;
        goto END;
    }

    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, e), e,
                    OPGP_GLOBAL_PUBLIC_EXPONENT_LENGTH);

    mk82KeysafeWrapKey(MK82_KEYSAFE_OPGP_KEK_ID, p, OPGP_GLOBAL_PRIME_LENGTH, opgpHalTempBuffer, NULL, 0, nonce, tag);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, p), opgpHalTempBuffer,
                    OPGP_GLOBAL_PRIME_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, pNonce), nonce,
                    MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, pTag), tag, MK82_KEYSAFE_TAG_LENGTH);

    mk82KeysafeWrapKey(MK82_KEYSAFE_OPGP_KEK_ID, q, OPGP_GLOBAL_PRIME_LENGTH, opgpHalTempBuffer, NULL, 0, nonce, tag);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, q), opgpHalTempBuffer,
                    OPGP_GLOBAL_PRIME_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, qNonce), nonce,
                    MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, qTag), tag, MK82_KEYSAFE_TAG_LENGTH);

    calleeRetVal = mbedtls_mpi_write_binary(&importedKey.N, opgpHalTempBuffer, OPGP_GLOBAL_MODULUS_LENGTH);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, n), opgpHalTempBuffer,
                    OPGP_GLOBAL_MODULUS_LENGTH);

    calleeRetVal = mbedtls_mpi_write_binary(&importedKey.DP, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }
    mk82KeysafeWrapKey(MK82_KEYSAFE_OPGP_KEK_ID, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH, opgpHalTempBuffer, NULL,
                       0, nonce, tag);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, dp1), opgpHalTempBuffer,
                    OPGP_GLOBAL_PRIME_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, dp1Nonce), nonce,
                    MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, dp1Tag), tag,
                    MK82_KEYSAFE_TAG_LENGTH);

    calleeRetVal = mbedtls_mpi_write_binary(&importedKey.DQ, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }
    mk82KeysafeWrapKey(MK82_KEYSAFE_OPGP_KEK_ID, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH, opgpHalTempBuffer, NULL,
                       0, nonce, tag);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, dq1), opgpHalTempBuffer,
                    OPGP_GLOBAL_PRIME_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, dq1Nonce), nonce,
                    MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, dq1Tag), tag,
                    MK82_KEYSAFE_TAG_LENGTH);

    calleeRetVal = mbedtls_mpi_write_binary(&importedKey.QP, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }
    mk82KeysafeWrapKey(MK82_KEYSAFE_OPGP_KEK_ID, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH, opgpHalTempBuffer, NULL,
                       0, nonce, tag);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, pq), opgpHalTempBuffer,
                    OPGP_GLOBAL_PRIME_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, pqNonce), nonce,
                    MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, pqTag), tag, MK82_KEYSAFE_TAG_LENGTH);

    trueFalse = OPGP_TRUE;
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, keyInitialized), (uint8_t*)&trueFalse,
                    sizeof(uint16_t));

    mk82FsCommitWrite(MK82_FS_FILE_ID_OPGP_KEYS);

    retVal = OPGP_NO_ERROR;

END:
    opgpHalWipeoutBuffer(opgpHalTempBuffer, sizeof(opgpHalTempBuffer));

    mbedtls_mpi_free(&p1);
    mbedtls_mpi_free(&q1);
    mbedtls_mpi_free(&h);
    mbedtls_mpi_free(&d);
    mbedtls_rsa_free(&importedKey);

    return retVal;
}

void opgpHalIsKeyInitialized(uint16_t keyType, uint16_t* result)
{
    uint32_t keyOffset;

    if (result == NULL)
    {
        opgpHalFatalError();
    }

    opgpHalGetKeyInfo(keyType, &keyOffset);

    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, keyInitialized), (uint8_t*)result,
                   sizeof(uint16_t));
}

void opgpHalSign(uint8_t* dataToSign, uint32_t dataToSignLength, uint8_t* signature)
{
    uint16_t keyInitialized;
    mbedtls_rsa_context rsaKey;
    int calleeRetVal;

    if ((dataToSign == NULL) || (signature == NULL))
    {
        opgpHalFatalError();
    }

    opgpHalIsKeyInitialized(OPGP_GLOBAL_KEY_TYPE_SIGNATURE, &keyInitialized);

    if (keyInitialized != OPGP_TRUE)
    {
        opgpHalFatalError();
    }

    mbedtls_rsa_init(&rsaKey, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);

    opgpHalGetRsaKeyFromFile(OPGP_GLOBAL_KEY_TYPE_SIGNATURE, &rsaKey);

    calleeRetVal = mbedtls_rsa_rsassa_pkcs1_v15_sign(&rsaKey, mk82SystemGetRandomForTLS, NULL, MBEDTLS_RSA_PRIVATE,
                                                     MBEDTLS_MD_NONE, dataToSignLength, dataToSign, opgpHalTempBuffer);

    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }

    opgpHalMemCpy(signature, opgpHalTempBuffer, OPGP_GLOBAL_MODULUS_LENGTH);

    mbedtls_rsa_free(&rsaKey);
}

uint16_t opgpHalDecipher(uint8_t* dataToDecipher, uint8_t* decipheredData, uint32_t* decipheredDataLength)
{
    uint16_t keyInitialized;
    mbedtls_rsa_context rsaKey;
    int calleeRetVal;
    uint16_t retVal = OPGP_INVALID_CRYPTO_DATA_ERROR;

    if ((dataToDecipher == NULL) || (decipheredData == NULL) || (decipheredDataLength == NULL))
    {
        opgpHalFatalError();
    }

    opgpHalIsKeyInitialized(OPGP_GLOBAL_KEY_TYPE_CONFIDENTIALITY, &keyInitialized);

    if (keyInitialized != OPGP_TRUE)
    {
        opgpHalFatalError();
    }

    mbedtls_rsa_init(&rsaKey, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);

    opgpHalGetRsaKeyFromFile(OPGP_GLOBAL_KEY_TYPE_CONFIDENTIALITY, &rsaKey);

    calleeRetVal = mbedtls_rsa_rsaes_pkcs1_v15_decrypt(&rsaKey, mk82SystemGetRandomForTLS, NULL, MBEDTLS_RSA_PRIVATE,
                                                       (size_t*)decipheredDataLength, dataToDecipher, opgpHalTempBuffer,
                                                       OPGP_GLOBAL_MODULUS_LENGTH);

    if (calleeRetVal != 0)
    {
        if (calleeRetVal == MBEDTLS_ERR_RSA_INVALID_PADDING)
        {
            retVal = OPGP_INVALID_CRYPTO_DATA_ERROR;
            goto END;
        }
        else
        {
            opgpHalFatalError();
        }
    }

    opgpHalMemCpy(decipheredData, opgpHalTempBuffer, *decipheredDataLength);

    retVal = OPGP_NO_ERROR;

END:
    mbedtls_rsa_free(&rsaKey);

    return retVal;
}

void opgpHalInternalAuthenticate(uint8_t* authenticationInput, uint32_t authenticationInputLength, uint8_t* signature)
{
    uint16_t keyInitialized;
    mbedtls_rsa_context rsaKey;
    int calleeRetVal;

    if ((authenticationInput == NULL) || (signature == NULL))
    {
        opgpHalFatalError();
    }

    opgpHalIsKeyInitialized(OPGP_GLOBAL_KEY_TYPE_AUTHENTICATION, &keyInitialized);

    if (keyInitialized != OPGP_TRUE)
    {
        opgpHalFatalError();
    }

    mbedtls_rsa_init(&rsaKey, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);

    opgpHalGetRsaKeyFromFile(OPGP_GLOBAL_KEY_TYPE_AUTHENTICATION, &rsaKey);

    calleeRetVal = mbedtls_rsa_rsassa_pkcs1_v15_sign(&rsaKey, mk82SystemGetRandomForTLS, NULL, MBEDTLS_RSA_PRIVATE,
                                                     MBEDTLS_MD_NONE, authenticationInputLength, authenticationInput,
                                                     opgpHalTempBuffer);

    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }

    opgpHalMemCpy(signature, opgpHalTempBuffer, OPGP_GLOBAL_MODULUS_LENGTH);

    mbedtls_rsa_free(&rsaKey);
}

void opgpHalGenerateKeyPair(uint16_t keyType)
{
    int calleeRetVal;
    mbedtls_rsa_context generatedKey;
    uint32_t keyOffset;
    uint16_t trueFalse;
    uint8_t tag[MK82_KEYSAFE_TAG_LENGTH];
    uint8_t nonce[MK82_KEYSAFE_NONCE_LENGTH];

    opgpHalGetKeyInfo(keyType, &keyOffset);

    mbedtls_rsa_init(&generatedKey, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);

    calleeRetVal = mbedtls_rsa_gen_key(&generatedKey, mk82SystemGetRandomForTLS, NULL, 2048, 65537);

    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }

    calleeRetVal = mbedtls_mpi_write_binary(&generatedKey.N, opgpHalTempBuffer, OPGP_GLOBAL_MODULUS_LENGTH);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, n), opgpHalTempBuffer,
                    OPGP_GLOBAL_MODULUS_LENGTH);

    calleeRetVal = mbedtls_mpi_write_binary(&generatedKey.E, opgpHalTempBuffer, OPGP_GLOBAL_PUBLIC_EXPONENT_LENGTH);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, e), opgpHalTempBuffer,
                    OPGP_GLOBAL_PUBLIC_EXPONENT_LENGTH);

    calleeRetVal = mbedtls_mpi_write_binary(&generatedKey.P, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }
    mk82KeysafeWrapKey(MK82_KEYSAFE_OPGP_KEK_ID, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH, opgpHalTempBuffer, NULL,
                       0, nonce, tag);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, p), opgpHalTempBuffer,
                    OPGP_GLOBAL_PRIME_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, pNonce), nonce,
                    MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, pTag), tag, MK82_KEYSAFE_TAG_LENGTH);

    calleeRetVal = mbedtls_mpi_write_binary(&generatedKey.Q, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }
    mk82KeysafeWrapKey(MK82_KEYSAFE_OPGP_KEK_ID, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH, opgpHalTempBuffer, NULL,
                       0, nonce, tag);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, q), opgpHalTempBuffer,
                    OPGP_GLOBAL_PRIME_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, qNonce), nonce,
                    MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, qTag), tag, MK82_KEYSAFE_TAG_LENGTH);

    calleeRetVal = mbedtls_mpi_write_binary(&generatedKey.DP, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }
    mk82KeysafeWrapKey(MK82_KEYSAFE_OPGP_KEK_ID, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH, opgpHalTempBuffer, NULL,
                       0, nonce, tag);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, dp1), opgpHalTempBuffer,
                    OPGP_GLOBAL_PRIME_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, dp1Nonce), nonce,
                    MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, dp1Tag), tag,
                    MK82_KEYSAFE_TAG_LENGTH);

    calleeRetVal = mbedtls_mpi_write_binary(&generatedKey.DQ, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }
    mk82KeysafeWrapKey(MK82_KEYSAFE_OPGP_KEK_ID, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH, opgpHalTempBuffer, NULL,
                       0, nonce, tag);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, dq1), opgpHalTempBuffer,
                    OPGP_GLOBAL_PRIME_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, dq1Nonce), nonce,
                    MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, dq1Tag), tag,
                    MK82_KEYSAFE_TAG_LENGTH);

    calleeRetVal = mbedtls_mpi_write_binary(&generatedKey.QP, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH);
    if (calleeRetVal != 0)
    {
        opgpHalFatalError();
    }
    mk82KeysafeWrapKey(MK82_KEYSAFE_OPGP_KEK_ID, opgpHalTempBuffer, OPGP_GLOBAL_PRIME_LENGTH, opgpHalTempBuffer, NULL,
                       0, nonce, tag);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, pq), opgpHalTempBuffer,
                    OPGP_GLOBAL_PRIME_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, pqNonce), nonce,
                    MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, pqTag), tag, MK82_KEYSAFE_TAG_LENGTH);

    trueFalse = OPGP_TRUE;
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, keyInitialized), (uint8_t*)&trueFalse,
                    sizeof(uint16_t));

    mk82FsCommitWrite(MK82_FS_FILE_ID_OPGP_KEYS);

    mbedtls_rsa_free(&generatedKey);
    opgpHalWipeoutBuffer(opgpHalTempBuffer, sizeof(opgpHalTempBuffer));
}

void opgpHalGetPublicKey(uint16_t keyType, uint8_t* modulus, uint8_t* publicExponent)
{
    uint32_t keyOffset;
    uint16_t keyInitialized;

    if ((modulus == NULL) || (publicExponent == NULL))
    {
        opgpHalFatalError();
    }

    opgpHalIsKeyInitialized(keyType, &keyInitialized);

    if (keyInitialized != OPGP_TRUE)
    {
        opgpHalFatalError();
    }

    opgpHalGetKeyInfo(keyType, &keyOffset);

    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, n), modulus,
                   OPGP_GLOBAL_MODULUS_LENGTH);
    mk82FsReadFile(MK82_FS_FILE_ID_OPGP_KEYS, keyOffset + offsetof(OPGP_HAL_KEY, e), publicExponent,
                   OPGP_GLOBAL_PUBLIC_EXPONENT_LENGTH);
}

void opgpHalGetRandom(uint8_t* buffer, uint32_t length)
{
    if (buffer == NULL)
    {
        opgpHalFatalError();
    }

    mk82SystemGetRandom(buffer, length);
}

void opgpHalWipeout(void)
{
    uint32_t bytesWritten = 0;
    uint16_t trueOrFalse;
    uint8_t byteValue;
    OPGP_HAL_NVM_COUNTERS counters = {3, 3, 0, 0};

    uint8_t pw1Hash[] = {0x8d, 0x96, 0x9e, 0xef, 0x6e, 0xca, 0xd3, 0xc2, 0x9a, 0x3a, 0x62,
                         0x92, 0x80, 0xe6, 0x86, 0xcf, 0x0c, 0x3f, 0x5d, 0x5a, 0x86, 0xaf,
                         0xf3, 0xca, 0x12, 0x02, 0x0c, 0x92, 0x3a, 0xdc, 0x6c, 0x92};

    uint8_t pw3Hash[] = {0xef, 0x79, 0x7c, 0x81, 0x18, 0xf0, 0x2d, 0xfb, 0x64, 0x96, 0x07,
                         0xdd, 0x5d, 0x3f, 0x8c, 0x76, 0x23, 0x04, 0x8c, 0x9c, 0x06, 0x3d,
                         0x53, 0x2c, 0xc9, 0x5c, 0x5e, 0xd7, 0xa8, 0x98, 0xa6, 0x4f};

    uint8_t rcHash[] = {0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14, 0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
                        0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c, 0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55};

    opgpHalMemSet(opgpHalTempBuffer, 0x00, sizeof(opgpHalTempBuffer));

    while ((bytesWritten + sizeof(opgpHalTempBuffer)) < sizeof(OPGP_HAL_NVM_KEYS))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, bytesWritten, opgpHalTempBuffer, sizeof(opgpHalTempBuffer));
        bytesWritten += sizeof(opgpHalTempBuffer);
    }

    if (bytesWritten < sizeof(OPGP_HAL_NVM_KEYS))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, bytesWritten, opgpHalTempBuffer,
                        (sizeof(OPGP_HAL_NVM_KEYS) - bytesWritten));
    }

    trueOrFalse = OPGP_FALSE;
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, offsetof(OPGP_HAL_NVM_KEYS, signatureKey.keyInitialized),
                    (uint8_t*)&trueOrFalse, sizeof(trueOrFalse));
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, offsetof(OPGP_HAL_NVM_KEYS, decryptionKey.keyInitialized),
                    (uint8_t*)&trueOrFalse, sizeof(trueOrFalse));
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_KEYS, offsetof(OPGP_HAL_NVM_KEYS, authenticationKey.keyInitialized),
                    (uint8_t*)&trueOrFalse, sizeof(trueOrFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_OPGP_KEYS);

    bytesWritten = 0;

    while ((bytesWritten + sizeof(opgpHalTempBuffer)) < sizeof(OPGP_HAL_NVM_DATA))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, bytesWritten, opgpHalTempBuffer, sizeof(opgpHalTempBuffer));
        bytesWritten += sizeof(opgpHalTempBuffer);
    }

    if (bytesWritten < sizeof(OPGP_HAL_NVM_DATA))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, bytesWritten, opgpHalTempBuffer,
                        (sizeof(OPGP_HAL_NVM_DATA) - bytesWritten));
    }

    byteValue = 6;
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, offsetof(OPGP_HAL_NVM_DATA, pw1Length), (uint8_t*)&byteValue,
                    sizeof(byteValue));
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, offsetof(OPGP_HAL_NVM_DATA, pw1), pw1Hash, OPGP_GLOBAL_PIN_HASH_LENGTH);
    byteValue = 8;
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, offsetof(OPGP_HAL_NVM_DATA, pw3Length), (uint8_t*)&byteValue,
                    sizeof(byteValue));
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, offsetof(OPGP_HAL_NVM_DATA, pw3), pw3Hash, OPGP_GLOBAL_PIN_HASH_LENGTH);
    byteValue = 0;
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, offsetof(OPGP_HAL_NVM_DATA, rcLength), (uint8_t*)&byteValue,
                    sizeof(byteValue));
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, offsetof(OPGP_HAL_NVM_DATA, rc), rcHash, OPGP_GLOBAL_PIN_HASH_LENGTH);

    byteValue = OPGP_GLOBAL_PW1_81_VALID_FOR_MULTIPLE_COMMANDS;
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, offsetof(OPGP_HAL_NVM_DATA, doPW1_81_Status), (uint8_t*)&byteValue,
                    sizeof(byteValue));

    byteValue = OPGP_GLOBAL_CARD_STATE_INITIALIZATION;
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, offsetof(OPGP_HAL_NVM_DATA, cardState), (uint8_t*)&byteValue,
                    sizeof(byteValue));

    mk82FsCommitWrite(MK82_FS_FILE_ID_OPGP_DATA);

    bytesWritten = 0;

    while ((bytesWritten + sizeof(opgpHalTempBuffer)) < sizeof(OPGP_HAL_NVM_CERTIFICATES))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_CERTIFICATES, bytesWritten, opgpHalTempBuffer, sizeof(opgpHalTempBuffer));
        bytesWritten += sizeof(opgpHalTempBuffer);
    }

    if (bytesWritten < sizeof(OPGP_HAL_NVM_CERTIFICATES))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_CERTIFICATES, bytesWritten, opgpHalTempBuffer,
                        (sizeof(OPGP_HAL_NVM_CERTIFICATES) - bytesWritten));
    }

    mk82FsCommitWrite(MK82_FS_FILE_ID_OPGP_CERTIFICATES);

    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_COUNTERS, 0, (uint8_t*)&counters, sizeof(counters));
    mk82FsCommitWrite(MK82_FS_FILE_ID_OPGP_COUNTERS);

    byteValue = OPGP_GLOBAL_CARD_STATE_OPERATIONAL;
    mk82FsWriteFile(MK82_FS_FILE_ID_OPGP_DATA, offsetof(OPGP_HAL_NVM_DATA, cardState), (uint8_t*)&byteValue,
                    sizeof(byteValue));

    mk82FsCommitWrite(MK82_FS_FILE_ID_OPGP_DATA);
}

void opgpHalFatalError(void) { mk82SystemFatalError(); }
