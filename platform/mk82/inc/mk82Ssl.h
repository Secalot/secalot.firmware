/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __MK82_SSL_H__
#define __MK82_SSL_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MK82_SSL_STATUS_UNWRAPPED (0x9999)
#define MK82_SSL_STATUS_ERROR_OCCURED (0x6666)
#define MK82_SSL_STATUS_NOT_SSL (0xCCCC)

void mk82SslInit(void);

void mk82SslGetAID(uint8_t* aid, uint32_t* aidLength);

void mk82SslProcessAPDU(uint8_t* apdu, uint32_t* apduLength);

void mk82SslUnwrapAPDUCommand(uint8_t* apdu, uint32_t* apduLength, uint16_t* status);
void mk82SslWrapAPDUResponse(uint8_t* apdu, uint32_t* apduLength);




#ifdef __cplusplus
}
#endif

#endif /* __MK82_SSL_H__ */

