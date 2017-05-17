/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __MK82_USB_INT_H__
#define __MK82_USB_INT_H__

#include "stdint.h"

#define MK82_USB_DEVICE_INTERRUPT_PRIORITY (0U)
#define MK82_PIT_WTX_INTERRUPT_PRIORITY (1U)
#define MK82_PIT_U2F_INTERRUPT_PRIORITY (1U)

#define MK82_USB_MAX_HISTCHARS_LENGTH (15)

#define MK82_USB_EVENT_CCID_PACKET_RECEIVED (0x9999)
#define MK82_USB_EVENT_U2F_PACKET_RECEIVED (0x6666)
#define MK82_USB_EVENT_U2F_TIMER_EXPIRED (0xCCCC)
#define MK82_USB_EVENT_BTC_PACKET_RECEIVED (0x3333)
#define MK82_USB_EVENT_NOTHING_HAPPENED (0x5555)

MK82_MAKE_PACKED(typedef struct)
{
    uint8_t keyModifier;
    uint8_t reserved;
    uint8_t pressedKeys[6];
}
MK82_USB_KEYBOARD_INPUT_REPORT;

#define MK82_USB_DEVICE_HID_REQUEST_GET_REPORT (0x01)
#define MK82_USB_DEVICE_HID_REQUEST_GET_IDLE (0x02)
#define MK82_USB_DEVICE_HID_REQUEST_GET_PROTOCOL (0x03)
#define MK82_USB_DEVICE_HID_REQUEST_SET_REPORT (0x09)
#define MK82_USB_DEVICE_HID_REQUEST_SET_IDLE (0x0A)
#define MK82_USB_DEVICE_HID_REQUEST_SET_PROTOCOL (0x0B)

#endif /* __MK82_USB_INT_H__ */
