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
#include "btcBase58Int.h"
#include "btcBase58.h"

#include "libbase58.h"

static bool btcBase58Libbase58Sha256Implementation(void* hash, const void* data, size_t dataLength);

void btcBase58Init(void) { b58_sha256_impl = btcBase58Libbase58Sha256Implementation; }

void btcBase58Deinit(void) {}

static bool btcBase58Libbase58Sha256Implementation(void* hash, const void* data, size_t dataLength)
{
    btcHalSha256((uint8_t*)data, dataLength, (uint8_t*)hash);

    return true;
}

void btcBase58EncodeBitcoinAddress(uint8_t* data, uint32_t dataLength, uint8_t* encodedData,
                                   uint32_t* encodedDataLength, uint8_t version)
{
    bool calleeRetVal = false;

    if ((data == NULL) || (encodedData == NULL) || (encodedDataLength == NULL))
    {
        btcHalFatalError();
    }

    calleeRetVal = b58check_enc((char*)encodedData, (size_t*)encodedDataLength, version, data, dataLength);

    if (calleeRetVal != true)
    {
        btcHalFatalError();
    }
}

uint16_t btcBase58DecodeAndCheckBitcoinAddress(uint8_t* data, uint32_t dataLength, uint8_t* decodedData,
                                               uint8_t version)
{
    uint8_t addressWithTypeAndChecksum[BTC_BASE58_CHECK_ADDRESS_LENGTH];
    uint32_t addressWithTypeAndChecksumLength;
    bool boolCalleeRetVal = false;
    bool intCalleeRetVal = -1;
    uint16_t retVal = BTC_GENERAL_ERROR;

    if ((data == NULL) || (decodedData == NULL))
    {
        btcHalFatalError();
    }

    addressWithTypeAndChecksumLength = sizeof(addressWithTypeAndChecksum);

    boolCalleeRetVal =
        b58tobin(addressWithTypeAndChecksum, (size_t*)&addressWithTypeAndChecksumLength, (const char*)data, dataLength);

    if (boolCalleeRetVal != true)
    {
        retVal = BTC_INVALID_INPUT_ERROR;
        goto END;
    }

    if (addressWithTypeAndChecksumLength != BTC_BASE58_CHECK_ADDRESS_LENGTH)
    {
        retVal = BTC_INVALID_INPUT_ERROR;
        goto END;
    }

    if (addressWithTypeAndChecksum[BTC_BASE58_CHECK_ADDRESS_VERSION_OFFSET] != version)
    {
        retVal = BTC_INVALID_INPUT_ERROR;
        goto END;
    }

    intCalleeRetVal =
        b58check(addressWithTypeAndChecksum, addressWithTypeAndChecksumLength, (const char*)data, dataLength);

    if (intCalleeRetVal != 0)
    {
        retVal = BTC_INVALID_INPUT_ERROR;
        goto END;
    }

    btcHalMemCpy(decodedData, &addressWithTypeAndChecksum[BTC_BASE58_CHECK_ADDRESS_ADDRESS_OFFSET],
                 BTC_GLOBAL_RIPEMD160_SIZE);

    retVal = BTC_NO_ERROR;

END:
    return retVal;
}
