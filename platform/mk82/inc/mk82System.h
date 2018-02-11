/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __MK82_SYSTEM_H__
#define __MK82_SYSTEM_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MK82_CMP_EQUAL 0x9999
#define MK82_CMP_NOT_EQUAL 0x6666

#ifndef BOOTSTRAPPER
void mk82SystemInit(void);

void mk82SystemGetSerialNumber(uint32_t* serialNumber);

#ifdef FIRMWARE
void mk82SystemTickerGetMsPassed(uint64_t* ms);
void mk82SystemGetRandom(uint8_t* buffer, uint32_t bufferLength);
int mk82SystemGetRandomForTLS(void* param, unsigned char* buffer, size_t bufferLength);
#endif /* FIRMWARE */

#endif /* BOOTSTRAPPER */

void mk82SystemMemCpy(uint8_t* dst, uint8_t* src, uint16_t length);
void mk82SystemMemSet(uint8_t* dst, uint8_t value, uint16_t length);
uint16_t mk82SystemMemCmp(uint8_t* array1, uint8_t* array2, uint16_t length);

void mk82SystemFatalError(void);

#ifdef __cplusplus
}
#endif

#endif /* __MK82_SYSTEM_H__ */
