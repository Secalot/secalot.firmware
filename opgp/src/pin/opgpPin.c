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
#include "opgpHal.h"
#include "opgpPinInt.h"
#include "opgpPin.h"

OPGP_PIN_VOLATILE_PIN_STATUS opgpCoreVolatilePinStatus;

static void opgpPinSetPinStatus(uint8_t pinID, uint16_t pinStatus);

void opgpPinInit(void) { opgpPinResetVolatilePinStatus(); }

void opgpPinResetVolatilePinStatus(void)
{
    opgpCoreVolatilePinStatus.PW1_81_Verified = OPGP_FALSE;
    opgpCoreVolatilePinStatus.PW1_82_Verified = OPGP_FALSE;
    opgpCoreVolatilePinStatus.PW3Verified = OPGP_FALSE;
}

void opgpPinResetPW1_81_Status(void) { opgpCoreVolatilePinStatus.PW1_81_Verified = OPGP_FALSE; }

uint16_t opgpPinIsPW1_81_Verified() { return opgpCoreVolatilePinStatus.PW1_81_Verified; }

uint16_t opgpPinIsPW1_82_Verified() { return opgpCoreVolatilePinStatus.PW1_82_Verified; }

uint16_t opgpPinIsPW3Verified() { return opgpCoreVolatilePinStatus.PW3Verified; }

uint16_t opgpPinVerifyPin(uint8_t pinID, uint8_t* input, uint32_t inputLength)
{
    uint16_t retVal = OPGP_GENERAL_ERROR;
    uint8_t errorCounter;
    uint32_t correctPinLength = 0x00;
    uint16_t comparisonResult = OPGP_CMP_NOT_EQUAL;
    uint32_t minimumPinLength;
    uint8_t candidatePinHash[OPGP_GLOBAL_PIN_HASH_LENGTH];
    uint8_t correctPinHash[OPGP_GLOBAL_PIN_HASH_LENGTH];

    if (input == NULL)
    {
        opgpHalFatalError();
    }

    if ((pinID != OPGP_GLOBAL_PIN_ID_PW1_81) && (pinID != OPGP_GLOBAL_PIN_ID_PW1_82) &&
        (pinID != OPGP_GLOBAL_PIN_ID_PW3) && (pinID != OPGP_GLOBAL_PIN_ID_RC))
    {
        opgpHalFatalError();
    }

    opgpHalGetPinErrorCounter(pinID, &errorCounter);

    if (pinID == OPGP_GLOBAL_PIN_ID_PW1_81)
    {
        minimumPinLength = OPGP_GLOBAL_PW1_MINIMUM_LENGTH;
    }
    else if (pinID == OPGP_GLOBAL_PIN_ID_PW1_82)
    {
        minimumPinLength = OPGP_GLOBAL_PW1_MINIMUM_LENGTH;
    }
    else if (pinID == OPGP_GLOBAL_PIN_ID_PW3)
    {
        minimumPinLength = OPGP_GLOBAL_PW3_MINIMUM_LENGTH;
    }
    else if (pinID == OPGP_GLOBAL_PIN_ID_RC)
    {
        minimumPinLength = OPGP_GLOBAL_RC_MINIMUM_LENGTH;
    }

    if (inputLength < minimumPinLength)
    {
        retVal = OPGP_INVALID_PIN_LENGTH_ERROR;
        goto END;
    }

    if (inputLength > OPGP_GLOBAL_MAX_PIN_LENGTH)
    {
        retVal = OPGP_INVALID_PIN_LENGTH_ERROR;
        goto END;
    }

    if (errorCounter == OPGP_GLOBAL_PIN_BLOCKED_ERROR_COUNTER_VALUE)
    {
        retVal = OPGP_PIN_BLOCKED_ERROR;
        goto END;
    }

    errorCounter--;

    opgpHalSetPinErrorCounter(pinID, errorCounter);

    opgpHalGetPinHashAndLength(pinID, correctPinHash, &correctPinLength);

    opgpHalCalculatePinHash(input, inputLength, candidatePinHash);

    comparisonResult = opgpHalMemCmp(correctPinHash, candidatePinHash, OPGP_GLOBAL_PIN_HASH_LENGTH);

    if ((comparisonResult != OPGP_CMP_EQUAL) || (inputLength != correctPinLength))
    {
        if (pinID != OPGP_GLOBAL_PIN_ID_RC)
        {
            opgpPinSetPinStatus(pinID, OPGP_FALSE);
        }

        retVal = OPGP_INVALID_PIN_ERROR;
        goto END;
    }
    else
    {
        errorCounter = OPGP_GLOBAL_PIN_INITIAL_ERROR_COUNTER_VALUE;

        opgpHalSetPinErrorCounter(pinID, errorCounter);

        if (pinID != OPGP_GLOBAL_PIN_ID_RC)
        {
            opgpPinSetPinStatus(pinID, OPGP_TRUE);
        }
    }

    retVal = OPGP_NO_ERROR;

END:

    return retVal;
}

uint16_t opgpPinChangePin(uint8_t pinID, uint8_t* input, uint32_t inputLength)
{
    uint16_t retVal = OPGP_GENERAL_ERROR;
    uint16_t calleeRetVal = OPGP_GENERAL_ERROR;
    uint32_t correctOldPinLength;
    uint32_t oldPinLength;
    uint32_t newPinLength;
    uint32_t newPinMinimumLength;
    uint8_t newPinHash[OPGP_GLOBAL_PIN_HASH_LENGTH];

    if (input == NULL)
    {
        opgpHalFatalError();
    }

    if ((pinID != OPGP_GLOBAL_PIN_ID_PW1_81) && (pinID != OPGP_GLOBAL_PIN_ID_PW3))
    {
        opgpHalFatalError();
    }

    opgpHalGetPinLength(pinID, &correctOldPinLength);

    if (correctOldPinLength > inputLength)
    {
        oldPinLength = inputLength;
    }
    else
    {
        oldPinLength = correctOldPinLength;
    }

    calleeRetVal = opgpPinVerifyPin(pinID, input, oldPinLength);

    if (calleeRetVal != OPGP_NO_ERROR)
    {
        retVal = calleeRetVal;
        goto END;
    }

    newPinLength = inputLength - correctOldPinLength;

    if (pinID == OPGP_GLOBAL_PIN_ID_PW1_81)
    {
        newPinMinimumLength = OPGP_GLOBAL_PW1_MINIMUM_LENGTH;
    }
    else if (pinID == OPGP_GLOBAL_PIN_ID_PW3)
    {
        newPinMinimumLength = OPGP_GLOBAL_PW3_MINIMUM_LENGTH;
    }

    if (newPinLength < newPinMinimumLength)
    {
        retVal = OPGP_INVALID_PIN_LENGTH_ERROR;
        goto END;
    }

    if (newPinLength > OPGP_GLOBAL_MAX_PIN_LENGTH)
    {
        retVal = OPGP_INVALID_PIN_LENGTH_ERROR;
        goto END;
    }

    opgpHalCalculatePinHash(&input[correctOldPinLength], newPinLength, newPinHash);

    opgpHalSetPinHashAndLength(pinID, newPinHash, newPinLength);

    retVal = OPGP_NO_ERROR;

END:

    return retVal;
}

uint16_t opgpPinUnblockPW1WithRC(uint8_t* input, uint32_t inputLength)
{
    uint16_t retVal = OPGP_GENERAL_ERROR;
    uint16_t calleeRetVal = OPGP_GENERAL_ERROR;
    uint32_t correctRCLength;
    uint32_t rcLength;
    uint32_t newPinLength;
    uint8_t newPinHash[OPGP_GLOBAL_PIN_HASH_LENGTH];

    if (input == NULL)
    {
        opgpHalFatalError();
    }

    opgpHalGetPinLength(OPGP_GLOBAL_PIN_ID_RC, &correctRCLength);

    if (correctRCLength > inputLength)
    {
        rcLength = inputLength;
    }
    else
    {
        rcLength = correctRCLength;
    }

    calleeRetVal = opgpPinVerifyPin(OPGP_GLOBAL_PIN_ID_RC, input, rcLength);

    if (calleeRetVal != OPGP_NO_ERROR)
    {
        retVal = calleeRetVal;
        goto END;
    }

    newPinLength = inputLength - correctRCLength;

    if (newPinLength < OPGP_GLOBAL_PW1_MINIMUM_LENGTH)
    {
        retVal = OPGP_INVALID_PIN_LENGTH_ERROR;
        goto END;
    }

    if (newPinLength > OPGP_GLOBAL_MAX_PIN_LENGTH)
    {
        retVal = OPGP_INVALID_PIN_LENGTH_ERROR;
        goto END;
    }

    opgpHalCalculatePinHash(&input[correctRCLength], newPinLength, newPinHash);

    opgpHalSetPW1HashAndLengthAndUnblock(newPinHash, newPinLength);

    retVal = OPGP_NO_ERROR;

END:

    return retVal;
}

uint16_t opgpPinUnblockPW1(uint8_t* input, uint32_t inputLength)
{
    uint16_t retVal = OPGP_GENERAL_ERROR;
    uint8_t newPinHash[OPGP_GLOBAL_PIN_HASH_LENGTH];

    if (input == NULL)
    {
        opgpHalFatalError();
    }

    if (opgpPinIsPW3Verified() != OPGP_TRUE)
    {
        opgpHalFatalError();
    }

    if (inputLength < OPGP_GLOBAL_PW1_MINIMUM_LENGTH)
    {
        retVal = OPGP_INVALID_PIN_LENGTH_ERROR;
        goto END;
    }

    if (inputLength > OPGP_GLOBAL_MAX_PIN_LENGTH)
    {
        retVal = OPGP_INVALID_PIN_LENGTH_ERROR;
        goto END;
    }

    opgpHalCalculatePinHash(input, inputLength, newPinHash);

    opgpHalSetPW1HashAndLengthAndUnblock(newPinHash, inputLength);

    retVal = OPGP_NO_ERROR;

END:

    return retVal;
}

uint16_t opgpPinSetResettingCode(uint8_t* input, uint32_t inputLength)
{
    uint16_t retVal = OPGP_GENERAL_ERROR;
    uint8_t rcHash[OPGP_GLOBAL_PIN_HASH_LENGTH];

    if (input == NULL)
    {
        opgpHalFatalError();
    }

    if (opgpPinIsPW3Verified() != OPGP_TRUE)
    {
        opgpHalFatalError();
    }

    if (inputLength == 0)
    {
        opgpHalCalculatePinHash(input, 0x00, rcHash);

        opgpHalSetRCHashAndLengthAndSetErrorCounter(rcHash, 0, OPGP_GLOBAL_PIN_BLOCKED_ERROR_COUNTER_VALUE);
    }
    else
    {
        if (inputLength < OPGP_GLOBAL_PW1_MINIMUM_LENGTH)
        {
            retVal = OPGP_INVALID_PIN_LENGTH_ERROR;
            goto END;
        }

        if (inputLength > OPGP_GLOBAL_MAX_PIN_LENGTH)
        {
            retVal = OPGP_INVALID_PIN_LENGTH_ERROR;
            goto END;
        }

        opgpHalCalculatePinHash(input, inputLength, rcHash);

        opgpHalSetRCHashAndLengthAndSetErrorCounter(rcHash, inputLength, OPGP_GLOBAL_PIN_INITIAL_ERROR_COUNTER_VALUE);
    }

    retVal = OPGP_NO_ERROR;

END:

    return retVal;
}

static void opgpPinSetPinStatus(uint8_t pinID, uint16_t pinStatus)
{
    if (pinID == OPGP_GLOBAL_PIN_ID_PW1_81)
    {
        opgpCoreVolatilePinStatus.PW1_81_Verified = pinStatus;
    }
    else if (pinID == OPGP_GLOBAL_PIN_ID_PW1_82)
    {
        opgpCoreVolatilePinStatus.PW1_82_Verified = pinStatus;
    }
    else if (pinID == OPGP_GLOBAL_PIN_ID_PW3)
    {
        opgpCoreVolatilePinStatus.PW3Verified = pinStatus;
    }
}
