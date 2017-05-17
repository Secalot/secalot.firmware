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
#include <otpCore.h>
#include <core/otpCoreInt.h>
#include <otpHal.h>

#include <apduGlobal.h>
#include <apduCore.h>

static void otpCoreProcessSetOptions(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void otpCoreProcessSetKeyAndType(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void otpCoreProcessGetOptionsAndType(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void otpCoreProcessSetExternalTime(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);

static void otpCoreIntegerToString(uint8_t* str, uint8_t strLength, uint32_t value);

void otpCoreInit() { otpHalInit(); }

void otpCoreDeinit() { otpHalDeinit(); }

static void otpCoreProcessSetOptions(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint8_t numberOfDigits;

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != OTP_CORE_P1P2_SET_OPTIONS)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (commandAPDU->lc != OTP_CORE_SET_OPTIONS_NUMBER_OF_DIGITS_LENGTH)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    numberOfDigits = commandAPDU->data[OTP_CORE_SET_OPTONS_NUMBER_OF_DIGITS_OFFSET];

    if ((numberOfDigits < OTP_GLOBAL_MIN_DIGITS) || (numberOfDigits > OTP_GLOBAL_MAX_DIGITS))
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

    otpHalSetOptions(numberOfDigits);

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void otpCoreProcessSetKeyAndType(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint8_t keyLength;
    uint16_t type;

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != OTP_CORE_P1P2_SET_KEY_AND_TYPE)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (commandAPDU->lc < 1)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    keyLength = commandAPDU->lc - 1;

    if ((keyLength < OTP_GLOBAL_MIN_KEYLENGTH) || (keyLength > OTP_GLOBAL_MAX_KEYLENGTH))
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->data[0] == OTP_CORE_TYPE_HOTP)
    {
        type = OTP_GLOBAL_TYPE_HOTP;
    }
    else if (commandAPDU->data[0] == OTP_CORE_TYPE_TOTP)
    {
        type = OTP_GLOBAL_TYPE_TOTP;
    }
    else
    {
        sw = APDU_CORE_SW_WRONG_DATA;
        goto END;
    }

    otpHalSetKeySetTypeAndResetCounter(&commandAPDU->data[1], keyLength, type);

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void otpCoreProcessGetOptionsAndType(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint16_t type;

    if (commandAPDU->lcPresent != APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != OTP_CORE_P1P2_GET_OPTIONS_AND_TYPE)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    otpHalGetOptions(&responseAPDU->data[0]);

    otpHalGetType(&type);

    if (type == OTP_GLOBAL_TYPE_HOTP)
    {
        responseAPDU->data[1] = OTP_CORE_TYPE_HOTP;
    }
    else if (type == OTP_GLOBAL_TYPE_TOTP)
    {
        responseAPDU->data[1] = OTP_CORE_TYPE_TOTP;
    }
    else
    {
        otpHalFatalError();
    }

    responseAPDU->dataLength = 2;

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

static void otpCoreProcessSetExternalTime(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    uint32_t externalTime;

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != OTP_CORE_P1P2_SET_EXTERNAL_TIME)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if (commandAPDU->lc != OTP_CORE_SET_EXTERNAL_TIME_TIME_LEMGTH)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    externalTime = OTP_MAKEDWORD(OTP_MAKEWORD(commandAPDU->data[3], commandAPDU->data[2]),
                                 OTP_MAKEWORD(commandAPDU->data[1], commandAPDU->data[0]));

    otpHalSetExternalTime(externalTime);

    sw = APDU_CORE_SW_NO_ERROR;

END:
    responseAPDU->sw = sw;
}

void otpCoreProcessControlAPDU(uint8_t* apdu, uint32_t* apduLength)
{
    APDU_CORE_COMMAND_APDU commandAPDU;
    APDU_CORE_RESPONSE_APDU responseAPDU;
    uint16_t calleeRetVal = APDU_GENERAL_ERROR;

    if ((apdu == NULL) || (apduLength == NULL))
    {
        otpHalFatalError();
    }

    apduCorePrepareResponseAPDUStructure(apdu, &responseAPDU);

    calleeRetVal = apduCoreParseIncomingAPDU(apdu, *apduLength, &commandAPDU);

    if (calleeRetVal != APDU_NO_ERROR)
    {
        if (calleeRetVal == APDU_GENERAL_ERROR)
        {
            responseAPDU.sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }
        else
        {
            otpHalFatalError();
        }
    }

    if (commandAPDU.cla != OTP_CORE_CLA)
    {
        responseAPDU.sw = APDU_CORE_SW_CLA_NOT_SUPPORTED;
        goto END;
    }

    switch (commandAPDU.ins)
    {
        case OTP_CORE_INS_SET_OPTIONS:
            otpCoreProcessSetOptions(&commandAPDU, &responseAPDU);
            break;
        case OTP_CORE_INS_SET_KEY_AND_TYPE:
            otpCoreProcessSetKeyAndType(&commandAPDU, &responseAPDU);
            break;
        case OTP_CORE_INS_GET_OPTIONS_AND_TYPE:
            otpCoreProcessGetOptionsAndType(&commandAPDU, &responseAPDU);
            break;
        case OTP_CORE_INS_SET_EXTERNAL_TIME:
            otpCoreProcessSetExternalTime(&commandAPDU, &responseAPDU);
            break;
        default:
            responseAPDU.sw = APDU_CORE_SW_INS_NOT_SUPPORTED;
            break;
    }

END:
    apduCorePrepareOutgoingAPDU(apdu, apduLength, &responseAPDU);
}

void otpCoreGetAID(uint8_t* aid, uint32_t* aidLength)
{
    uint8_t aidTemplate[] = OTP_CORE_AID;

    if ((aid == NULL) || (aidLength == NULL))
    {
        otpHalFatalError();
    }

    otpHalMemCpy(aid, aidTemplate, OTP_CORE_AID_LENGTH);

    *aidLength = OTP_CORE_AID_LENGTH;
}

static void otpCoreIntegerToString(uint8_t* str, uint8_t strLength, uint32_t value)
{
    uint32_t i;

    for (i = 1; i <= strLength; i++)
    {
        str[strLength - i] = (uint8_t)((value % 10UL) + '0');
        value /= 10;
    }
}

uint16_t otpCoreComputeOtp(uint8_t* otp, uint32_t* otpLength)
{
    uint16_t retVal = OTP_GENERAL_ERROR;
    uint16_t keyInitialized = OTP_FALSE;
    uint8_t key[OTP_GLOBAL_MAX_KEYLENGTH];
    uint32_t keyLength;
    uint8_t numberOfDigits;
    uint16_t type;
    uint64_t counter;
    uint8_t counterAsAByteArray[OTP_GLOBAL_COUNTER_LENGTH];
    uint8_t hmac[OTP_GLOBAL_HAMC_LENGTH];
    uint32_t offset;
    uint32_t binary;
    uint32_t i;
    uint32_t digitPower[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000};
    uint16_t calleeRetVal;
    uint32_t currentTime;

    if ((otp == NULL) || (otpLength == NULL))
    {
        otpHalFatalError();
    }

    otpHalIsKeyInitialized(&keyInitialized);

    if (keyInitialized != OTP_TRUE)
    {
        retVal = OTP_KEY_NOT_SET_ERROR;
        goto END;
    }

    otpHalGetKey(key, &keyLength);

    otpHalGetOptions(&numberOfDigits);

    otpHalGetType(&type);

    if (type == OTP_GLOBAL_TYPE_HOTP)
    {
        otpHalGetCounter(&counter);

        otpHalSetCounter((counter + 1));
    }
    else if (type == OTP_GLOBAL_TYPE_TOTP)
    {
        calleeRetVal = otpHalGetCurrentTime(&currentTime);

        if (calleeRetVal != OTP_NO_ERROR)
        {
            if (calleeRetVal == OTP_TIME_NOT_SET_ERROR)
            {
                retVal = OTP_TIME_NOT_SET_ERROR;
                goto END;
            }
            else
            {
                otpHalFatalError();
            }
        }

        counter = currentTime / OTP_CORE_TI;
    }
    else
    {
        otpHalFatalError();
    }

    for (i = 0; i < OTP_GLOBAL_COUNTER_LENGTH; i++)
    {
        counterAsAByteArray[OTP_GLOBAL_COUNTER_LENGTH - i - 1] = (counter & 0xff);
        counter >>= 8;
    }

    otpHalComputeHmac(key, keyLength, counterAsAByteArray, hmac);

    offset = hmac[OTP_GLOBAL_HAMC_LENGTH - 1] & 0x0F;

    binary = ((hmac[offset] & 0x7F) << 24) | ((hmac[offset + 1] & 0xff) << 16) | ((hmac[offset + 2] & 0xff) << 8) |
             (hmac[offset + 3] & 0xff);

    binary = binary % digitPower[numberOfDigits];

    otpCoreIntegerToString(otp, numberOfDigits, binary);

    *otpLength = numberOfDigits;

    retVal = OTP_NO_ERROR;

END:
    otpHalMemSet(key, 0x00, sizeof(key));
    return retVal;
}
