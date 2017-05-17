/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <otpGlobal.h>
#include <otpGlobalInt.h>
#include <otpHal.h>
#include <hal/k82/otpHalInt.h>

#include "mk82Global.h"
#include "mk82System.h"
#include "mk82KeySafe.h"
#include "mk82Fs.h"

#include "mbedtls/md.h"

static uint16_t otpHalReferenceTimeSet;
static uint32_t otpHalExternalReferenceTimeInSec;
static uint64_t otpHalInternalReferenceTimeInMs;

void otpHalInit(void) { otpHalReferenceTimeSet = OTP_FALSE; }

void otpHalDeinit(void) {}

uint16_t otpHalMemCmp(uint8_t* array1, uint8_t* array2, uint16_t length)
{
    uint16_t comparisonResult;

    comparisonResult = mk82SystemMemCmp(array1, array2, length);

    if (comparisonResult != OTP_TRUE)
    {
        return OTP_CMP_NOT_EQUAL;
    }
    else
    {
        return OTP_CMP_EQUAL;
    }
}

void otpHalMemSet(uint8_t* dst, uint8_t value, uint16_t length) { mk82SystemMemSet(dst, value, length); }

void otpHalMemCpy(uint8_t* dst, uint8_t* src, uint16_t length) { mk82SystemMemCpy(dst, src, length); }

void otpHalSetKeySetTypeAndResetCounter(uint8_t* key, uint32_t keyLength, uint16_t type)
{
    uint8_t paddedKey[OTP_GLOBAL_MAX_KEYLENGTH];
    uint8_t encryptedKey[OTP_GLOBAL_MAX_KEYLENGTH];
    uint8_t nonce[MK82_KEYSAFE_NONCE_LENGTH];
    uint8_t tag[MK82_KEYSAFE_TAG_LENGTH];
    uint16_t trueFalse;
    uint64_t zeroCounter = 0;

    if (key == NULL)
    {
        otpHalFatalError();
    }

    if ((keyLength < OTP_GLOBAL_MIN_KEYLENGTH) || (keyLength > OTP_GLOBAL_MAX_KEYLENGTH))
    {
        otpHalFatalError();
    }

    if ((type != OTP_GLOBAL_TYPE_HOTP) && (type != OTP_GLOBAL_TYPE_TOTP))
    {
        otpHalFatalError();
    }

    mk82SystemMemSet(paddedKey, 0x00, sizeof(paddedKey));

    mk82SystemMemCpy(paddedKey, key, keyLength);

    mk82KeysafeWrapKey(MK82_KEYSAFE_OTP_KEK_ID, paddedKey, sizeof(paddedKey), encryptedKey, NULL, 0, nonce, tag);

    mk82SystemMemSet(paddedKey, 0x00, sizeof(paddedKey));

    trueFalse = OTP_FALSE;
    mk82FsWriteFile(MK82_FS_FILE_ID_OTP_KEYS, offsetof(OTP_HAL_NVM_KEYS, keyInitialized), (uint8_t*)&trueFalse,
                    sizeof(trueFalse));
    mk82FsCommitWrite(MK82_FS_FILE_ID_OTP_KEYS);

    mk82FsWriteFile(MK82_FS_FILE_ID_OTP_COUNTERS, offsetof(OTP_HAL_NVM_COUNTERS, counter), (uint8_t*)&zeroCounter,
                    sizeof(zeroCounter));
    mk82FsCommitWrite(MK82_FS_FILE_ID_OTP_COUNTERS);

    mk82FsWriteFile(MK82_FS_FILE_ID_OTP_DATA, offsetof(OTP_HAL_NVM_DATA, type), (uint8_t*)&type, sizeof(type));
    mk82FsCommitWrite(MK82_FS_FILE_ID_OTP_DATA);

    mk82FsWriteFile(MK82_FS_FILE_ID_OTP_KEYS, offsetof(OTP_HAL_NVM_KEYS, key), encryptedKey, OTP_GLOBAL_MAX_KEYLENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OTP_KEYS, offsetof(OTP_HAL_NVM_KEYS, keyNonce), nonce, MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OTP_KEYS, offsetof(OTP_HAL_NVM_KEYS, keyTag), tag, MK82_KEYSAFE_TAG_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_OTP_KEYS, offsetof(OTP_HAL_NVM_KEYS, keyLength), (uint8_t*)&keyLength,
                    sizeof(keyLength));
    trueFalse = OTP_TRUE;
    mk82FsWriteFile(MK82_FS_FILE_ID_OTP_KEYS, offsetof(OTP_HAL_NVM_KEYS, keyInitialized), (uint8_t*)&trueFalse,
                    sizeof(trueFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_OTP_KEYS);
}

void otpHalSetOptions(uint8_t numberOfDigits)
{
    if ((numberOfDigits < OTP_GLOBAL_MIN_DIGITS) || (numberOfDigits > OTP_GLOBAL_MAX_DIGITS))
    {
        otpHalFatalError();
    }

    mk82FsWriteFile(MK82_FS_FILE_ID_OTP_DATA, offsetof(OTP_HAL_NVM_DATA, numberOfDigits), (uint8_t*)&numberOfDigits,
                    sizeof(numberOfDigits));
    mk82FsCommitWrite(MK82_FS_FILE_ID_OTP_DATA);
}

void otpHalGetOptions(uint8_t* numberOfDigits)
{
    if (numberOfDigits == NULL)
    {
        otpHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_OTP_DATA, offsetof(OTP_HAL_NVM_DATA, numberOfDigits), (uint8_t*)numberOfDigits,
                   sizeof(uint8_t));

    if ((*numberOfDigits < OTP_GLOBAL_MIN_DIGITS) || (*numberOfDigits > OTP_GLOBAL_MAX_DIGITS))
    {
        otpHalFatalError();
    }
}

void otpHalIsKeyInitialized(uint16_t* result)
{
    if (result == NULL)
    {
        otpHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_OTP_KEYS, offsetof(OTP_HAL_NVM_KEYS, keyInitialized), (uint8_t*)result,
                   sizeof(uint16_t));
}

void otpHalGetKey(uint8_t* key, uint32_t* keyLength)
{
    uint8_t encryptedKey[OTP_GLOBAL_MAX_KEYLENGTH];
    uint8_t decryptedKey[OTP_GLOBAL_MAX_KEYLENGTH];
    uint8_t nonce[MK82_KEYSAFE_NONCE_LENGTH];
    uint8_t tag[MK82_KEYSAFE_TAG_LENGTH];
    uint16_t calleeRetVal;

    uint16_t keyInitialized = OTP_FALSE;

    if ((key == NULL) || (keyLength == NULL))
    {
        otpHalFatalError();
    }

    otpHalIsKeyInitialized(&keyInitialized);

    if (keyInitialized != OTP_TRUE)
    {
        otpHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_OTP_KEYS, offsetof(OTP_HAL_NVM_KEYS, key), encryptedKey, sizeof(encryptedKey));
    mk82FsReadFile(MK82_FS_FILE_ID_OTP_KEYS, offsetof(OTP_HAL_NVM_KEYS, keyNonce), nonce, sizeof(nonce));
    mk82FsReadFile(MK82_FS_FILE_ID_OTP_KEYS, offsetof(OTP_HAL_NVM_KEYS, keyTag), tag, sizeof(tag));
    mk82FsReadFile(MK82_FS_FILE_ID_OTP_KEYS, offsetof(OTP_HAL_NVM_KEYS, keyLength), (uint8_t*)keyLength,
                   sizeof(uint32_t));

    calleeRetVal = mk82KeysafeUnwrapKey(MK82_KEYSAFE_OTP_KEK_ID, encryptedKey, sizeof(encryptedKey), decryptedKey, NULL,
                                        0, nonce, tag);

    if (calleeRetVal != MK82_NO_ERROR)
    {
        otpHalFatalError();
    }

    if ((*keyLength < OTP_GLOBAL_MIN_KEYLENGTH) || (*keyLength > OTP_GLOBAL_MAX_KEYLENGTH))
    {
        otpHalFatalError();
    }

    mk82SystemMemCpy(key, decryptedKey, *keyLength);
}

void otpHalGetType(uint16_t* type)
{
    if (type == NULL)
    {
        otpHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_OTP_DATA, offsetof(OTP_HAL_NVM_DATA, type), (uint8_t*)type, sizeof(uint16_t));
}

void otpHalGetCounter(uint64_t* counter)
{
    if (counter == NULL)
    {
        otpHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_OTP_COUNTERS, offsetof(OTP_HAL_NVM_COUNTERS, counter), (uint8_t*)counter,
                   sizeof(uint64_t));
}

void otpHalSetCounter(uint64_t counter)
{
    mk82FsWriteFile(MK82_FS_FILE_ID_OTP_COUNTERS, offsetof(OTP_HAL_NVM_COUNTERS, counter), (uint8_t*)&counter,
                    sizeof(counter));
    mk82FsCommitWrite(MK82_FS_FILE_ID_OTP_COUNTERS);
}

void otpHalComputeHmac(uint8_t* key, uint32_t keyLength, uint8_t* counter, uint8_t* hmac)
{
    int calleeRetVal;

    calleeRetVal = mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), key, keyLength, counter,
                                   OTP_GLOBAL_COUNTER_LENGTH, hmac);

    if (calleeRetVal != 0)
    {
        otpHalFatalError();
    }
}

void otpHalSetExternalTime(uint32_t externalTime)
{
    otpHalExternalReferenceTimeInSec = externalTime;

    mk82SystemTickerGetMsPassed(&otpHalInternalReferenceTimeInMs);

    otpHalReferenceTimeSet = OTP_TRUE;
}

uint16_t otpHalGetCurrentTime(uint32_t* time)
{
    uint16_t retVal = OTP_GENERAL_ERROR;
    uint64_t currentInternalTimeInMs;
    uint32_t secondsPassed;

    if (time == NULL)
    {
        otpHalFatalError();
    }

    if (otpHalReferenceTimeSet != OTP_TRUE)
    {
        retVal = OTP_TIME_NOT_SET_ERROR;
        goto END;
    }

    mk82SystemTickerGetMsPassed(&currentInternalTimeInMs);

    if (currentInternalTimeInMs < otpHalInternalReferenceTimeInMs)
    {
        otpHalFatalError();
    }

    secondsPassed = (currentInternalTimeInMs - otpHalInternalReferenceTimeInMs) / 1000;

    *time = otpHalExternalReferenceTimeInSec + secondsPassed;

    retVal = OTP_NO_ERROR;

END:
    return retVal;
}

void otpHalWipeout(void)
{
    uint32_t bytesWritten = 0;
    uint8_t wipeoutBuffer[OTP_HAL_WIPEOUT_BUFFER_SIZE];
    uint16_t trueOrFalse;
    OTP_HAL_NVM_COUNTERS counters = {0};
    OTP_HAL_NVM_DATA data = {OTP_GLOBAL_MIN_DIGITS, OTP_GLOBAL_TYPE_HOTP};

    mk82SystemMemSet(wipeoutBuffer, 0x00, sizeof(wipeoutBuffer));

    while ((bytesWritten + sizeof(wipeoutBuffer)) < sizeof(OTP_HAL_NVM_KEYS))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_OTP_KEYS, bytesWritten, wipeoutBuffer, sizeof(wipeoutBuffer));
        bytesWritten += sizeof(wipeoutBuffer);
    }

    if (bytesWritten < sizeof(OTP_HAL_NVM_KEYS))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_OTP_KEYS, bytesWritten, wipeoutBuffer,
                        (sizeof(OTP_HAL_NVM_KEYS) - bytesWritten));
    }

    trueOrFalse = OTP_FALSE;
    mk82FsWriteFile(MK82_FS_FILE_ID_OTP_KEYS, offsetof(OTP_HAL_NVM_KEYS, keyInitialized), (uint8_t*)&trueOrFalse,
                    sizeof(trueOrFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_OTP_KEYS);

    mk82FsWriteFile(MK82_FS_FILE_ID_OTP_DATA, 0, (uint8_t*)&data, sizeof(data));
    mk82FsCommitWrite(MK82_FS_FILE_ID_OTP_DATA);

    mk82FsWriteFile(MK82_FS_FILE_ID_OTP_COUNTERS, 0, (uint8_t*)&counters, sizeof(counters));
    mk82FsCommitWrite(MK82_FS_FILE_ID_OTP_COUNTERS);
}

void otpHalFatalError(void) { mk82SystemFatalError(); }
