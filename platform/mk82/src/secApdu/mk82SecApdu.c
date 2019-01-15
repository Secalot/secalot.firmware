/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "mk82Global.h"
#include "mk82GlobalInt.h"
#include "mk82System.h"
#include "mk82SecApdu.h"
#include "mk82SecApduInt.h"

#include "mk82Usb.h"
#include "mk82As.h"
#include "mk82Ssl.h"

static uint16_t mk82SecApduPrimaryDataType;

void mk82SecApduInit(void) {}

void mk82SecApduSetPrimaryDataType(uint16_t primaryDataType) { mk82SecApduPrimaryDataType = primaryDataType; }

uint16_t mk82SecApduGetPrimaryDataType(void) { return mk82SecApduPrimaryDataType; }

void mk82SecApduProcessCommandIfAvailable(uint32_t dataTypesToProcess, uint32_t allowedCommands)
{
    uint8_t* data;
    uint32_t dataLength;
    uint16_t dataType;
    uint16_t newUsbCommandReceived;

    newUsbCommandReceived = mk82UsbCheckForNewCommand(dataTypesToProcess, &data, &dataLength, &dataType);

    if (newUsbCommandReceived == MK82_USB_COMMAND_RECEIVED)
    {
        uint16_t sslStatus;

        mk82SslUnwrapAPDUCommand(data, &dataLength, &sslStatus);

        if ((sslStatus == MK82_SSL_STATUS_UNWRAPPED) || (sslStatus == MK82_SSL_STATUS_NOT_SSL))
        {
            mk82AsProcessAPDU(data, &dataLength, allowedCommands);
        }
        else
        {
            if (sslStatus != MK82_SSL_STATUS_ERROR_OCCURED)
            {
                mk82SystemFatalError();
            }
        }

        if (sslStatus == MK82_SSL_STATUS_UNWRAPPED)
        {
            mk82SslWrapAPDUResponse(data, &dataLength);
        }

        mk82UsbSendResponse(dataLength, dataType);
    }
}
