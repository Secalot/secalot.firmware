/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __BTC_HID_REPORT_DESCRIPTOR_H__
#define __BTC_HID_REPORT_DESCRIPTOR_H__

#include <btcGlobal.h>

#define BTC_HID_INPUT_REPORT_BYTES 64
#define BTC_HID_OUTPUT_REPORT_BYTES 64

#define BTC_HID_DATA_MASK 0x00
#define BTC_HID_VARIABLE_MASK 0x02
#define BTC_HID_ABSOLUTE_MASK 0x00

#define BTC_HID_WALLET_USAGE_PAGE 0xff00
#define BTC_HID_WALLET_USAGE 0x01
#define BTC_HID_APPLICATION 0x01
#define BTC_HID_WALLET_USAGE_DATA_IN 0x20
#define BTC_HID_WALLET_USAGE_DATA_OUT 0x21

#define BTC_HID_USAGE_PAGE(value) 0x06, BTC_LOBYTE(value), BTC_HIBYTE(value)
#define BTC_HID_USAGE(value) 0x09, value
#define BTC_HID_COLLECTION(value) 0xA1, value
#define BTC_HID_LOGICALMIN(value) 0x15, value
#define BTC_HID_LOGICALMAXS(value) 0x26, BTC_LOBYTE(value), BTC_HIBYTE(value)
#define BTC_HID_REPORT_SIZE(value) 0x75, value
#define BTC_HID_RECORDCOUNT(value) 0x95, value
#define BTC_HID_INPUT(value) 0x81, value
#define BTC_HID_OUTPUT(value) 0x91, value
#define BTC_HID_END_COLLECTION 0xC0

#define BTC_REPORT_DESCRIPTOR                                                                           \
    BTC_HID_USAGE_PAGE(BTC_HID_WALLET_USAGE_PAGE)                                                       \
    , BTC_HID_USAGE(BTC_HID_WALLET_USAGE), BTC_HID_COLLECTION(BTC_HID_APPLICATION),                     \
        BTC_HID_USAGE(BTC_HID_WALLET_USAGE_DATA_IN), BTC_HID_LOGICALMIN(0), BTC_HID_LOGICALMAXS(0xff),  \
        BTC_HID_REPORT_SIZE(8), BTC_HID_RECORDCOUNT(BTC_HID_INPUT_REPORT_BYTES),                        \
        BTC_HID_INPUT(BTC_HID_DATA_MASK | BTC_HID_VARIABLE_MASK | BTC_HID_ABSOLUTE_MASK),               \
        BTC_HID_USAGE(BTC_HID_WALLET_USAGE_DATA_OUT), BTC_HID_LOGICALMIN(0), BTC_HID_LOGICALMAXS(0xff), \
        BTC_HID_REPORT_SIZE(8), BTC_HID_RECORDCOUNT(BTC_HID_OUTPUT_REPORT_BYTES),                       \
        BTC_HID_OUTPUT(BTC_HID_DATA_MASK | BTC_HID_VARIABLE_MASK | BTC_HID_ABSOLUTE_MASK), BTC_HID_END_COLLECTION

#endif /* __BTC_HID_REPORT_DESCRIPTOR_H__ */
