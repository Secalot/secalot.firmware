/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdint.h>

#include "ethGlobal.h"
#include "ethGlobalInt.h"
#include "ethHal.h"
#include "ethPinInt.h"
#include "ethPin.h"

static uint16_t ethPinVerified = ETH_FALSE;

static void ethPinSetVolatilePinStatus(uint16_t pinStatus);

void ethPinInit(void) { ethPinSetVolatilePinStatus(ETH_FALSE); }

void ethPinDeinit(void) { ethPinSetVolatilePinStatus(ETH_FALSE); }

static void ethPinSetVolatilePinStatus(uint16_t pinStatus) { ethPinVerified = pinStatus; }

uint16_t ethPinIsPinVerified() { return ethPinVerified; }

uint16_t ethPinIsPinBlocked(void)
{
    uint8_t errorCounter;
    uint16_t retVal = ETH_TRUE;

    ethHalGetPinErrorCounter(&errorCounter);

    if (errorCounter == ETH_GLOBAL_PIN_BLOCKED_ERROR_COUNTER_VALUE)
    {
        retVal = ETH_TRUE;
    }
    else
    {
        retVal = ETH_FALSE;
    }

    return retVal;
}

uint16_t ethPinVerifyPin(uint8_t* input, uint32_t inputLength)
{
    uint16_t retVal = ETH_GENERAL_ERROR;
    uint8_t errorCounter;
    uint16_t comparisonResult = ETH_CMP_NOT_EQUAL;
    uint8_t hashBuffer1[ETH_GLOBAL_PIN_HASH_LENGTH];
    uint8_t hashBuffer2[ETH_GLOBAL_PIN_HASH_LENGTH];

    if (input == NULL)
    {
        ethHalFatalError();
    }

    ethHalGetPinErrorCounter(&errorCounter);

    if (inputLength < ETH_GLOBAL_PIN_MINIMUM_LENGTH)
    {
        retVal = ETH_INVALID_PIN_LENGTH_ERROR;
        goto END;
    }

    if (inputLength > ETH_GLOBAL_PIN_MAXIMUM_LENGTH)
    {
        retVal = ETH_INVALID_PIN_LENGTH_ERROR;
        goto END;
    }

    if (errorCounter == ETH_GLOBAL_PIN_BLOCKED_ERROR_COUNTER_VALUE)
    {
        retVal = ETH_PIN_BLOCKED_ERROR;
        goto END;
    }

    errorCounter--;

    ethHalSetPinErrorCounter(errorCounter);

    ethHalGetPinHash(hashBuffer1);

    ethHalComputePinHash(input, inputLength, hashBuffer2);

    comparisonResult = ethHalMemCmp(hashBuffer1, hashBuffer2, ETH_GLOBAL_PIN_HASH_LENGTH);

    if (comparisonResult != ETH_CMP_EQUAL)
    {
        ethPinSetVolatilePinStatus(ETH_FALSE);

        if (errorCounter == ETH_GLOBAL_PIN_BLOCKED_ERROR_COUNTER_VALUE)
        {
            ethHalWipeout();
        }

        retVal = ETH_INVALID_PIN_ERROR;
        goto END;
    }
    else
    {
        errorCounter = ETH_GLOBAL_PIN_INITIAL_ERROR_COUNTER_VALUE;

        ethHalSetPinErrorCounter(errorCounter);

        ethPinSetVolatilePinStatus(ETH_TRUE);
    }

    retVal = ETH_NO_ERROR;

END:

    return retVal;
}

uint16_t ethPinCheckNewPinAndGetPinHash(uint8_t* input, uint32_t inputLength, uint8_t* pinHash)
{
    uint16_t retVal = ETH_GENERAL_ERROR;

    if ((input == NULL) || (pinHash == NULL))
    {
        ethHalFatalError();
    }

    if (inputLength < ETH_GLOBAL_PIN_MINIMUM_LENGTH)
    {
        retVal = ETH_INVALID_PIN_LENGTH_ERROR;
        goto END;
    }

    if (inputLength > ETH_GLOBAL_PIN_MAXIMUM_LENGTH)
    {
        retVal = ETH_INVALID_PIN_LENGTH_ERROR;
        goto END;
    }

    ethHalComputePinHash(input, inputLength, pinHash);

    retVal = ETH_NO_ERROR;

END:

    return retVal;
}
