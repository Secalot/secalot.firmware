/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdint.h>

#include "opgpGlobal.h"
#include "opgpGlobalInt.h"
#include "opgpCore.h"
#include "opgpCoreInt.h"
#include "opgpHal.h"
#include "opgpPin.h"

#include "apduGlobal.h"
#include "apduCore.h"

static void opgpCoreProcessGetData(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void opgpCoreProcessVerify(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void opgpCoreProcessChangeReferenceData(APDU_CORE_COMMAND_APDU* commandAPDU,
                                               APDU_CORE_RESPONSE_APDU* responseAPDU);
static void opgpCoreProcessResetRetryCounter(APDU_CORE_COMMAND_APDU* commandAPDU,
                                             APDU_CORE_RESPONSE_APDU* responseAPDU);
static void opgpCoreProcessPutData(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void opgpCoreProcessPutDataImportKey(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void opgpCoreProcessGenKeyPair(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void opgpCoreProcessPSO(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void opgpCoreProcessInternalAuthenticate(APDU_CORE_COMMAND_APDU* commandAPDU,
                                                APDU_CORE_RESPONSE_APDU* responseAPDU);
static void opgpCoreProcessGetChallenge(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void opgpCoreProcessTerminateDF(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void opgpCoreProcessActivateFile(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);

static void opgpCoreSetPublicKeyDO(uint16_t keyType, uint8_t* buffer);

void opgpCoreInit(void)
{
    opgpPinInit();
    opgpHalInit();
}

static void opgpCoreProcessGetData(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;

    responseAPDU->dataLength = 0x00;

#ifndef OPGP_CORE_IGNORE_LE
    if ((commandAPDU->lePresent == APDU_FALSE) || (commandAPDU->le != 0x00))
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }
#endif /* OPGP_CORE_IGNORE_LE */

    if (commandAPDU->lcPresent == APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    switch (commandAPDU->p1p2)
    {
        case OPGP_GLOBAL_TAG_PRIVATE_USE_1:
        {
            opgpHalGetDataObjectWithLength(OPGP_GLOBAL_TAG_PRIVATE_USE_1, responseAPDU->data,
                                           &responseAPDU->dataLength);
        }
        break;
        case OPGP_GLOBAL_TAG_PRIVATE_USE_2:
        {
            opgpHalGetDataObjectWithLength(OPGP_GLOBAL_TAG_PRIVATE_USE_2, responseAPDU->data,
                                           &responseAPDU->dataLength);
        }
        break;
        case OPGP_GLOBAL_TAG_PRIVATE_USE_3:
        {
            if (opgpPinIsPW1_82_Verified() != OPGP_TRUE)
            {
                sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
                goto END;
            }

            opgpHalGetDataObjectWithLength(OPGP_GLOBAL_TAG_PRIVATE_USE_3, responseAPDU->data,
                                           &responseAPDU->dataLength);
        }
        break;
        case OPGP_GLOBAL_TAG_PRIVATE_USE_4:
        {
            if (opgpPinIsPW3Verified() != OPGP_TRUE)
            {
                sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
                goto END;
            }

            opgpHalGetDataObjectWithLength(OPGP_GLOBAL_TAG_PRIVATE_USE_4, responseAPDU->data,
                                           &responseAPDU->dataLength);
        }
        break;
        case OPGP_GLOBAL_TAG_AID:
        {
            opgpCoreGetAID(responseAPDU->data, &responseAPDU->dataLength);
        }
        break;
        case OPGP_GLOBAL_TAG_LOGIN_DATA:
        {
            opgpHalGetDataObjectWithLength(OPGP_GLOBAL_TAG_LOGIN_DATA, responseAPDU->data, &responseAPDU->dataLength);
        }
        break;
        case OPGP_GLOBAL_TAG_URL:
        {
            opgpHalGetDataObjectWithLength(OPGP_GLOBAL_TAG_URL, responseAPDU->data, &responseAPDU->dataLength);
        }
        break;
        case OPGP_GLOBAL_TAG_HISTORICAL_BYTES:
        {
            opgpCoreGetHistChars(responseAPDU->data, &responseAPDU->dataLength);
        }
        break;
        case OPGP_GLOBAL_TAG_CARDHOLDER_RELATED_DATA:
        {
            uint32_t offset = 0;
            uint32_t currentTagLength;

            responseAPDU->data[offset++] = OPGP_GLOBAL_TAG_NAME;

            opgpHalGetDataObjectWithLength(OPGP_GLOBAL_TAG_NAME, &responseAPDU->data[offset + 1], &currentTagLength);

            responseAPDU->data[offset] = currentTagLength;

            offset += currentTagLength + 1;

            responseAPDU->data[offset++] = OPGP_HIBYTE(OPGP_GLOBAL_TAG_LANGUAGE_PREFERENCE);
            responseAPDU->data[offset++] = OPGP_LOBYTE(OPGP_GLOBAL_TAG_LANGUAGE_PREFERENCE);

            opgpHalGetDataObjectWithLength(OPGP_GLOBAL_TAG_LANGUAGE_PREFERENCE, &responseAPDU->data[offset + 1],
                                           &currentTagLength);

            responseAPDU->data[offset] = currentTagLength;

            offset += currentTagLength + 1;

            responseAPDU->data[offset++] = OPGP_HIBYTE(OPGP_GLOBAL_TAG_SEX);
            responseAPDU->data[offset++] = OPGP_LOBYTE(OPGP_GLOBAL_TAG_SEX);
            responseAPDU->data[offset++] = OPGP_GLOBAL_DO_SEX_LENGTH;

            opgpHalGetDataObject(OPGP_GLOBAL_TAG_SEX, &responseAPDU->data[offset++]);

            responseAPDU->dataLength = offset;
        }
        break;

        case OPGP_GLOBAL_TAG_APPLICATION_RELATED_DATA:
        {
            uint32_t offset = 0;
            uint32_t discretionaryDataObjectsPosition;
            uint8_t extendedCapabilities[] = OPGP_CORE_EXTENDED_CAPABILITIES;
            uint8_t algorithmAttributes[] = OPGP_CORE_ALGORITHM_ATTRIBUTES;
            uint32_t aidLength;
            uint32_t histCharsLength;

            responseAPDU->data[offset++] = OPGP_GLOBAL_TAG_AID;
            responseAPDU->data[offset++] = OPGP_CORE_AID_LENGTH;

            opgpCoreGetAID(&responseAPDU->data[offset], &aidLength);

            offset += aidLength;

            responseAPDU->data[offset++] = OPGP_HIBYTE(OPGP_GLOBAL_TAG_HISTORICAL_BYTES);
            responseAPDU->data[offset++] = OPGP_LOBYTE(OPGP_GLOBAL_TAG_HISTORICAL_BYTES);
            responseAPDU->data[offset++] = OPGP_CORE_HIST_CHARS_LENGTH;

            opgpCoreGetHistChars(&responseAPDU->data[offset], &histCharsLength);

            offset += histCharsLength;

            responseAPDU->data[offset++] = OPGP_GLOBAL_TAG_DISCRETIONARY_DATA_OBJECTS;
            responseAPDU->data[offset++] = 0x81;
            discretionaryDataObjectsPosition = offset;
            responseAPDU->data[offset++] = 0x00;

            responseAPDU->data[offset++] = OPGP_GLOBAL_TAG_EXTENDED_CAPABILITIES;
            responseAPDU->data[offset++] = OPGP_CORE_EXTENDED_CAPABILITIES_LENGTH;

            opgpHalMemCpy(&responseAPDU->data[offset], extendedCapabilities, OPGP_CORE_EXTENDED_CAPABILITIES_LENGTH);

            offset += OPGP_CORE_EXTENDED_CAPABILITIES_LENGTH;

            responseAPDU->data[offset++] = OPGP_GLOBAL_TAG_ALGORITHM_ATTRIBUTES_SIGNATURE;
            responseAPDU->data[offset++] = OPGP_CORE_ALGORITHM_ATTRIBUTES_LENGTH;

            opgpHalMemCpy(&responseAPDU->data[offset], algorithmAttributes, OPGP_CORE_ALGORITHM_ATTRIBUTES_LENGTH);

            offset += OPGP_CORE_ALGORITHM_ATTRIBUTES_LENGTH;

            responseAPDU->data[offset++] = OPGP_GLOBAL_TAG_ALGORITHM_ATTRIBUTES_DECRYPTION;
            responseAPDU->data[offset++] = OPGP_CORE_ALGORITHM_ATTRIBUTES_LENGTH;

            opgpHalMemCpy(&responseAPDU->data[offset], algorithmAttributes, OPGP_CORE_ALGORITHM_ATTRIBUTES_LENGTH);

            offset += OPGP_CORE_ALGORITHM_ATTRIBUTES_LENGTH;

            responseAPDU->data[offset++] = OPGP_GLOBAL_TAG_ALGORITHM_ATTRIBUTES_AUTHENTICATION;
            responseAPDU->data[offset++] = OPGP_CORE_ALGORITHM_ATTRIBUTES_LENGTH;

            opgpHalMemCpy(&responseAPDU->data[offset], algorithmAttributes, OPGP_CORE_ALGORITHM_ATTRIBUTES_LENGTH);

            offset += OPGP_CORE_ALGORITHM_ATTRIBUTES_LENGTH;

            responseAPDU->data[offset++] = OPGP_GLOBAL_TAG_PW_STATUS_BYTES;
            responseAPDU->data[offset++] = OPGP_CORE_PW_STATUS_LENGTH;

            opgpHalGetDataObject(OPGP_GLOBAL_TAG_PW_STATUS_BYTES, &responseAPDU->data[offset++]);

            responseAPDU->data[offset++] = OPGP_GLOBAL_MAX_PIN_LENGTH;
            responseAPDU->data[offset++] = OPGP_GLOBAL_MAX_PIN_LENGTH;
            responseAPDU->data[offset++] = OPGP_GLOBAL_MAX_PIN_LENGTH;

            opgpHalGetPinErrorCounter(OPGP_GLOBAL_PIN_ID_PW1_81, &responseAPDU->data[offset++]);

            opgpHalGetPinErrorCounter(OPGP_GLOBAL_PIN_ID_RC, &responseAPDU->data[offset++]);

            opgpHalGetPinErrorCounter(OPGP_GLOBAL_PIN_ID_PW3, &responseAPDU->data[offset++]);

            responseAPDU->data[offset++] = OPGP_GLOBAL_TAG_FINGERPRINTS;
            responseAPDU->data[offset++] = OPGP_GLOBAL_DO_FINGERPRINT_LENGTH * 3;

            opgpHalGetDataObject(OPGP_GLOBAL_TAG_SIGNATURE_FINGERPRINT, &responseAPDU->data[offset]);

            offset += OPGP_GLOBAL_DO_FINGERPRINT_LENGTH;

            opgpHalGetDataObject(OPGP_GLOBAL_TAG_DECRYPTION_FINGERPRINT, &responseAPDU->data[offset]);

            offset += OPGP_GLOBAL_DO_FINGERPRINT_LENGTH;

            opgpHalGetDataObject(OPGP_GLOBAL_TAG_AUTHENTICATION_FINGERPRINT, &responseAPDU->data[offset]);

            offset += OPGP_GLOBAL_DO_FINGERPRINT_LENGTH;

            responseAPDU->data[offset++] = OPGP_GLOBAL_TAG_CA_FINGERPRINTS;
            responseAPDU->data[offset++] = OPGP_GLOBAL_DO_CA_FINGERPRINT_LENGTH * 3;

            opgpHalGetDataObject(OPGP_GLOBAL_TAG_1ST_CA_FINGERPRINT, &responseAPDU->data[offset]);

            offset += OPGP_GLOBAL_DO_CA_FINGERPRINT_LENGTH;

            opgpHalGetDataObject(OPGP_GLOBAL_TAG_2ND_CA_FINGERPRINT, &responseAPDU->data[offset]);

            offset += OPGP_GLOBAL_DO_CA_FINGERPRINT_LENGTH;

            opgpHalGetDataObject(OPGP_GLOBAL_TAG_3RD_CA_FINGERPRINT, &responseAPDU->data[offset]);

            offset += OPGP_GLOBAL_DO_CA_FINGERPRINT_LENGTH;

            responseAPDU->data[offset++] = OPGP_GLOBAL_TAG_GENERATION_DATE_TIMES;
            responseAPDU->data[offset++] = OPGP_GLOBAL_DO_GENERATION_DATE_TIME_LENGTH * 3;

            opgpHalGetDataObject(OPGP_GLOBAL_TAG_GENERATION_DATE_TIME_SIGNATURE_KEY, &responseAPDU->data[offset]);

            offset += OPGP_GLOBAL_DO_GENERATION_DATE_TIME_LENGTH;

            opgpHalGetDataObject(OPGP_GLOBAL_TAG_GENERATION_DATE_TIME_DECRYPTION_KEY, &responseAPDU->data[offset]);

            offset += OPGP_GLOBAL_DO_GENERATION_DATE_TIME_LENGTH;

            opgpHalGetDataObject(OPGP_GLOBAL_TAG_GENERATION_DATE_TIME_AUTHENTICATION_KEY, &responseAPDU->data[offset]);

            offset += OPGP_GLOBAL_DO_GENERATION_DATE_TIME_LENGTH;

            responseAPDU->data[discretionaryDataObjectsPosition] = (offset - discretionaryDataObjectsPosition - 1);
            responseAPDU->dataLength = offset;
        }
        break;
        case OPGP_GLOBAL_TAG_SECURITY_SUPPORT_TEMPLATE:
        {
            uint32_t offset = 0;
            uint32_t signatureCounter;

            responseAPDU->data[offset++] = OPGP_GLOBAL_TAG_DS_COUNTER;
            responseAPDU->data[offset++] = 0x03;

            opgpHalGetSignatureCounter(&signatureCounter);

            responseAPDU->data[offset++] = OPGP_LOBYTE(OPGP_HIWORD(signatureCounter));
            responseAPDU->data[offset++] = OPGP_HIBYTE(OPGP_LOWORD(signatureCounter));
            responseAPDU->data[offset++] = OPGP_LOBYTE(OPGP_LOWORD(signatureCounter));

            responseAPDU->dataLength = offset;
        }
        break;

        case OPGP_GLOBAL_TAG_CARD_HOLDER_CERTIFICATE:
        {
            opgpHalGetDataObjectWithLength(OPGP_GLOBAL_TAG_CARD_HOLDER_CERTIFICATE, responseAPDU->data,
                                           &responseAPDU->dataLength);
        }
        break;

        case OPGP_GLOBAL_TAG_PW_STATUS_BYTES:
        {
            uint32_t offset = 0;

            opgpHalGetDataObject(OPGP_GLOBAL_TAG_PW_STATUS_BYTES, &responseAPDU->data[offset++]);

            responseAPDU->data[offset++] = OPGP_GLOBAL_MAX_PIN_LENGTH;
            responseAPDU->data[offset++] = OPGP_GLOBAL_MAX_PIN_LENGTH;
            responseAPDU->data[offset++] = OPGP_GLOBAL_MAX_PIN_LENGTH;

            opgpHalGetPinErrorCounter(OPGP_GLOBAL_PIN_ID_PW1_81, &responseAPDU->data[offset++]);

            opgpHalGetPinErrorCounter(OPGP_GLOBAL_PIN_ID_RC, &responseAPDU->data[offset++]);

            opgpHalGetPinErrorCounter(OPGP_GLOBAL_PIN_ID_PW3, &responseAPDU->data[offset++]);

            responseAPDU->dataLength = offset;
        }
        break;

        default:
            sw = APDU_CORE_SW_WRONG_P1P2;
            goto END;
    }

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void opgpCoreProcessVerify(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t calleeRetVal = OPGP_GENERAL_ERROR;
    uint16_t sw;

    responseAPDU->dataLength = 0x00;

#ifndef OPGP_CORE_IGNORE_LE
    if (commandAPDU->lePresent == APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }
#endif /* OPGP_CORE_IGNORE_LE */

    if (commandAPDU->lcPresent == APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if ((commandAPDU->p1p2 != OPGP_GLOBAL_PIN_ID_PW1_81) && (commandAPDU->p1p2 != OPGP_GLOBAL_PIN_ID_PW1_82) &&
        (commandAPDU->p1p2 != OPGP_GLOBAL_PIN_ID_PW3))
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    calleeRetVal = opgpPinVerifyPin((uint8_t)commandAPDU->p1p2, commandAPDU->data, commandAPDU->lc);

    if (calleeRetVal == OPGP_INVALID_PIN_ERROR)
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }
    else if (calleeRetVal == OPGP_INVALID_PIN_LENGTH_ERROR)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }
    else if (calleeRetVal == OPGP_PIN_BLOCKED_ERROR)
    {
        sw = APDU_CORE_SW_PIN_BLOCKED;
        goto END;
    }
    else if (calleeRetVal != OPGP_NO_ERROR)
    {
        opgpHalFatalError();
    }

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void opgpCoreProcessChangeReferenceData(APDU_CORE_COMMAND_APDU* commandAPDU,
                                               APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t calleeRetVal = OPGP_GENERAL_ERROR;
    uint16_t sw;

    responseAPDU->dataLength = 0x00;

#ifndef OPGP_CORE_IGNORE_LE
    if (commandAPDU->lePresent == APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }
#endif /* OPGP_CORE_IGNORE_LE */

    if (commandAPDU->lcPresent == APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if ((commandAPDU->p1p2 != OPGP_GLOBAL_PIN_ID_PW1_81) && (commandAPDU->p1p2 != OPGP_GLOBAL_PIN_ID_PW3))
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    calleeRetVal = opgpPinChangePin((uint8_t)commandAPDU->p1p2, commandAPDU->data, commandAPDU->lc);

    if (calleeRetVal == OPGP_INVALID_PIN_ERROR)
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }
    else if (calleeRetVal == OPGP_INVALID_PIN_LENGTH_ERROR)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }
    else if (calleeRetVal == OPGP_PIN_BLOCKED_ERROR)
    {
        sw = APDU_CORE_SW_PIN_BLOCKED;
        goto END;
    }
    else if (calleeRetVal != OPGP_NO_ERROR)
    {
        opgpHalFatalError();
    }

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void opgpCoreProcessResetRetryCounter(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t calleeRetVal = OPGP_GENERAL_ERROR;
    uint16_t sw;

    responseAPDU->dataLength = 0x00;

#ifndef OPGP_CORE_IGNORE_LE
    if (commandAPDU->lePresent == APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }
#endif /* OPGP_CORE_IGNORE_LE */

    if (commandAPDU->lcPresent == APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if ((commandAPDU->p1p2 != OPGP_CORE_P1P2_RESET_RETRY_COUNTER_WITHOUT_RC) &&
        (commandAPDU->p1p2 != OPGP_CORE_P1P2_RESET_RETRY_COUNTER_WITH_RC))
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (commandAPDU->p1p2 == OPGP_CORE_P1P2_RESET_RETRY_COUNTER_WITHOUT_RC)
    {
        if (opgpPinIsPW3Verified() != OPGP_TRUE)
        {
            sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
            goto END;
        }

        calleeRetVal = opgpPinUnblockPW1(commandAPDU->data, commandAPDU->lc);
    }
    else if (commandAPDU->p1p2 == OPGP_CORE_P1P2_RESET_RETRY_COUNTER_WITH_RC)
    {
        calleeRetVal = opgpPinUnblockPW1WithRC(commandAPDU->data, commandAPDU->lc);
    }

    if (calleeRetVal == OPGP_INVALID_PIN_ERROR)
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }
    else if (calleeRetVal == OPGP_INVALID_PIN_LENGTH_ERROR)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }
    else if (calleeRetVal == OPGP_PIN_BLOCKED_ERROR)
    {
        sw = APDU_CORE_SW_PIN_BLOCKED;
        goto END;
    }
    else if (calleeRetVal != OPGP_NO_ERROR)
    {
        opgpHalFatalError();
    }

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void opgpCoreProcessPutData(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t calleeRetVal = OPGP_GENERAL_ERROR;
    uint16_t sw;

    responseAPDU->dataLength = 0x00;

#ifndef OPGP_CORE_IGNORE_LE
    if (commandAPDU->lePresent == APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }
#endif /* OPGP_CORE_IGNORE_LE */

    switch (commandAPDU->p1p2)
    {
        case OPGP_GLOBAL_TAG_PRIVATE_USE_1:
        case OPGP_GLOBAL_TAG_PRIVATE_USE_3:
        {
            uint32_t maximumLength;

            if (commandAPDU->p1p2 == OPGP_GLOBAL_TAG_PRIVATE_USE_1)
            {
                maximumLength = OPGP_GLOBAL_DO_PRIVATE_USE_1_MAX_LENGTH;
            }
            else
            {
                maximumLength = OPGP_GLOBAL_DO_PRIVATE_USE_3_MAX_LENGTH;
            }

            if (opgpPinIsPW1_82_Verified() != OPGP_TRUE)
            {
                sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
                goto END;
            }

            if (commandAPDU->lc > maximumLength)
            {
                sw = APDU_CORE_SW_WRONG_LENGTH;
                goto END;
            }

            opgpHalSetDataObjectWithLength(commandAPDU->p1p2, commandAPDU->data, commandAPDU->lc);
        }
        break;
        case OPGP_GLOBAL_TAG_PRIVATE_USE_2:
        case OPGP_GLOBAL_TAG_PRIVATE_USE_4:
        case OPGP_GLOBAL_TAG_NAME:
        case OPGP_GLOBAL_TAG_LOGIN_DATA:
        case OPGP_GLOBAL_TAG_LANGUAGE_PREFERENCE:
        case OPGP_GLOBAL_TAG_URL:
        case OPGP_GLOBAL_TAG_CARD_HOLDER_CERTIFICATE:
        {
            uint32_t maximumLength;

            if (commandAPDU->p1p2 == OPGP_GLOBAL_TAG_PRIVATE_USE_2)
            {
                maximumLength = OPGP_GLOBAL_DO_PRIVATE_USE_2_MAX_LENGTH;
            }
            else if (commandAPDU->p1p2 == OPGP_GLOBAL_TAG_PRIVATE_USE_4)
            {
                maximumLength = OPGP_GLOBAL_DO_PRIVATE_USE_4_MAX_LENGTH;
            }
            else if (commandAPDU->p1p2 == OPGP_GLOBAL_TAG_NAME)
            {
                maximumLength = OPGP_GLOBAL_DO_NAME_MAX_LENGTH;
            }
            else if (commandAPDU->p1p2 == OPGP_GLOBAL_TAG_LOGIN_DATA)
            {
                maximumLength = OPGP_GLOBAL_DO_NAME_MAX_LENGTH;
            }
            else if (commandAPDU->p1p2 == OPGP_GLOBAL_TAG_LANGUAGE_PREFERENCE)
            {
                maximumLength = OPGP_GLOBAL_DO_LANGUAGE_PREFERENCE_MAX_LENGTH;
            }
            else if (commandAPDU->p1p2 == OPGP_GLOBAL_TAG_URL)
            {
                maximumLength = OPGP_GLOBAL_DO_URL_MAX_LENGTH;
            }
            else if (commandAPDU->p1p2 == OPGP_GLOBAL_TAG_CARD_HOLDER_CERTIFICATE)
            {
                maximumLength = OPGP_GLOBAL_DO_CERTIFICATE_MAX_LENGTH;
            }

            if (opgpPinIsPW3Verified() != OPGP_TRUE)
            {
                sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
                goto END;
            }

            if (commandAPDU->lc > maximumLength)
            {
                sw = APDU_CORE_SW_WRONG_LENGTH;
                goto END;
            }

            opgpHalSetDataObjectWithLength(commandAPDU->p1p2, commandAPDU->data, commandAPDU->lc);
        }
        break;

        case OPGP_GLOBAL_TAG_SEX:
        case OPGP_GLOBAL_TAG_SIGNATURE_FINGERPRINT:
        case OPGP_GLOBAL_TAG_DECRYPTION_FINGERPRINT:
        case OPGP_GLOBAL_TAG_AUTHENTICATION_FINGERPRINT:
        case OPGP_GLOBAL_TAG_1ST_CA_FINGERPRINT:
        case OPGP_GLOBAL_TAG_2ND_CA_FINGERPRINT:
        case OPGP_GLOBAL_TAG_3RD_CA_FINGERPRINT:
        case OPGP_GLOBAL_TAG_GENERATION_DATE_TIME_SIGNATURE_KEY:
        case OPGP_GLOBAL_TAG_GENERATION_DATE_TIME_DECRYPTION_KEY:
        case OPGP_GLOBAL_TAG_GENERATION_DATE_TIME_AUTHENTICATION_KEY:
        {
            uint32_t length;

            if (commandAPDU->p1p2 == OPGP_GLOBAL_TAG_SEX)
            {
                length = OPGP_GLOBAL_DO_SEX_LENGTH;
            }
            else if ((commandAPDU->p1p2 == OPGP_GLOBAL_TAG_SIGNATURE_FINGERPRINT) ||
                     (commandAPDU->p1p2 == OPGP_GLOBAL_TAG_DECRYPTION_FINGERPRINT) ||
                     (commandAPDU->p1p2 == OPGP_GLOBAL_TAG_AUTHENTICATION_FINGERPRINT))
            {
                length = OPGP_GLOBAL_DO_FINGERPRINT_LENGTH;
            }
            else if ((commandAPDU->p1p2 == OPGP_GLOBAL_TAG_1ST_CA_FINGERPRINT) ||
                     (commandAPDU->p1p2 == OPGP_GLOBAL_TAG_2ND_CA_FINGERPRINT) ||
                     (commandAPDU->p1p2 == OPGP_GLOBAL_TAG_3RD_CA_FINGERPRINT))
            {
                length = OPGP_GLOBAL_DO_CA_FINGERPRINT_LENGTH;
            }
            else if ((commandAPDU->p1p2 == OPGP_GLOBAL_TAG_GENERATION_DATE_TIME_SIGNATURE_KEY) ||
                     (commandAPDU->p1p2 == OPGP_GLOBAL_TAG_GENERATION_DATE_TIME_DECRYPTION_KEY) ||
                     (commandAPDU->p1p2 == OPGP_GLOBAL_TAG_GENERATION_DATE_TIME_AUTHENTICATION_KEY))
            {
                length = OPGP_GLOBAL_DO_GENERATION_DATE_TIME_LENGTH;
            }

            if (opgpPinIsPW3Verified() != OPGP_TRUE)
            {
                sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
                goto END;
            }

            if (commandAPDU->lc != length)
            {
                sw = APDU_CORE_SW_WRONG_LENGTH;
                goto END;
            }

            opgpHalSetDataObject(commandAPDU->p1p2, commandAPDU->data);
        }
        break;
        case OPGP_GLOBAL_TAG_PW_STATUS_BYTES:
        {
            if (opgpPinIsPW3Verified() != OPGP_TRUE)
            {
                sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
                goto END;
            }

            if (commandAPDU->lc != OPGP_GLOBAL_DO_PW_STATUS_BYTE_LENGTH)
            {
                sw = APDU_CORE_SW_WRONG_LENGTH;
                goto END;
            }

            if ((commandAPDU->data[0] != OPGP_GLOBAL_PW1_81_VALID_FOR_ONE_COMMAND) &&
                (commandAPDU->data[0] != OPGP_GLOBAL_PW1_81_VALID_FOR_MULTIPLE_COMMANDS))
            {
                sw = APDU_CORE_SW_WRONG_DATA;
                goto END;
            }

            opgpHalSetDataObject(OPGP_GLOBAL_TAG_PW_STATUS_BYTES, commandAPDU->data);
        }
        break;
        case OPGP_GLOBAL_TAG_RESETTING_CODE:
        {
            if (opgpPinIsPW3Verified() != OPGP_TRUE)
            {
                sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
                goto END;
            }

            calleeRetVal = opgpPinSetResettingCode(commandAPDU->data, commandAPDU->lc);

            if (calleeRetVal == OPGP_INVALID_PIN_LENGTH_ERROR)
            {
                sw = APDU_CORE_SW_WRONG_LENGTH;
                goto END;
            }
            else if (calleeRetVal != OPGP_NO_ERROR)
            {
                opgpHalFatalError();
            }
        }
        break;

        default:
            sw = APDU_CORE_SW_WRONG_P1P2;
            goto END;
    }

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void opgpCoreProcessPutDataImportKey(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t calleeRetVal = OPGP_GENERAL_ERROR;
    uint16_t sw;
    uint8_t commandHeader[] = OPGP_CORE_IMPORT_KEY_HEADER;
    uint16_t comparisonResult;
    uint16_t keyType;

    responseAPDU->dataLength = 0x00;

#ifndef OPGP_CORE_IGNORE_LE
    if (commandAPDU->lePresent == APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }
#endif /* OPGP_CORE_IGNORE_LE */

    if (commandAPDU->lcPresent == APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != OPGP_CORE_P1P2_PUT_DATA_IMPORT_KEY)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (opgpPinIsPW3Verified() != OPGP_TRUE)
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }

    if (commandAPDU->lc != OPGP_CORE_IMPORT_KEY_IMCOMING_DATA_LENGTH)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    commandHeader[OPGP_CORE_KEY_IMPORT_KEY_TYPE_OFFSET] = commandAPDU->data[OPGP_CORE_KEY_IMPORT_KEY_TYPE_OFFSET];

    comparisonResult = opgpHalMemCmp(commandHeader, commandAPDU->data, OPGP_CORE_IMPORT_KEY_HEADER_LENGTH);

    if (comparisonResult != OPGP_CMP_EQUAL)
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

    keyType = OPGP_MAKEWORD(commandAPDU->data[OPGP_CORE_KEY_IMPORT_KEY_TYPE_OFFSET + 1],
                            commandAPDU->data[OPGP_CORE_KEY_IMPORT_KEY_TYPE_OFFSET]);

    if ((keyType != OPGP_GLOBAL_KEY_TYPE_SIGNATURE) && (keyType != OPGP_GLOBAL_KEY_TYPE_CONFIDENTIALITY) &&
        (keyType != OPGP_GLOBAL_KEY_TYPE_AUTHENTICATION))
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

    calleeRetVal = opgpHalImportKey(keyType, &commandAPDU->data[OPGP_CORE_IMPORT_KEY_PUBLIC_EXPONENT_OFFSET],
                                    &commandAPDU->data[OPGP_CORE_IMPORT_KEY_P_OFFSET],
                                    &commandAPDU->data[OPGP_CORE_IMPORT_KEY_Q_OFFSET]);

    if (calleeRetVal == OPGP_INVALID_CRYPTO_DATA_ERROR)
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }
    else if (calleeRetVal != OPGP_NO_ERROR)
    {
        opgpHalFatalError();
    }

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void opgpCoreProcessGenKeyPair(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;

    responseAPDU->dataLength = 0x00;

#ifndef OPGP_CORE_IGNORE_LE
    if ((commandAPDU->lePresent == APDU_FALSE) || (commandAPDU->le != 0x00))
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }
#endif /* OPGP_CORE_IGNORE_LE */

    if ((commandAPDU->lcPresent == APDU_FALSE) || (commandAPDU->lc != OPGP_CORE_P1P2_LC_GENERATE_KEY_PAIR))
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 == OPGP_CORE_P1P2_GENERATE_KEY_PAIR)
    {
        uint16_t keyType;

        if (opgpPinIsPW3Verified() != OPGP_TRUE)
        {
            sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
            goto END;
        }

        keyType = OPGP_MAKEWORD(commandAPDU->data[1], commandAPDU->data[0]);

        if ((keyType != OPGP_GLOBAL_KEY_TYPE_SIGNATURE) && (keyType != OPGP_GLOBAL_KEY_TYPE_CONFIDENTIALITY) &&
            (keyType != OPGP_GLOBAL_KEY_TYPE_AUTHENTICATION))
        {
            sw = APDU_CORE_SW_WRONG_DATA;
            goto END;
        }

        opgpHalGenerateKeyPair(keyType);

        opgpHalResetSignatureCounter();

        opgpCoreSetPublicKeyDO(keyType, responseAPDU->data);

        responseAPDU->dataLength = OPGP_CORE_PUBLIC_KEY_DO_LENGTH;
    }
    else if (commandAPDU->p1p2 == OPGP_CORE_P1P2_READ_PUBLIC_KEY)
    {
        uint16_t keyType;
        uint16_t keyInitialized = OPGP_FALSE;

        keyType = OPGP_MAKEWORD(commandAPDU->data[1], commandAPDU->data[0]);

        if ((keyType != OPGP_GLOBAL_KEY_TYPE_SIGNATURE) && (keyType != OPGP_GLOBAL_KEY_TYPE_CONFIDENTIALITY) &&
            (keyType != OPGP_GLOBAL_KEY_TYPE_AUTHENTICATION))
        {
            sw = APDU_CORE_SW_WRONG_DATA;
            goto END;
        }

        opgpHalIsKeyInitialized(keyType, &keyInitialized);

        if (keyInitialized != OPGP_TRUE)
        {
            sw = APDU_CORE_SW_REF_DATA_NOT_FOUND;
            goto END;
        }

        opgpCoreSetPublicKeyDO(keyType, responseAPDU->data);

        responseAPDU->dataLength = OPGP_CORE_PUBLIC_KEY_DO_LENGTH;
    }
    else
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void opgpCoreSetPublicKeyDO(uint16_t keyType, uint8_t* buffer)
{
    uint8_t publicKeyDOHeader[] = OPGP_CORE_PUBLIC_KEY_DO_HEADER;

    opgpHalMemCpy(buffer, publicKeyDOHeader, sizeof(publicKeyDOHeader));

    buffer[OPGP_CORE_PUBLIC_KEY_DO_PUBLIC_EXPONENT_TAG_OFFSET] = OPGP_CORE_PUBLIC_KEY_DO_PUBLIC_EXPONENT_TAG;
    buffer[OPGP_CORE_PUBLIC_KEY_DO_PUBLIC_EXPONENT_LENGTH_OFFSET] = OPGP_GLOBAL_PUBLIC_EXPONENT_LENGTH;

    opgpHalGetPublicKey(keyType, &buffer[OPGP_CORE_PUBLIC_KEY_DO_MODULUS_OFFSET],
                        &buffer[OPGP_CORE_PUBLIC_KEY_DO_PUBLIC_EXPONENT_OFFSET]);
}

static void opgpCoreProcessPSO(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t calleeRetVal = OPGP_GENERAL_ERROR;
    uint16_t sw;

    responseAPDU->dataLength = 0x00;

#ifndef OPGP_CORE_IGNORE_LE
    if ((commandAPDU->lePresent == APDU_FALSE) || (commandAPDU->le != 0x00))
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }
#endif /* OPGP_CORE_IGNORE_LE */

    if (commandAPDU->lcPresent == APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 == OPGP_CORE_P1P2_COMPUTE_DIGITAL_SIGNATURE)
    {
        uint8_t pw1Status;
        uint16_t keyInitialized = OPGP_FALSE;

        opgpHalGetDataObject(OPGP_GLOBAL_TAG_PW_STATUS_BYTES, &pw1Status);

        if (opgpPinIsPW1_81_Verified() != OPGP_TRUE)
        {
            sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
            goto END;
        }

        if (pw1Status == OPGP_GLOBAL_PW1_81_VALID_FOR_ONE_COMMAND)
        {
            opgpPinResetPW1_81_Status();
        }

        if (commandAPDU->lc > OPGP_CORE_MAX_DSI_LENGTH)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }

        opgpHalIsKeyInitialized(OPGP_GLOBAL_KEY_TYPE_SIGNATURE, &keyInitialized);

        if (keyInitialized != OPGP_TRUE)
        {
            sw = APDU_CORE_SW_REF_DATA_NOT_FOUND;
            goto END;
        }

        opgpHalSign(commandAPDU->data, commandAPDU->lc, responseAPDU->data);

        responseAPDU->dataLength = OPGP_GLOBAL_MODULUS_LENGTH;

        opgpHalIncrementSignatureCounter();
    }
    else if (commandAPDU->p1p2 == OPGP_CORE_P1P2_DECIPHER)
    {
        uint16_t keyInitialized = OPGP_FALSE;

        if (opgpPinIsPW1_82_Verified() != OPGP_TRUE)
        {
            sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
            goto END;
        }

        if (commandAPDU->lc != (OPGP_GLOBAL_MODULUS_LENGTH + 1))
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }

        if (commandAPDU->data[0x00] != OPGP_CORE_DECIPHER_PADDING_INDICATOR)
        {
            sw = APDU_CORE_SW_WRONG_DATA;
            goto END;
        }

        opgpHalIsKeyInitialized(OPGP_GLOBAL_KEY_TYPE_CONFIDENTIALITY, &keyInitialized);

        if (keyInitialized != OPGP_TRUE)
        {
            sw = APDU_CORE_SW_REF_DATA_NOT_FOUND;
            goto END;
        }

        calleeRetVal = opgpHalDecipher(&commandAPDU->data[1], responseAPDU->data, &responseAPDU->dataLength);

        if (calleeRetVal == OPGP_INVALID_CRYPTO_DATA_ERROR)
        {
            sw = APDU_CORE_SW_WRONG_DATA;
            goto END;
        }
        else if (calleeRetVal != OPGP_NO_ERROR)
        {
            opgpHalFatalError();
        }
    }
    else
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void opgpCoreProcessInternalAuthenticate(APDU_CORE_COMMAND_APDU* commandAPDU,
                                                APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint16_t keyInitialized = OPGP_FALSE;

    responseAPDU->dataLength = 0x00;

#ifndef OPGP_CORE_IGNORE_LE
    if ((commandAPDU->lePresent == APDU_FALSE) || (commandAPDU->le != 0x00))
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }
#endif /* OPGP_CORE_IGNORE_LE */

    if (commandAPDU->lcPresent == APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != OPGP_CORE_P1P2_INTERNAL_AUTHENTICATE)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (opgpPinIsPW1_82_Verified() != OPGP_TRUE)
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }

    if (commandAPDU->lc > OPGP_CORE_MAX_AUTHENTICATION_INPUT_LENGTH)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    opgpHalIsKeyInitialized(OPGP_GLOBAL_KEY_TYPE_AUTHENTICATION, &keyInitialized);

    if (keyInitialized != OPGP_TRUE)
    {
        sw = APDU_CORE_SW_REF_DATA_NOT_FOUND;
        goto END;
    }

    opgpHalInternalAuthenticate(commandAPDU->data, commandAPDU->lc, responseAPDU->data);

    responseAPDU->dataLength = OPGP_GLOBAL_MODULUS_LENGTH;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void opgpCoreProcessGetChallenge(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;

    responseAPDU->dataLength = 0x00;

    if ((commandAPDU->lePresent == APDU_FALSE) || (commandAPDU->le > OPGP_CORE_CHALLENGE_MAX_LENGTH))
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->lcPresent == APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != OPGP_CORE_P1P2_GET_CHALLENGE)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    opgpHalGetRandom(responseAPDU->data, commandAPDU->le);

    responseAPDU->dataLength = commandAPDU->le;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void opgpCoreProcessTerminateDF(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;

    responseAPDU->dataLength = 0x00;

#ifndef OPGP_CORE_IGNORE_LE
    if (commandAPDU->lePresent == APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }
#endif /* OPGP_CORE_IGNORE_LE */

    if (commandAPDU->lcPresent == APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != OPGP_CORE_P1P2_TERMINATE_DF)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    opgpHalSetCardState(OPGP_GLOBAL_CARD_STATE_INITIALIZATION);

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void opgpCoreProcessActivateFile(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint8_t cardState;

    responseAPDU->dataLength = 0x00;

#ifndef OPGP_CORE_IGNORE_LE
    if (commandAPDU->lePresent == APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }
#endif /* OPGP_CORE_IGNORE_LE */

    if (commandAPDU->lcPresent == APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != OPGP_CORE_P1P2_ACTIVATE_FILE)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    opgpHalGetCardState(&cardState);

    if (cardState == OPGP_GLOBAL_CARD_STATE_INITIALIZATION)
    {
        opgpHalWipeout();
    }
    else
    {
        sw = APDU_CORE_SW_NO_ERROR;
        goto END;
    }

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

void opgpCoreProcessAPDU(uint8_t* apdu, uint32_t* apduLength)
{
    uint16_t calleeRetVal = OPGP_GENERAL_ERROR;
    APDU_CORE_COMMAND_APDU commandAPDU;
    APDU_CORE_RESPONSE_APDU responseAPDU;
    uint8_t cardState;

    if ((apdu == NULL) || (apduLength == NULL))
    {
        opgpHalFatalError();
    }

    apduCorePrepareResponseAPDUStructure(apdu, &responseAPDU);

    calleeRetVal = apduCoreParseIncomingAPDU(apdu, *apduLength, &commandAPDU);

    if (calleeRetVal != APDU_NO_ERROR)
    {
        if (calleeRetVal == APDU_GENERAL_ERROR)
        {
            responseAPDU.dataLength = 0;
            responseAPDU.sw = APDU_CORE_SW_WRONG_LENGTH;

            goto END;
        }
        else
        {
            opgpHalFatalError();
        }
    }

    if (commandAPDU.cla != OPGP_CORE_CLA_NORMAL)
    {
        responseAPDU.dataLength = 0;
        responseAPDU.sw = APDU_CORE_SW_CLA_NOT_SUPPORTED;
        goto END;
    }

    opgpHalGetCardState(&cardState);

    if (cardState == OPGP_GLOBAL_CARD_STATE_INITIALIZATION)
    {
        if (commandAPDU.ins == OPGP_CORE_INS_ACTIVATE_FILE)
        {
            opgpCoreProcessActivateFile(&commandAPDU, &responseAPDU);
        }
        else
        {
            responseAPDU.dataLength = 0;
            responseAPDU.sw = APDU_CORE_SW_INS_NOT_SUPPORTED;
        }

        goto END;
    }

    switch (commandAPDU.ins)
    {
        case OPGP_CORE_INS_GET_DATA:
            opgpCoreProcessGetData(&commandAPDU, &responseAPDU);
            break;
        case OPGP_CORE_INS_VERIFY:
            opgpCoreProcessVerify(&commandAPDU, &responseAPDU);
            break;
        case OPGP_CORE_INS_CHANGE_REFERENCE_DATA:
            opgpCoreProcessChangeReferenceData(&commandAPDU, &responseAPDU);
            break;
        case OPGP_CORE_INS_RESET_RETRY_COUNTER:
            opgpCoreProcessResetRetryCounter(&commandAPDU, &responseAPDU);
            break;
        case OPGP_CORE_INS_PUT_DATA:
            opgpCoreProcessPutData(&commandAPDU, &responseAPDU);
            break;
        case OPGP_CORE_INS_PUT_DATA_IMPORT_KEY:
            opgpCoreProcessPutDataImportKey(&commandAPDU, &responseAPDU);
            break;
        case OPGP_CORE_INS_GEN_KEY_PAIR:
            opgpCoreProcessGenKeyPair(&commandAPDU, &responseAPDU);
            break;
        case OPGP_CORE_INS_PSO:
            opgpCoreProcessPSO(&commandAPDU, &responseAPDU);
            break;
        case OPGP_CORE_INS_INTERNAL_AUTHENTICATE:
            opgpCoreProcessInternalAuthenticate(&commandAPDU, &responseAPDU);
            break;
        case OPGP_CORE_INS_GET_CHALLENGE:
            opgpCoreProcessGetChallenge(&commandAPDU, &responseAPDU);
            break;
        case OPGP_CORE_INS_TERMINATE_DF:
            opgpCoreProcessTerminateDF(&commandAPDU, &responseAPDU);
            break;
        case OPGP_CORE_INS_ACTIVATE_FILE:
            opgpCoreProcessActivateFile(&commandAPDU, &responseAPDU);
            break;
        default:
            responseAPDU.dataLength = 0;
            responseAPDU.sw = APDU_CORE_SW_INS_NOT_SUPPORTED;
            break;
    }

    goto END;

END:

    apduCorePrepareOutgoingAPDU(apdu, apduLength, &responseAPDU);
}

void opgpCoreSelect(uint16_t* sw)
{
    uint8_t cardState;

    if (sw == NULL)
    {
        opgpHalFatalError();
    }

    opgpPinResetVolatilePinStatus();

    opgpHalGetCardState(&cardState);

    if (cardState == OPGP_GLOBAL_CARD_STATE_OPERATIONAL)
    {
        *sw = APDU_CORE_SW_NO_ERROR;
    }
    else
    {
        *sw = APDU_CORE_SW_TERMINATION_STATE;
    }
}

void opgpCoreGetAID(uint8_t* aid, uint32_t* aidLength)
{
    uint8_t aidTemplate[] = OPGP_CORE_AID;

    if ((aid == NULL) || (aidLength == NULL))
    {
        opgpHalFatalError();
    }

    opgpHalMemCpy(aid, aidTemplate, OPGP_CORE_AID_LENGTH);

    opgpHalGetSerialNumber(&aid[OPGP_CORE_AID_SERIAL_NUMBER_OFFSET]);

    *aidLength = OPGP_CORE_AID_LENGTH;
}

void opgpCoreGetHistChars(uint8_t* histChars, uint32_t* histCharsLength)
{
    uint8_t histCharsTemplate[] = OPGP_CORE_HIST_CHARS;

    if ((histChars == NULL) || (histCharsLength == NULL))
    {
        opgpHalFatalError();
    }

    opgpHalMemCpy(histChars, histCharsTemplate, OPGP_CORE_HIST_CHARS_LENGTH);

    *histCharsLength = OPGP_CORE_HIST_CHARS_LENGTH;
}
