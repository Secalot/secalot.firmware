/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __APDU_CORE_H__
#define __APDU_CORE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8_t cla;
    uint8_t ins;
    uint16_t p1p2;
    uint16_t lcPresent;
    uint32_t lc;
    uint16_t lePresent;
    uint8_t le;
    uint8_t* data;
} APDU_CORE_COMMAND_APDU;

typedef struct
{
    uint8_t* data;
    uint32_t dataLength;
    uint16_t sw;
} APDU_CORE_RESPONSE_APDU;

#define APDU_CORE_OFFSET_CLA (0x00)
#define APDU_CORE_OFFSET_INS (0x01)
#define APDU_CORE_OFFSET_P1 (0x02)
#define APDU_CORE_OFFSET_P2 (0x03)
#define APDU_CORE_OFFSET_LC_OR_LE (0x04)
#define APDU_CORE_OFFSET_DATA (0x05)
#define APDU_CORE_OFFSET_EXTENDED_LENGTH_LC_OR_LE1 (0x04)
#define APDU_CORE_OFFSET_EXTENDED_LENGTH_LC_OR_LE2 (0x05)
#define APDU_CORE_OFFSET_EXTENDED_LENGTH_LC_OR_LE3 (0x06)
#define APDU_CORE_OFFSET_EXTENDED_LENGTH_DATA (0x07)

#define APDU_CORE_MAX_NORMAL_APDU_DATA_LENGTH (0xFF)

#define APDU_CORE_SW_BYTES_REMAINING (0x6100)
#define APDU_CORE_SW_TERMINATION_STATE (0x6285)
#define APDU_CORE_SW_MEMORY_FAILURE (0x6581)
#define APDU_CORE_SW_WRONG_LENGTH (0x6700)
#define APDU_CORE_SW_SECURE_MESSAGING_NOT_SUPPORTED (0x6882)
#define APDU_CORE_SW_LAST_COMMAND_EXPECTED (0x6883)
#define APDU_CORE_SW_COMMAND_CHAINING_NOT_SUPPORTED (0x6884)
#define APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED (0x6982)
#define APDU_CORE_SW_PIN_BLOCKED (0x6983)
#define APDU_CORE_SW_CONDITIONS_NOT_SATISFIED (0x6985)
#define APDU_CORE_SW_EXPECTED_SM_DATA_OBJECTS_MISSING (0x6987)
#define APDU_CORE_SW_EXPECTED_SM_DATA_OBJECTS_INCORRECT (0x6988)
#define APDU_CORE_SW_WRONG_DATA (0x6A80)
#define APDU_CORE_SW_REF_DATA_NOT_FOUND (0x6A88)
#define APDU_CORE_SW_WRONG_P1P2 (0x6B00)
#define APDU_CORE_SW_INS_NOT_SUPPORTED (0x6D00)
#define APDU_CORE_SW_CLA_NOT_SUPPORTED (0x6E00)
#define APDU_CORE_SW_UNKNOWN (0x6F00)
#define APDU_CORE_SW_NO_ERROR (0x9000)

void apduCoreInit(void);
void apduCoreDeinit(void);

uint16_t apduCoreParseIncomingAPDU(uint8_t* apdu, uint32_t apduLength, APDU_CORE_COMMAND_APDU* parsedAPDU);
void apduCorePrepareResponseAPDUStructure(uint8_t* apdu, APDU_CORE_RESPONSE_APDU* responseAPDU);
void apduCorePrepareOutgoingAPDU(uint8_t* apdu, uint32_t* apduLength, APDU_CORE_RESPONSE_APDU* responseAPDU);

#ifdef __cplusplus
}
#endif

#endif /* __OTP_CORE_H__ */
