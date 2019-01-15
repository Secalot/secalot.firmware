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
extern "C"
{
#endif

#define MK82_SSL_STATUS_UNWRAPPED (0x9999)
#define MK82_SSL_STATUS_ERROR_OCCURED (0x6666)
#define MK82_SSL_STATUS_NOT_SSL (0xCCCC)

#define MK82_SSL_PUBLIC_KEY_LENGTH (65)
#define MK82_SSL_PRIVATE_KEY_LENGTH (32)
#define MK82_SSL_NONCE_LENGTH (12)
#define MK82_SSL_TAG_LENGTH (16)

    MK82_MAKE_PACKED(typedef struct)
    {
        uint16_t keyInitialized; /* MK82_FALSE or 0 */
        uint8_t privateKey[MK82_SSL_PRIVATE_KEY_LENGTH];
        uint8_t privateKeyNonce[MK82_SSL_NONCE_LENGTH];
        uint8_t privateKeyTag[MK82_SSL_TAG_LENGTH];
        uint8_t publicKey[MK82_SSL_PUBLIC_KEY_LENGTH];
    }
    SSL_NVM_KEYS;

    void mk82SslInit(void);

    void mk82SslGetAID(uint8_t* aid, uint32_t* aidLength);

    void mk82SslProcessAPDU(uint8_t* apdu, uint32_t* apduLength);

    void mk82SslUnwrapAPDUCommand(uint8_t* apdu, uint32_t* apduLength, uint16_t* status);
    void mk82SslWrapAPDUResponse(uint8_t* apdu, uint32_t* apduLength);

#ifdef __cplusplus
}
#endif

#endif /* __MK82_SSL_H__ */
