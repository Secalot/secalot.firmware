/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdint.h>

#include "btcGlobal.h"
#include "btcGlobalInt.h"
#include "btcHal.h"
#include "btcPinInt.h"
#include "btcPin.h"

static uint16_t btcPinVerified = BTC_FALSE;

static void btcPinSetVolatilePinStatus(uint16_t pinStatus);

void btcPinInit(void) { btcPinSetVolatilePinStatus(BTC_FALSE); }

void btcPinDeinit(void) { btcPinSetVolatilePinStatus(BTC_FALSE); }

static void btcPinSetVolatilePinStatus(uint16_t pinStatus) { btcPinVerified = pinStatus; }

uint16_t btcPinIsPinVerified() { return btcPinVerified; }

uint16_t btcPinIsPinBlocked(void)
{
    uint8_t errorCounter;
    uint16_t retVal = BTC_TRUE;

    btcHalGetPinErrorCounter(&errorCounter);

    if (errorCounter == BTC_GLOBAL_PIN_BLOCKED_ERROR_COUNTER_VALUE)
    {
        retVal = BTC_TRUE;
    }
    else
    {
        retVal = BTC_FALSE;
    }

    return retVal;
}

uint16_t btcPinVerifyPin(uint8_t* input, uint32_t inputLength)
{
    uint16_t retVal = BTC_GENERAL_ERROR;
    uint8_t errorCounter;
    uint16_t comparisonResult = BTC_CMP_NOT_EQUAL;
    uint8_t hashBuffer1[BTC_GLOBAL_PIN_HASH_LENGTH];
    uint8_t hashBuffer2[BTC_GLOBAL_PIN_HASH_LENGTH];

    if (input == NULL)
    {
        btcHalFatalError();
    }

    btcHalGetPinErrorCounter(&errorCounter);

    if (inputLength < BTC_GLOBAL_PIN_MINIMUM_LENGTH)
    {
        retVal = BTC_INVALID_PIN_LENGTH_ERROR;
        goto END;
    }

    if (inputLength > BTC_GLOBAL_PIN_MAXIMUM_LENGTH)
    {
        retVal = BTC_INVALID_PIN_LENGTH_ERROR;
        goto END;
    }

    if (errorCounter == BTC_GLOBAL_PIN_BLOCKED_ERROR_COUNTER_VALUE)
    {
        retVal = BTC_PIN_BLOCKED_ERROR;
        goto END;
    }

    errorCounter--;

    btcHalSetPinErrorCounter(errorCounter);

    btcHalGetPinHash(hashBuffer1);

    btcHalComputePinHash(input, inputLength, hashBuffer2);

    comparisonResult = btcHalMemCmp(hashBuffer1, hashBuffer2, BTC_GLOBAL_PIN_HASH_LENGTH);

    if (comparisonResult != BTC_CMP_EQUAL)
    {
        btcPinSetVolatilePinStatus(BTC_FALSE);

        if (errorCounter == BTC_GLOBAL_PIN_BLOCKED_ERROR_COUNTER_VALUE)
        {
            btcHalWipeout();
        }

        retVal = BTC_INVALID_PIN_ERROR;
        goto END;
    }
    else
    {
        errorCounter = BTC_GLOBAL_PIN_INITIAL_ERROR_COUNTER_VALUE;

        btcHalSetPinErrorCounter(errorCounter);

        btcPinSetVolatilePinStatus(BTC_TRUE);
    }

    retVal = BTC_NO_ERROR;

END:

    return retVal;
}

uint16_t btcPinCheckNewPinAndGetPinHash(uint8_t* input, uint32_t inputLength, uint8_t* pinHash)
{
    uint16_t retVal = BTC_GENERAL_ERROR;

    if ((input == NULL) || (pinHash == NULL))
    {
        btcHalFatalError();
    }

    if (inputLength < BTC_GLOBAL_PIN_MINIMUM_LENGTH)
    {
        retVal = BTC_INVALID_PIN_LENGTH_ERROR;
        goto END;
    }

    if (inputLength > BTC_GLOBAL_PIN_MAXIMUM_LENGTH)
    {
        retVal = BTC_INVALID_PIN_LENGTH_ERROR;
        goto END;
    }

    btcHalComputePinHash(input, inputLength, pinHash);

    retVal = BTC_NO_ERROR;

END:

    return retVal;
}
