/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SF_CORE_INT_H__
#define __SF_CORE_INT_H__

#define SF_CORE_SW_NO_ERROR 0x9000
#define SF_CORE_SW_GENERAL_ERROR 0x6F00
#define SF_CORE_SW_WRONG_DATA 0x6A80
#define SF_CORE_SW_CONDITIONS_NOT_SATISFIED 0x6985
#define SF_CORE_SW_INS_NOT_SUPPORTED 0x6d00
#define SF_CORE_SW_CLA_NOT_SUPPORTED 0x6e00
#define SF_CORE_SW_INVALID_LENGTH 0x6700

typedef struct
{
    uint8_t cla;
    uint8_t ins;
    uint8_t p1;
    uint8_t p2;
    uint8_t lc1;
    uint8_t lc2;
    uint8_t lc3;
    uint8_t data[];

} SF_CORE_COMMAND_APDU;

#define SF_CORE_MINIMUM_COMMAND_APDU_LENGTH (7)

#define SF_CORE_DEFAULT_CLA_VALUE (0x00)
#define SF_CORE_INS_REGISTER (0x01)
#define SF_CORE_INS_AUTHENTICATE (0x02)
#define SF_CORE_INS_GET_VERSION (0x03)

#define SF_CORE_REGISTER_HASH_ID 0x00
#define SF_CORE_REGISTER_ID 0x05

#define SF_CORE_ENFORCE_AUTHENTICATION 0x03
#define SF_CORE_AUTHENTICATION_CHECK_ONLY 0x07
#define SF_CORE_AUTHENTICATION_FLAG_TUP 0x01

// The length of a GUID string
#define SF_CORE_KEY_HANDLE_LENGTH_LENGTH (1)
#define SF_CORE_USER_PRESENCE_BYTE_LENGTH (1)
#define SF_CORE_REGISTER_ID_LENGTH (1)
#define SF_CORE_COUNTER_LENGTH (4)
#define SF_CORE_GET_VERSION_VERSION_STRING_LENGTH (6)

typedef struct
{
    uint8_t challenge[SF_GLOBAL_CHALLENGE_LENGTH];
    uint8_t applicationId[SF_GLOBAL_APPLICATION_ID_LENGTH];
} SF_CORE_REGISTER_REQ;

typedef struct
{
    uint8_t registerId;
    uint8_t publicKey[SF_GLOBAL_PUBLIC_KEY_LENGTH];
    uint8_t keyHandleLength;
    uint8_t keyHandle[SF_GLOBAL_KEY_HANDLE_LENGTH];
    uint8_t certificateAndSignature[];
} SF_CORE_REGISTER_RESP;

typedef struct
{
    uint8_t challenge[SF_GLOBAL_CHALLENGE_LENGTH];
    uint8_t applicationId[SF_GLOBAL_APPLICATION_ID_LENGTH];
    uint8_t keyHandleLength;
    uint8_t keyHandle[SF_GLOBAL_KEY_HANDLE_LENGTH];
} SF_CORE_AUTHENTICATE_REQ;

typedef struct
{
    uint8_t flags;
    uint8_t counter[SF_CORE_COUNTER_LENGTH];
    uint8_t signature[];
} SF_CORE_AUTHENTICATE_RESP;

typedef struct
{
    uint8_t version[SF_CORE_GET_VERSION_VERSION_STRING_LENGTH];
} SF_CORE_GET_VERSION_RESP;

#define SF_CORE_AUTHENTIATE_RESPONSE_LENGTH_WITHOUT_SIGNATURE \
    (SF_CORE_USER_PRESENCE_BYTE_LENGTH + SF_CORE_COUNTER_LENGTH)

#define SF_CORE_REGISTER_RESPONSE_LENGTH_WITOUT_CERTIFICATE_AND_SIGNATURE                          \
    (SF_CORE_REGISTER_ID_LENGTH + SF_GLOBAL_PUBLIC_KEY_LENGTH + SF_CORE_KEY_HANDLE_LENGTH_LENGTH + \
     SF_GLOBAL_KEY_HANDLE_LENGTH)

#define SF_CORE_GET_VERSION_REQUEST_DATA_LENGTH (0)

#define SF_CORE_GET_VERSION_VERSION_STRING "U2F_V2"

static uint16_t sfCoreProcessRegistration(uint8_t* apduBuffer, uint32_t* apduBufferLength);
static uint16_t sfCoreProcessAuthentication(uint8_t* apduBuffer, uint32_t* apduBufferLength);
static uint16_t sfCoreGetVersion(uint8_t* apduBuffer, uint32_t* apduBufferLength);

#endif /* __SF_CORE_INT_H__ */
