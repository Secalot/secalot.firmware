/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SF_HID_REPORT_DESCRIPTOR_H__
#define __SF_HID_REPORT_DESCRIPTOR_H__

#include <sfGlobal.h>

#define SF_HID_INPUT_REPORT_BYTES 64
#define SF_HID_OUTPUT_REPORT_BYTES 64

#define SF_HID_DATA_MASK 0x00
#define SF_HID_VARIABLE_MASK 0x02
#define SF_HID_ABSOLUTE_MASK 0x00

#define SF_HID_FIDO_USAGE_PAGE 0xf1d0
#define SF_HID_FIDO_USAGE_U2FHID 0x01
#define SF_HID_APPLICATION 0x01
#define SF_HID_FIDO_USAGE_DATA_IN 0x20
#define SF_HID_FIDO_USAGE_DATA_OUT 0x21

#define SF_HID_USAGE_PAGE(value) 0x06, SF_LOBYTE(value), SF_HIBYTE(value)
#define SF_HID_USAGE(value) 0x09, value
#define SF_HID_COLLECTION(value) 0xA1, value
#define SF_HID_LOGICALMIN(value) 0x15, value
#define SF_HID_LOGICALMAXS(value) 0x26, SF_LOBYTE(value), SF_HIBYTE(value)
#define SF_HID_REPORT_SIZE(value) 0x75, value
#define SF_HID_RECORDCOUNT(value) 0x95, value
#define SF_HID_INPUT(value) 0x81, value
#define SF_HID_OUTPUT(value) 0x91, value
#define SF_HID_END_COLLECTION 0xC0

#define SF_REPORT_DESCRIPTOR                                                                      \
    SF_HID_USAGE_PAGE(SF_HID_FIDO_USAGE_PAGE)                                                     \
    , SF_HID_USAGE(SF_HID_FIDO_USAGE_U2FHID), SF_HID_COLLECTION(SF_HID_APPLICATION),              \
        SF_HID_USAGE(SF_HID_FIDO_USAGE_DATA_IN), SF_HID_LOGICALMIN(0), SF_HID_LOGICALMAXS(0xff),  \
        SF_HID_REPORT_SIZE(8), SF_HID_RECORDCOUNT(SF_HID_INPUT_REPORT_BYTES),                     \
        SF_HID_INPUT(SF_HID_DATA_MASK | SF_HID_VARIABLE_MASK | SF_HID_ABSOLUTE_MASK),             \
        SF_HID_USAGE(SF_HID_FIDO_USAGE_DATA_OUT), SF_HID_LOGICALMIN(0), SF_HID_LOGICALMAXS(0xff), \
        SF_HID_REPORT_SIZE(8), SF_HID_RECORDCOUNT(SF_HID_OUTPUT_REPORT_BYTES),                    \
        SF_HID_OUTPUT(SF_HID_DATA_MASK | SF_HID_VARIABLE_MASK | SF_HID_ABSOLUTE_MASK), SF_HID_END_COLLECTION

#endif /* __SF_HID_REPORT_DESCRIPTOR_H__ */
