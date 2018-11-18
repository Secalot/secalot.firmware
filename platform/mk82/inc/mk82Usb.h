/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __MK82_USB_H__
#define __MK82_USB_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MK82_USB_COMMAND_RECEIVED (0x9999)
#define MK82_USB_COMMAND_NOT_RECEIVED (0x6666)




void mk82UsbInit(void);
uint16_t mk82UsbCheckForNewCommand(uint32_t dataTypesToProcess, uint8_t** data, uint32_t* dataLength, uint16_t* dataType);
void mk82UsbSendResponse(uint32_t dataLength, uint16_t dataType);

#ifdef FIRMWARE
void mk82UsbTypeStringWithAKeyboard(uint8_t* stringToType, uint32_t stringLength);
void mk82UsbFakeU2fWtx(void);
#endif /* FIRMWARE */

#ifdef __cplusplus
}
#endif

#endif /* __MK82_USB_H__ */
