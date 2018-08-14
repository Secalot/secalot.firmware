/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdint.h>

#include "xrpGlobal.h"
#include "xrpGlobalInt.h"
#include "xrpHal.h"
#include "xrpPinInt.h"
#include "xrpPin.h"

static uint16_t xrpPinVerified = XRP_FALSE;

static void xrpPinSetVolatilePinStatus(uint16_t pinStatus);

void xrpPinInit(void) { xrpPinSetVolatilePinStatus(XRP_FALSE); }

void xrpPinDeinit(void) { xrpPinSetVolatilePinStatus(XRP_FALSE); }

static void xrpPinSetVolatilePinStatus(uint16_t pinStatus) { xrpPinVerified = pinStatus; }

uint16_t xrpPinIsPinVerified() { return xrpPinVerified; }

uint16_t xrpPinIsPinBlocked(void)
{
    uint8_t errorCounter;
    uint16_t retVal = XRP_TRUE;

    xrpHalGetPinErrorCounter(&errorCounter);

    if (errorCounter == XRP_GLOBAL_PIN_BLOCKED_ERROR_COUNTER_VALUE)
    {
        retVal = XRP_TRUE;
    }
    else
    {
        retVal = XRP_FALSE;
    }

    return retVal;
}

uint16_t xrpPinVerifyPin(uint8_t* input, uint32_t inputLength)
{
    uint16_t retVal = XRP_GENERAL_ERROR;
    uint8_t errorCounter;
    uint16_t comparisonResult = XRP_CMP_NOT_EQUAL;
    uint8_t hashBuffer1[XRP_GLOBAL_PIN_HASH_LENGTH];
    uint8_t hashBuffer2[XRP_GLOBAL_PIN_HASH_LENGTH];

    if (input == NULL)
    {
        xrpHalFatalError();
    }

    xrpHalGetPinErrorCounter(&errorCounter);

    if (inputLength < XRP_GLOBAL_PIN_MINIMUM_LENGTH)
    {
        retVal = XRP_INVALID_PIN_LENGTH_ERROR;
        goto END;
    }

    if (inputLength > XRP_GLOBAL_PIN_MAXIMUM_LENGTH)
    {
        retVal = XRP_INVALID_PIN_LENGTH_ERROR;
        goto END;
    }

    if (errorCounter == XRP_GLOBAL_PIN_BLOCKED_ERROR_COUNTER_VALUE)
    {
        retVal = XRP_PIN_BLOCKED_ERROR;
        goto END;
    }

    errorCounter--;

    xrpHalSetPinErrorCounter(errorCounter);

    xrpHalGetPinHash(hashBuffer1);

    xrpHalComputePinHash(input, inputLength, hashBuffer2);

    comparisonResult = xrpHalMemCmp(hashBuffer1, hashBuffer2, XRP_GLOBAL_PIN_HASH_LENGTH);

    if (comparisonResult != XRP_CMP_EQUAL)
    {
        xrpPinSetVolatilePinStatus(XRP_FALSE);

        if (errorCounter == XRP_GLOBAL_PIN_BLOCKED_ERROR_COUNTER_VALUE)
        {
            xrpHalWipeout();
        }

        retVal = XRP_INVALID_PIN_ERROR;
        goto END;
    }
    else
    {
        errorCounter = XRP_GLOBAL_PIN_INITIAL_ERROR_COUNTER_VALUE;

        xrpHalSetPinErrorCounter(errorCounter);

        xrpPinSetVolatilePinStatus(XRP_TRUE);
    }

    retVal = XRP_NO_ERROR;

END:

    return retVal;
}

uint16_t xrpPinCheckNewPinAndGetPinHash(uint8_t* input, uint32_t inputLength, uint8_t* pinHash)
{
    uint16_t retVal = XRP_GENERAL_ERROR;

    if ((input == NULL) || (pinHash == NULL))
    {
        xrpHalFatalError();
    }

    if (inputLength < XRP_GLOBAL_PIN_MINIMUM_LENGTH)
    {
        retVal = XRP_INVALID_PIN_LENGTH_ERROR;
        goto END;
    }

    if (inputLength > XRP_GLOBAL_PIN_MAXIMUM_LENGTH)
    {
        retVal = XRP_INVALID_PIN_LENGTH_ERROR;
        goto END;
    }

    xrpHalComputePinHash(input, inputLength, pinHash);

    retVal = XRP_NO_ERROR;

END:

    return retVal;
}
