/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __BTC_BASE58_H__
#define __BTC_BASE58_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

void btcBase58Init(void);
void btcBase58Deinit(void);

void btcBase58EncodeBitcoinAddress(uint8_t* data, uint32_t dataLength, uint8_t* encodedData,
                                   uint32_t* encodedDataLength, uint8_t version);
uint16_t btcBase58DecodeAndCheckBitcoinAddress(uint8_t* data, uint32_t dataLength, uint8_t* decodedData,
                                               uint8_t version);

#ifdef __cplusplus
}
#endif

#endif /* __BTC_BASE58_H__ */
