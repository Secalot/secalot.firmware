/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "mk82Global.h"
#include "mk82GlobalInt.h"
#include "mk82AsInt.h"
#include "mk82As.h"

#include "stdint.h"

#include "mk82System.h"

#include "opgpGlobal.h"
#include "opgpCore.h"

#include "otpGlobal.h"
#include "otpCore.h"

#include "bldrGlobal.h"
#include "bldrCore.h"

static void mk82AsFatalError(void);
static void mk82AsPutSWToAPDUBuffer(uint8_t* apdu, uint32_t* apduLength, uint16_t sw);

static uint8_t mk82AsOPGPAid[MK82_AS_MAX_AID_LENGTH];
static uint32_t mk82AsOPGPAidLength;
static uint8_t mk82AsOTPAid[MK82_AS_MAX_AID_LENGTH];
static uint32_t mk82AsOTPAidLength;
static uint8_t mk82AsBldrAid[MK82_AS_MAX_AID_LENGTH];
static uint32_t mk82AsBldrAidLength;
static uint32_t mk82AsSelectedApplication = MK82_AS_NONE_SELECTED;

static void mk82AsFatalError(void) { mk82SystemFatalError(); }

void mk82AsInit(void)
{
    opgpCoreGetAID(mk82AsOPGPAid, &mk82AsOPGPAidLength);
    otpCoreGetAID(mk82AsOTPAid, &mk82AsOTPAidLength);
    bldrCoreGetAID(mk82AsBldrAid, &mk82AsBldrAidLength);
}

static void mk82AsPutSWToAPDUBuffer(uint8_t* apdu, uint32_t* apduLength, uint16_t sw)
{
    apdu[0] = (uint8_t)(sw >> 8);
    apdu[1] = (uint8_t)sw;
    *apduLength = 2;
}

void mk82AsProcessAPDU(uint8_t* apdu, uint32_t* apduLength)
{
    uint16_t sw = MK82_AS_SW_UNKNOWN;
    uint8_t applicationSelectHeader[] = MK82_AS_APPLICATION_SELECT_HEADER;

    if ((apdu == NULL) || (apduLength == NULL))
    {
        mk82AsFatalError();
    }

    if ((apdu[MK82_AS_OFFSET_LC] != 0x00) &&
        ((*apduLength == (0x05 + apdu[MK82_AS_OFFSET_LC])) || (*apduLength == (0x06 + apdu[MK82_AS_OFFSET_LC]))) &&
        (mk82SystemMemCmp(apdu, applicationSelectHeader, sizeof(applicationSelectHeader)) == MK82_CMP_EQUAL))
    {
        mk82AsSelectedApplication = MK82_AS_NONE_SELECTED;

        if ((apdu[MK82_AS_OFFSET_LC] <= mk82AsOPGPAidLength) &&
            (mk82SystemMemCmp(&apdu[MK82_AS_OFFSET_DATA], mk82AsOPGPAid, apdu[MK82_AS_OFFSET_LC]) == MK82_CMP_EQUAL))
        {
            opgpCoreSelect(&sw);
            mk82AsSelectedApplication = MK82_AS_OPGP_SELECTED;
            mk82AsPutSWToAPDUBuffer(apdu, apduLength, sw);
            goto END;
        }
        else if ((apdu[MK82_AS_OFFSET_LC] <= mk82AsOTPAidLength) &&
                 (mk82SystemMemCmp(&apdu[MK82_AS_OFFSET_DATA], mk82AsOTPAid, apdu[MK82_AS_OFFSET_LC]) ==
                  MK82_CMP_EQUAL))
        {
            mk82AsSelectedApplication = MK82_AS_OTP_SELECTED;
            sw = MK82_AS_SW_NO_ERROR;
            mk82AsPutSWToAPDUBuffer(apdu, apduLength, sw);
            goto END;
        }
        else if ((apdu[MK82_AS_OFFSET_LC] <= mk82AsBldrAidLength) &&
                 (mk82SystemMemCmp(&apdu[MK82_AS_OFFSET_DATA], mk82AsBldrAid, apdu[MK82_AS_OFFSET_LC]) ==
                  MK82_CMP_EQUAL))
        {
            mk82AsSelectedApplication = MK82_AS_BLDR_SELECTED;
            sw = MK82_AS_SW_NO_ERROR;
            mk82AsPutSWToAPDUBuffer(apdu, apduLength, sw);
            goto END;
        }

        else
        {
            mk82AsPutSWToAPDUBuffer(apdu, apduLength, MK82_AS_SW_REF_DATA_NOT_FOUND);
            goto END;
        }
    }
    else
    {
        if (mk82AsSelectedApplication == MK82_AS_OPGP_SELECTED)
        {
            opgpCoreProcessAPDU(apdu, apduLength);
            goto END;
        }
        else if (mk82AsSelectedApplication == MK82_AS_OTP_SELECTED)
        {
            otpCoreProcessControlAPDU(apdu, apduLength);
            goto END;
        }
        else if (mk82AsSelectedApplication == MK82_AS_BLDR_SELECTED)
        {
            bldrCoreProcessAPDU(apdu, apduLength);
            goto END;
        }
        else
        {
            mk82AsPutSWToAPDUBuffer(apdu, apduLength, MK82_AS_SW_UNKNOWN);
            goto END;
        }
    }

END:;
}
