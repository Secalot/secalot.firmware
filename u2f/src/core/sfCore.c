/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sfGlobal.h>
#include <sfGlobalInt.h>
#include <sfCore.h>
#include <core/sfCoreInt.h>
#include <sfHal.h>

#ifdef MEW_HACK
#include "ethGlobal.h"
#include "ethCore.h"
#endif /* MEW_HACK */

#ifdef XRP_HACK
#include "xrpGlobal.h"
#include "xrpCore.h"
#endif /* XRP_HACK */

static uint16_t systemInitialized = SF_FALSE;

void sfCoreInit()
{
    sfhalInit();

    systemInitialized = SF_TRUE;

    return;
}

void sfCoreDeinit() { sfHalDeinit(); }

void sfCoreProcessAPDU(uint8_t* apduBuffer, uint32_t* apduBufferLength)
{
    uint16_t sw = SF_CORE_SW_GENERAL_ERROR;
    SF_CORE_COMMAND_APDU* commandAPDU = (SF_CORE_COMMAND_APDU*)apduBuffer;

    if (systemInitialized != SF_TRUE)
    {
        sw = SF_CORE_SW_GENERAL_ERROR;
        goto END;
    }

    if (*apduBufferLength < SF_CORE_MINIMUM_COMMAND_APDU_LENGTH)
    {
        sw = SF_CORE_SW_INVALID_LENGTH;
        goto END;
    }

    if (commandAPDU->cla != SF_CORE_DEFAULT_CLA_VALUE)
    {
        sw = SF_CORE_SW_CLA_NOT_SUPPORTED;
        goto END;
    }

    switch (commandAPDU->ins)
    {
        case SF_CORE_INS_REGISTER:
        {
            sw = sfCoreProcessRegistration(apduBuffer, apduBufferLength);
        }
        break;
        case SF_CORE_INS_AUTHENTICATE:
        {
            sw = sfCoreProcessAuthentication(apduBuffer, apduBufferLength);
        }
        break;
        case SF_CORE_INS_GET_VERSION:
        {
            sw = sfCoreGetVersion(apduBuffer, apduBufferLength);
        }
        break;
        default:
            sw = SF_CORE_SW_INS_NOT_SUPPORTED;
            goto END;
    }

END:
    if (sw != SF_CORE_SW_NO_ERROR)
    {
        *apduBufferLength = 0;
    }

    apduBuffer[*apduBufferLength] = SF_HIBYTE(sw);
    apduBuffer[*apduBufferLength + 1] = SF_LOBYTE(sw);
    *apduBufferLength += 2;

    return;
}

// TODO: delete keypair if an error occurs later.

static uint16_t sfCoreProcessRegistration(uint8_t* apduBuffer, uint32_t* apduBufferLength)
{
    uint16_t sw = SF_CORE_SW_GENERAL_ERROR;
    SF_CORE_COMMAND_APDU* commandAPDU = (SF_CORE_COMMAND_APDU*)apduBuffer;
    SF_CORE_REGISTER_REQ* registerRequest = (SF_CORE_REGISTER_REQ*)commandAPDU->data;
    SF_CORE_REGISTER_RESP* registerResponse = (SF_CORE_REGISTER_RESP*)apduBuffer;
    uint8_t applicationId[SF_GLOBAL_APPLICATION_ID_LENGTH];
    uint8_t challenge[SF_GLOBAL_CHALLENGE_LENGTH];
    uint16_t certificateLength;
    uint16_t signatureLength;
    uint32_t lc = SF_MAKEDWORD(SF_MAKEWORD(commandAPDU->lc3, commandAPDU->lc2), SF_MAKEWORD(commandAPDU->lc1, 0x00));
    uint16_t userPresent = SF_FALSE;

    // P1 and P2 are unspecified.

    if (lc != (SF_GLOBAL_CHALLENGE_LENGTH + SF_GLOBAL_APPLICATION_ID_LENGTH))
    {
        sw = SF_CORE_SW_INVALID_LENGTH;
        goto END;
    }

    sfhalCheckUserPresence(&userPresent);

    if (userPresent != SF_TRUE)
    {
        sw = SF_CORE_SW_CONDITIONS_NOT_SATISFIED;
        goto END;
    }

    sfHalMemCpy(applicationId, registerRequest->applicationId, sizeof(applicationId));

    sfHalMemCpy(challenge, registerRequest->challenge, sizeof(challenge));

    sfHalGenerateKeyPair(registerResponse->publicKey, applicationId, registerResponse->keyHandle);

    sfHalGetAttestationCertificate(registerResponse->certificateAndSignature, &certificateLength);

    sfHalComputeRegistrationSignature(SF_CORE_REGISTER_HASH_ID, applicationId, challenge, registerResponse->keyHandle,
                                      registerResponse->publicKey,
                                      &registerResponse->certificateAndSignature[certificateLength], &signatureLength);

    registerResponse->registerId = SF_CORE_REGISTER_ID;
    registerResponse->keyHandleLength = SF_GLOBAL_KEY_HANDLE_LENGTH;

    *apduBufferLength =
        SF_CORE_REGISTER_RESPONSE_LENGTH_WITOUT_CERTIFICATE_AND_SIGNATURE + certificateLength + signatureLength;

    sfHalDiscardUserPresence();

    sw = SF_CORE_SW_NO_ERROR;

END:

    if (sw != SF_CORE_SW_NO_ERROR)
    {
        *apduBufferLength = 0;
    }

    return sw;
}

static uint16_t sfCoreProcessAuthentication(uint8_t* apduBuffer, uint32_t* apduBufferLength)
{
    uint16_t sw = SF_CORE_SW_GENERAL_ERROR;
    uint32_t counterValue;
    uint16_t signatureLength;
    SF_CORE_COMMAND_APDU* commandAPDU = (SF_CORE_COMMAND_APDU*)apduBuffer;
    SF_CORE_AUTHENTICATE_REQ* authenticateRequest = (SF_CORE_AUTHENTICATE_REQ*)commandAPDU->data;
    SF_CORE_AUTHENTICATE_RESP* authenticateResponse = (SF_CORE_AUTHENTICATE_RESP*)apduBuffer;
    uint32_t lc = SF_MAKEDWORD(SF_MAKEWORD(commandAPDU->lc3, commandAPDU->lc2), SF_MAKEWORD(commandAPDU->lc1, 0x00));
    uint16_t userPresent = SF_FALSE;
    uint16_t keyFound = SF_FALSE;

    if (lc != (SF_GLOBAL_APPLICATION_ID_LENGTH + SF_GLOBAL_CHALLENGE_LENGTH + SF_CORE_KEY_HANDLE_LENGTH_LENGTH +
               authenticateRequest->keyHandleLength))
    {
        sw = SF_CORE_SW_INVALID_LENGTH;
        goto END;
    }

#ifdef MEW_HACK
    {
        uint8_t magicString[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

        if (authenticateRequest->keyHandleLength > sizeof(magicString))
        {
            if (sfHalMemCmp(authenticateRequest->keyHandle, magicString, sizeof(magicString)) == SF_CMP_EQUAL)
            {
                if (commandAPDU->p1 == SF_CORE_AUTHENTICATION_CHECK_ONLY)
                {
                    sw = SF_CORE_SW_CONDITIONS_NOT_SATISFIED;
                    goto END;
                }
                else if (commandAPDU->p1 == SF_CORE_ENFORCE_AUTHENTICATION)
                {
                    uint32_t ethApduLength = authenticateRequest->keyHandleLength - sizeof(magicString);

                    sfHalMemCpy(apduBuffer, authenticateRequest->keyHandle + sizeof(magicString), ethApduLength);

                    ethCoreProcessAPDU(apduBuffer, &ethApduLength);

                    sfHalMemCpy(apduBuffer + 5, apduBuffer, ethApduLength);

                    apduBuffer[0] = SF_CORE_AUTHENTICATION_FLAG_TUP;
                    apduBuffer[1] = 0x00;
                    apduBuffer[2] = 0x00;
                    apduBuffer[3] = 0x00;
                    apduBuffer[4] = 0x00;

                    ethApduLength += 5;

                    *apduBufferLength = ethApduLength;

                    sw = SF_CORE_SW_NO_ERROR;

                    goto END;
                }
                else
                {
                    sw = SF_CORE_SW_GENERAL_ERROR;
                    goto END;
                }
            }
        }
    }
#endif /* MEW_HACK */

#ifdef XRP_HACK
    {
        uint8_t magicString[] = {0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};

        if (authenticateRequest->keyHandleLength > sizeof(magicString))
        {
            if (sfHalMemCmp(authenticateRequest->keyHandle, magicString, sizeof(magicString)) == SF_CMP_EQUAL)
            {
                if (commandAPDU->p1 == SF_CORE_AUTHENTICATION_CHECK_ONLY)
                {
                    sw = SF_CORE_SW_CONDITIONS_NOT_SATISFIED;
                    goto END;
                }
                else if (commandAPDU->p1 == SF_CORE_ENFORCE_AUTHENTICATION)
                {
                    uint32_t xrpApduLength = authenticateRequest->keyHandleLength - sizeof(magicString);

                    sfHalMemCpy(apduBuffer, authenticateRequest->keyHandle + sizeof(magicString), xrpApduLength);

                    xrpCoreProcessAPDU(apduBuffer, &xrpApduLength);

                    sfHalMemCpy(apduBuffer + 5, apduBuffer, xrpApduLength);

                    apduBuffer[0] = SF_CORE_AUTHENTICATION_FLAG_TUP;
                    apduBuffer[1] = 0x00;
                    apduBuffer[2] = 0x00;
                    apduBuffer[3] = 0x00;
                    apduBuffer[4] = 0x00;

                    xrpApduLength += 5;

                    *apduBufferLength = xrpApduLength;

                    sw = SF_CORE_SW_NO_ERROR;

                    goto END;
                }
                else
                {
                    sw = SF_CORE_SW_GENERAL_ERROR;
                    goto END;
                }
            }
        }
    }
#endif /* XRP_HACK */

    // P2 is unspecified.

    if (commandAPDU->p1 == SF_CORE_AUTHENTICATION_CHECK_ONLY)
    {
        sfhalCheckKeyPresence(authenticateRequest->keyHandle, authenticateRequest->keyHandleLength,
                              authenticateRequest->applicationId, &keyFound);

        if (keyFound != SF_TRUE)
        {
            sw = SF_CORE_SW_WRONG_DATA;
            goto END;
        }

        sw = SF_CORE_SW_CONDITIONS_NOT_SATISFIED;
        goto END;
    }
    else if (commandAPDU->p1 == SF_CORE_ENFORCE_AUTHENTICATION)
    {
        if (authenticateRequest->keyHandleLength != SF_GLOBAL_KEY_HANDLE_LENGTH)
        {
            sw = SF_CORE_SW_GENERAL_ERROR;
            goto END;
        }

        sfhalCheckKeyPresence(authenticateRequest->keyHandle, authenticateRequest->keyHandleLength,
                              authenticateRequest->applicationId, &keyFound);

        if (keyFound != SF_TRUE)
        {
            sw = SF_CORE_SW_WRONG_DATA;
            goto END;
        }

        sfhalCheckUserPresence(&userPresent);

        if (userPresent != SF_TRUE)
        {
            sw = SF_CORE_SW_CONDITIONS_NOT_SATISFIED;
            goto END;
        }

        sfHalGetAndIncrementACounter(&counterValue);

        authenticateResponse->flags = SF_CORE_AUTHENTICATION_FLAG_TUP;
        authenticateResponse->counter[0] = SF_HIBYTE(SF_HIWORD(counterValue));
        authenticateResponse->counter[1] = SF_LOBYTE(SF_HIWORD(counterValue));
        authenticateResponse->counter[2] = SF_HIBYTE(SF_LOWORD(counterValue));
        authenticateResponse->counter[3] = SF_LOBYTE(SF_LOWORD(counterValue));

        sfHalComputeAuthenticationSignature(authenticateRequest->keyHandle, authenticateRequest->applicationId,
                                            SF_CORE_AUTHENTICATION_FLAG_TUP, authenticateResponse->counter,
                                            authenticateRequest->challenge, authenticateResponse->signature,
                                            &signatureLength);

        *apduBufferLength = SF_CORE_AUTHENTIATE_RESPONSE_LENGTH_WITHOUT_SIGNATURE + signatureLength;

        sfHalDiscardUserPresence();

        sw = SF_CORE_SW_NO_ERROR;
    }
    else
    {
        sw = SF_CORE_SW_GENERAL_ERROR;
        goto END;
    }

END:

    if (sw != SF_CORE_SW_NO_ERROR)
    {
        *apduBufferLength = 0;
    }

    return sw;
}

static uint16_t sfCoreGetVersion(uint8_t* apduBuffer, uint32_t* apduBufferLength)
{
    uint16_t sw = SF_CORE_SW_GENERAL_ERROR;
    SF_CORE_COMMAND_APDU* commandAPDU = (SF_CORE_COMMAND_APDU*)apduBuffer;
    SF_CORE_GET_VERSION_RESP* getVersionResponse = (SF_CORE_GET_VERSION_RESP*)apduBuffer;
    uint32_t lc = SF_MAKEDWORD(SF_MAKEWORD(commandAPDU->lc3, commandAPDU->lc2), SF_MAKEWORD(commandAPDU->lc1, 0x00));

    // P1 and P2 are unspecified.

    if (lc != SF_CORE_GET_VERSION_REQUEST_DATA_LENGTH)
    {
        sw = SF_CORE_SW_INVALID_LENGTH;
        goto END;
    }

    sfHalMemCpy(getVersionResponse->version, SF_CORE_GET_VERSION_VERSION_STRING,
                SF_CORE_GET_VERSION_VERSION_STRING_LENGTH);

    *apduBufferLength = SF_CORE_GET_VERSION_VERSION_STRING_LENGTH;

    sw = SF_CORE_SW_NO_ERROR;

END:

    if (sw != SF_CORE_SW_NO_ERROR)
    {
        *apduBufferLength = 0;
    }

    return sw;
}
