/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "MK82F25615.h"

#include <sfGlobal.h>
#include <sfGlobalInt.h>
#include <sfHal.h>
#include <hal/k82/sfHalInt.h>
#include <hal/sfHalAttestationCertificate.h>

#include <mk82Global.h>
#include <mk82System.h>
#ifdef USE_BUTTON
#include <mk82Button.h>
#endif
#ifdef USE_TOUCH
#include <mk82Touch.h>
#endif
#include <mk82Fs.h>
#include <mk82KeySafe.h>

#include <fsl_pit.h>

#include "mbedtls/ecdsa.h"
#include "mbedtls/ecp.h"
#include "mbedtls/sha256.h"

static uint16_t shHalUserPresent = SF_FALSE;

static void shHalButtonPressed(void);
static void sfHalSignData(uint8_t* privateKey, uint8_t* arraysToHash[], uint32_t arrayLengths[], uint32_t listLength,
                          uint8_t* signature, uint16_t* signatureLength);

static void sfHalSignData(uint8_t* privateKey, uint8_t* arraysToHash[], uint32_t arrayLengths[], uint32_t listLength,
                          uint8_t* signature, uint16_t* signatureLength)
{
    int tlsCalleeRetVal;
    mbedtls_ecdsa_context ecsdaContext;
    uint8_t hash[SF_HAL_SHA256_LENGTH];
    mbedtls_sha256_context shaContext;
    uint8_t signatureInternal[MBEDTLS_ECDSA_MAX_LEN];
    uint32_t signatureLengthInternal;
    uint32_t i;

    mbedtls_ecdsa_init(&ecsdaContext);

    tlsCalleeRetVal = mbedtls_ecp_group_load(&ecsdaContext.grp, MBEDTLS_ECP_DP_SECP256R1);

    if (tlsCalleeRetVal != 0)
    {
        sfHalFatalError();
    }

    tlsCalleeRetVal = mbedtls_mpi_read_binary(&ecsdaContext.d, privateKey, SF_GLOBAL_PRIVATE_KEY_LENGTH);

    if (tlsCalleeRetVal != 0)
    {
        sfHalFatalError();
    }

    mbedtls_sha256_init(&shaContext);
    mbedtls_sha256_starts(&shaContext, false);

    for (i = 0; i < listLength; i++)
    {
        mbedtls_sha256_update(&shaContext, arraysToHash[i], arrayLengths[i]);
    }

    mbedtls_sha256_finish(&shaContext, hash);
    mbedtls_sha256_free(&shaContext);

    tlsCalleeRetVal =
        mbedtls_ecdsa_write_signature(&ecsdaContext, MBEDTLS_MD_SHA256, hash, sizeof(hash), signatureInternal,
                                      (size_t*)&signatureLengthInternal, mk82SystemGetRandomForTLS, NULL, 0);

    if (tlsCalleeRetVal != 0)
    {
        sfHalFatalError();
    }

    if (signatureLengthInternal > SF_GLOBAL_MAXIMAL_SIGNATURE_LENGTH)
    {
        sfHalFatalError();
    }

    *signatureLength = (uint16_t)signatureLengthInternal;
    mk82SystemMemCpy(signature, signatureInternal, signatureLengthInternal);

    mbedtls_ecdsa_free(&ecsdaContext);
}

void PIT2_IRQHandler(void)
{
    PIT_StopTimer(PIT, kPIT_Chnl_2);
    PIT_ClearStatusFlags(PIT0, kPIT_Chnl_2, PIT_TFLG_TIF_MASK);

    shHalUserPresent = SF_FALSE;
}

static void shHalButtonPressed(void)
{
    shHalUserPresent = SF_TRUE;

    PIT_StopTimer(PIT, kPIT_Chnl_2);
    PIT_ClearStatusFlags(PIT0, kPIT_Chnl_2, PIT_TFLG_TIF_MASK);
    PIT_StartTimer(PIT, kPIT_Chnl_2);
}

void sfhalInit(void)
{
    PIT_SetTimerPeriod(PIT0, kPIT_Chnl_2,
                       MSEC_TO_COUNT(SF_HAL_USER_PRESENCE_TIMEOUT_IN_MS, CLOCK_GetFreq(kCLOCK_BusClk)));
    PIT_EnableInterrupts(PIT0, kPIT_Chnl_2, kPIT_TimerInterruptEnable);
    NVIC_EnableIRQ(PIT2_IRQn);

#ifdef USE_BUTTON
    mk82ButtonRegisterButtonClickedCallback(shHalButtonPressed);
#endif

#ifdef USE_TOUCH
    mk82TouchRegisterButton1PressedCallback(shHalButtonPressed);
#endif
}

void sfHalDeinit() {}

void sfhalCheckKeyPresence(uint8_t* keyHandle, uint8_t keyLength, uint8_t* applicationId, uint16_t* keyFound)
{
    uint16_t calleeRetVal;
    uint8_t privateKey[SF_GLOBAL_PRIVATE_KEY_LENGTH];

    if ((keyHandle == NULL) || (applicationId == NULL) || (keyFound == NULL))
    {
        sfHalFatalError();
    }

    if (keyLength != SF_GLOBAL_KEY_HANDLE_LENGTH)
    {
        *keyFound = SF_FALSE;
    }
    else
    {
        calleeRetVal = mk82KeysafeUnwrapKey(MK82_KEYSAFE_SF_KEK_ID, keyHandle, SF_GLOBAL_PRIVATE_KEY_LENGTH, privateKey,
                                            applicationId, SF_GLOBAL_APPLICATION_ID_LENGTH,
                                            (keyHandle + SF_GLOBAL_PRIVATE_KEY_LENGTH),
                                            (keyHandle + SF_GLOBAL_PRIVATE_KEY_LENGTH + MK82_KEYSAFE_NONCE_LENGTH));

        mk82SystemMemSet(privateKey, 0x00, sizeof(privateKey));

        if (calleeRetVal != MK82_NO_ERROR)
        {
            if (calleeRetVal == MK82_WRAPPED_KEY_CORRUPTED_ERROR)
            {
                *keyFound = SF_FALSE;
            }
            else
            {
                sfHalFatalError();
            }
        }
        else
        {
            *keyFound = SF_TRUE;
        }
    }
}

void sfHalComputeAuthenticationSignature(uint8_t* keyHandle, uint8_t* applicationId, uint8_t userPresenceByte,
                                         uint8_t* counter, uint8_t* challenge, uint8_t* signature,
                                         uint16_t* signatureLength)
{
    uint16_t calleeRetVal;
    uint8_t privateKey[SF_GLOBAL_PRIVATE_KEY_LENGTH];
    uint8_t* dataToHashList[] = {applicationId, &userPresenceByte, counter, challenge};
    uint32_t dataToHashLengthsList[] = {SF_GLOBAL_APPLICATION_ID_LENGTH, sizeof(uint8_t), sizeof(uint32_t),
                                        SF_GLOBAL_CHALLENGE_LENGTH};
    uint32_t listLength = sizeof(dataToHashLengthsList) / sizeof(dataToHashLengthsList[1]);

    if ((keyHandle == NULL) || (applicationId == NULL) || (counter == NULL) || (challenge == NULL) ||
        (signature == NULL) || (signatureLength == NULL))
    {
        sfHalFatalError();
    }

    calleeRetVal =
        mk82KeysafeUnwrapKey(MK82_KEYSAFE_SF_KEK_ID, keyHandle, SF_GLOBAL_PRIVATE_KEY_LENGTH, privateKey, applicationId,
                             SF_GLOBAL_APPLICATION_ID_LENGTH, (keyHandle + SF_GLOBAL_PRIVATE_KEY_LENGTH),
                             (keyHandle + SF_GLOBAL_PRIVATE_KEY_LENGTH + MK82_KEYSAFE_NONCE_LENGTH));

    if (calleeRetVal != MK82_NO_ERROR)
    {
        sfHalFatalError();
    }

    sfHalSignData(privateKey, dataToHashList, dataToHashLengthsList, listLength, signature, signatureLength);

    mk82SystemMemSet(privateKey, 0x00, sizeof(privateKey));
}

void sfHalComputeRegistrationSignature(uint8_t hashID, uint8_t* applicationId, uint8_t* challenge, uint8_t* keyHandle,
                                       uint8_t* publicKey, uint8_t* signature, uint16_t* signatureLength)
{
    uint8_t privateKey[SF_GLOBAL_PRIVATE_KEY_LENGTH];
    uint8_t* dataToHashList[] = {&hashID, applicationId, challenge, keyHandle, publicKey};
    uint32_t dataToHashLengthsList[] = {sizeof(uint8_t), SF_GLOBAL_APPLICATION_ID_LENGTH, SF_GLOBAL_CHALLENGE_LENGTH,
                                        SF_GLOBAL_KEY_HANDLE_LENGTH, SF_GLOBAL_PUBLIC_KEY_LENGTH};
    uint32_t listLength = sizeof(dataToHashLengthsList) / sizeof(dataToHashLengthsList[1]);

    if ((applicationId == NULL) || (challenge == NULL) || (keyHandle == NULL) || (publicKey == NULL) ||
        (signature == NULL) || (signatureLength == NULL))
    {
        sfHalFatalError();
    }

    mk82KeysafeGetSfAttestationKey(privateKey);

    sfHalSignData(privateKey, dataToHashList, dataToHashLengthsList, listLength, signature, signatureLength);

    mk82SystemMemSet(privateKey, 0x00, sizeof(privateKey));
}

void sfHalGenerateKeyPair(uint8_t* publicKey, uint8_t* applicationId, uint8_t* keyHandle)
{
    int calleeRetVal;
    mbedtls_ecdsa_context ecsdaContext;
    uint8_t privateKey[SF_GLOBAL_PRIVATE_KEY_LENGTH];
    uint32_t pointLength;
    uint32_t i;

    if ((publicKey == NULL) || (applicationId == NULL) || (keyHandle == NULL))
    {
        sfHalFatalError();
    }

    mbedtls_ecdsa_init(&ecsdaContext);

    calleeRetVal = mbedtls_ecdsa_genkey(&ecsdaContext, MBEDTLS_ECP_DP_SECP256R1, mk82SystemGetRandomForTLS, NULL);

    if (calleeRetVal != 0)
    {
        sfHalFatalError();
    }

    calleeRetVal = mbedtls_mpi_write_binary(&ecsdaContext.d, privateKey, SF_GLOBAL_PRIVATE_KEY_LENGTH);

    if (calleeRetVal != 0)
    {
        sfHalFatalError();
    }

    mk82KeysafeWrapKey(MK82_KEYSAFE_SF_KEK_ID, privateKey, SF_GLOBAL_PRIVATE_KEY_LENGTH, keyHandle, applicationId,
                       SF_GLOBAL_APPLICATION_ID_LENGTH, (keyHandle + SF_GLOBAL_PRIVATE_KEY_LENGTH),
                       (keyHandle + SF_GLOBAL_PRIVATE_KEY_LENGTH + MK82_KEYSAFE_NONCE_LENGTH));

    mk82SystemMemSet(privateKey, 0x00, sizeof(privateKey));

    calleeRetVal = mbedtls_ecp_point_write_binary(&ecsdaContext.grp, &ecsdaContext.Q, MBEDTLS_ECP_PF_UNCOMPRESSED,
                                                  (size_t*)&pointLength, publicKey, SF_GLOBAL_PUBLIC_KEY_LENGTH);

    if (calleeRetVal != 0)
    {
        sfHalFatalError();
    }

    if (pointLength != SF_GLOBAL_PUBLIC_KEY_LENGTH)
    {
        sfHalFatalError();
    }

    for (i = 0; i < SF_GLOBAL_KEY_HANDLE_PADDING_LENGTH; i++)
    {
        keyHandle[SF_GLOBAL_PRIVATE_KEY_LENGTH + MK82_KEYSAFE_NONCE_LENGTH + MK82_KEYSAFE_TAG_LENGTH + i] = 0x00;
    }

    mbedtls_ecdsa_free(&ecsdaContext);
}

void sfhalCheckUserPresence(uint16_t* userPresent)
{
    if (userPresent == NULL)
    {
        sfHalFatalError();
    }

    *userPresent = shHalUserPresent;
}

void sfHalDiscardUserPresence()
{
    PIT_StopTimer(PIT, kPIT_Chnl_2);
    PIT_ClearStatusFlags(PIT0, kPIT_Chnl_2, PIT_TFLG_TIF_MASK);

    shHalUserPresent = SF_FALSE;
}

void sfHalGetAndIncrementACounter(uint32_t* counterValue)
{
    uint32_t counterValueInternal;

    if (counterValue == NULL)
    {
        sfHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_SF_COUNTERS, offsetof(SF_HAL_NVM_COUNTERS, signatureCounter),
                   (uint8_t*)&counterValueInternal, sizeof(uint32_t));

    *counterValue = counterValueInternal;

    counterValueInternal++;

    mk82FsWriteFile(MK82_FS_FILE_ID_SF_COUNTERS, offsetof(SF_HAL_NVM_COUNTERS, signatureCounter),
                    (uint8_t*)&counterValueInternal, sizeof(uint32_t));
    mk82FsCommitWrite(MK82_FS_FILE_ID_SF_COUNTERS);
}

void sfHalGetAttestationCertificate(uint8_t* certificate, uint16_t* certificateLength)
{
    if ((certificate == NULL) || (certificateLength == NULL))
    {
        sfHalFatalError();
    }

    *certificateLength = (uint16_t)sizeof(sfHalAttestationCertificate);

    sfHalMemCpy(certificate, (uint8_t*)sfHalAttestationCertificate, (uint16_t)sizeof(sfHalAttestationCertificate));
}

void sfHalMemCpy(uint8_t* dst, uint8_t* src, uint16_t length) { mk82SystemMemCpy(dst, src, length); }

void sfHalMemSet(uint8_t* dst, uint8_t value, uint16_t length) { mk82SystemMemSet(dst, value, length); }

uint16_t sfHalMemCmp(uint8_t* array1, uint8_t* array2, uint16_t length)
{
    uint16_t comparisonResult;

    comparisonResult = mk82SystemMemCmp(array1, array2, length);

    if (comparisonResult != MK82_TRUE)
    {
        return SF_CMP_NOT_EQUAL;
    }
    else
    {
        return SF_CMP_EQUAL;
    }
}

void sfHalGenerateNonSecureRandom(uint8_t* data, uint16_t length)
{
    if (data == NULL)
    {
        sfHalFatalError();
    }

    mk82SystemGetRandom(data, length);
}

void sfHalWipeout(void)
{
    SF_HAL_NVM_COUNTERS counters = {0};

    mk82FsWriteFile(MK82_FS_FILE_ID_SF_COUNTERS, 0, (uint8_t*)&counters, sizeof(counters));
    mk82FsCommitWrite(MK82_FS_FILE_ID_SF_COUNTERS);
}

void sfHalFatalError(void) { mk82SystemFatalError(); }
