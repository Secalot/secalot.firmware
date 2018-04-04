/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <apduGlobal.h>
#include <apduGlobalInt.h>
#include <apduCore.h>
#include <core/apduCoreInt.h>

#include <ccidGlobal.h>

void apduCoreInit() {}

void apduCoreDeinit() {}

uint16_t apduCoreParseIncomingAPDU(uint8_t* apdu, uint32_t apduLength, APDU_CORE_COMMAND_APDU* parsedAPDU)
{
    uint16_t retVal = APDU_GENERAL_ERROR;

    if (apduLength < 0x04)
    {
        retVal = APDU_GENERAL_ERROR;
        goto END;
    }

    if (apduLength == 0x04)
    {
        parsedAPDU->lc = 0x00;
        parsedAPDU->lcPresent = APDU_FALSE;
        parsedAPDU->le = 0x00;
        parsedAPDU->lePresent = APDU_FALSE;
        parsedAPDU->data = &apdu[APDU_CORE_OFFSET_DATA];
    }
    else if (apduLength == 0x05)
    {
        parsedAPDU->lc = 0x00;
        parsedAPDU->lcPresent = APDU_FALSE;
        parsedAPDU->le = apdu[APDU_CORE_OFFSET_LC_OR_LE];
        parsedAPDU->lePresent = APDU_TRUE;
        parsedAPDU->data = &apdu[APDU_CORE_OFFSET_DATA];
    }
    else if ((apduLength == (0x05 + apdu[APDU_CORE_OFFSET_LC_OR_LE])) && (apdu[APDU_CORE_OFFSET_LC_OR_LE] != 0x00))
    {
        parsedAPDU->lc = apdu[APDU_CORE_OFFSET_LC_OR_LE];
        parsedAPDU->lcPresent = APDU_TRUE;
        parsedAPDU->le = 0x00;
        parsedAPDU->lePresent = APDU_FALSE;
        parsedAPDU->data = &apdu[APDU_CORE_OFFSET_DATA];
    }
    else if (apduLength == (0x06 + apdu[APDU_CORE_OFFSET_LC_OR_LE]) && (apdu[APDU_CORE_OFFSET_LC_OR_LE] != 0x00))
    {
        parsedAPDU->lc = apdu[APDU_CORE_OFFSET_LC_OR_LE];
        parsedAPDU->lcPresent = APDU_TRUE;
        parsedAPDU->le = apdu[apduLength - 1];
        parsedAPDU->lePresent = APDU_TRUE;
        parsedAPDU->data = &apdu[APDU_CORE_OFFSET_DATA];
    }
    else if ((apduLength == 0x07) && (apdu[APDU_CORE_OFFSET_EXTENDED_LENGTH_LC_OR_LE1] == 0x00))
    {
        parsedAPDU->lc = 0x00;
        parsedAPDU->lcPresent = APDU_FALSE;
        parsedAPDU->le = APDU_MAKEWORD(apdu[APDU_CORE_OFFSET_EXTENDED_LENGTH_LC_OR_LE3],
                                       apdu[APDU_CORE_OFFSET_EXTENDED_LENGTH_LC_OR_LE2]);
        parsedAPDU->lePresent = APDU_TRUE;
        parsedAPDU->data = &apdu[APDU_CORE_OFFSET_EXTENDED_LENGTH_DATA];
    }
    else if ((apduLength == (0x07 + APDU_MAKEWORD(apdu[APDU_CORE_OFFSET_EXTENDED_LENGTH_LC_OR_LE3],
                                                  apdu[APDU_CORE_OFFSET_EXTENDED_LENGTH_LC_OR_LE2]))) &&
             (apdu[APDU_CORE_OFFSET_EXTENDED_LENGTH_LC_OR_LE1] == 0x00))
    {
        parsedAPDU->lc = APDU_MAKEWORD(apdu[APDU_CORE_OFFSET_EXTENDED_LENGTH_LC_OR_LE3],
                                       apdu[APDU_CORE_OFFSET_EXTENDED_LENGTH_LC_OR_LE2]);
        parsedAPDU->lcPresent = APDU_TRUE;
        parsedAPDU->le = 0x00;
        parsedAPDU->lePresent = APDU_FALSE;
        parsedAPDU->data = &apdu[APDU_CORE_OFFSET_EXTENDED_LENGTH_DATA];
    }
    else if ((apduLength == (0x09 + APDU_MAKEWORD(apdu[APDU_CORE_OFFSET_EXTENDED_LENGTH_LC_OR_LE3],
                                                  apdu[APDU_CORE_OFFSET_EXTENDED_LENGTH_LC_OR_LE2]))) &&
             (apdu[APDU_CORE_OFFSET_EXTENDED_LENGTH_LC_OR_LE1] == 0x00))
    {
        parsedAPDU->lc = APDU_MAKEWORD(apdu[APDU_CORE_OFFSET_EXTENDED_LENGTH_LC_OR_LE3],
                                       apdu[APDU_CORE_OFFSET_EXTENDED_LENGTH_LC_OR_LE2]);
        parsedAPDU->lcPresent = APDU_TRUE;
        parsedAPDU->le = APDU_MAKEWORD(apdu[apduLength - 1], apdu[apduLength - 2]);
        ;
        parsedAPDU->lePresent = APDU_TRUE;
        parsedAPDU->data = &apdu[APDU_CORE_OFFSET_EXTENDED_LENGTH_DATA];
    }
    else
    {
        retVal = APDU_GENERAL_ERROR;
        goto END;
    }

    parsedAPDU->cla = apdu[APDU_CORE_OFFSET_CLA];
    parsedAPDU->ins = apdu[APDU_CORE_OFFSET_INS];
    parsedAPDU->p1p2 = APDU_MAKEWORD(apdu[APDU_CORE_OFFSET_P2], apdu[APDU_CORE_OFFSET_P1]);

    retVal = APDU_NO_ERROR;

END:
    return retVal;
}

void apduCorePrepareResponseAPDUStructure(uint8_t* apdu, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    responseAPDU->data = apdu;
    responseAPDU->dataLength = 0;
    responseAPDU->sw = APDU_CORE_SW_UNKNOWN;
    responseAPDU->maxLength = CCID_MAX_APDU_DATA_SIZE;
}

void apduCorePrepareOutgoingAPDU(uint8_t* apdu, uint32_t* apduLength, APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    apdu[responseAPDU->dataLength] = APDU_HIBYTE(responseAPDU->sw);
    apdu[responseAPDU->dataLength + 1] = APDU_LOBYTE(responseAPDU->sw);

    *apduLength = responseAPDU->dataLength + 2;
}
