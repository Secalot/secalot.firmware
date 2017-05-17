/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SF_OPGP_CORE_INT_H__
#define __SF_OPGP_CORE_INT_H__

#include <opgpGlobal.h>
#include <opgpGlobalInt.h>
#include <ccidGlobal.h>

#define OPGP_CORE_IGNORE_LE 1

#define OPGP_CORE_CLA_NORMAL (0x00)

#define OPGP_CORE_INS_GET_RESPONSE (0xC0)

#define OPGP_CORE_INS_GET_DATA (0xCA)
#define OPGP_CORE_INS_VERIFY (0x20)
#define OPGP_CORE_INS_CHANGE_REFERENCE_DATA (0x24)
#define OPGP_CORE_INS_RESET_RETRY_COUNTER (0x2C)
#define OPGP_CORE_INS_PUT_DATA (0xDA)
#define OPGP_CORE_INS_PUT_DATA_IMPORT_KEY (0xDB)
#define OPGP_CORE_INS_GEN_KEY_PAIR (0x47)
#define OPGP_CORE_INS_PSO (0x2A)
#define OPGP_CORE_INS_INTERNAL_AUTHENTICATE (0x88)
#define OPGP_CORE_INS_GET_CHALLENGE (0x84)
#define OPGP_CORE_INS_TERMINATE_DF (0xE6)
#define OPGP_CORE_INS_ACTIVATE_FILE (0x44)

#define OPGP_CORE_P1P2_GET_RESPONSE (0x0000)
#define OPGP_CORE_P1P2_RESET_RETRY_COUNTER_WITH_RC (0x0081)
#define OPGP_CORE_P1P2_RESET_RETRY_COUNTER_WITHOUT_RC (0x0281)
#define OPGP_CORE_P1P2_PUT_DATA_IMPORT_KEY (0x3FFF)
#define OPGP_CORE_P1P2_COMPUTE_DIGITAL_SIGNATURE (0x9E9A)
#define OPGP_CORE_P1P2_DECIPHER (0x8086)
#define OPGP_CORE_P1P2_INTERNAL_AUTHENTICATE (0x0000)
#define OPGP_CORE_P1P2_GENERATE_KEY_PAIR (0x8000)
#define OPGP_CORE_P1P2_READ_PUBLIC_KEY (0x8100)
#define OPGP_CORE_P1P2_GET_CHALLENGE (0x0000)
#define OPGP_CORE_P1P2_TERMINATE_DF (0x0000)
#define OPGP_CORE_P1P2_ACTIVATE_FILE (0x0000)

#define OPGP_CORE_P1P2_LC_GENERATE_KEY_PAIR (0x02)

#define OPGP_CORE_AID                                                                                  \
    {                                                                                                  \
        0xD2, 0x76, 0x00, 0x01, 0x24, 0x01, 0x02, 0x01, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 \
    }
#define OPGP_CORE_AID_LENGTH (0x10)
#define OPGP_CORE_AID_SERIAL_NUMBER_OFFSET (0x0A)

#define OPGP_CORE_HIST_CHARS                                       \
    {                                                              \
        0x00, 0x31, 0xC5, 0x73, 0xC0, 0x01, 0x40, 0x00, 0x90, 0x00 \
    }
#define OPGP_CORE_HIST_CHARS_LENGTH (0x0A)

#define OPGP_CORE_CHALLENGE_MAX_LENGTH (0xFF)

#define OPGP_CORE_EXTENDED_CAPABILITIES                                                                             \
    {                                                                                                               \
        0x78, 0x00, OPGP_HIBYTE(OPGP_CORE_CHALLENGE_MAX_LENGTH), OPGP_LOBYTE(OPGP_CORE_CHALLENGE_MAX_LENGTH),       \
            OPGP_HIBYTE(OPGP_GLOBAL_DO_CERTIFICATE_MAX_LENGTH), OPGP_LOBYTE(OPGP_GLOBAL_DO_CERTIFICATE_MAX_LENGTH), \
            OPGP_HIBYTE(CCID_MAX_APDU_DATA_SIZE), OPGP_LOBYTE(CCID_MAX_APDU_DATA_SIZE),                             \
            OPGP_HIBYTE(CCID_MAX_APDU_DATA_SIZE), OPGP_LOBYTE(CCID_MAX_APDU_DATA_SIZE)                              \
    }

#define OPGP_CORE_EXTENDED_CAPABILITIES_LENGTH (0x0A)

#define OPGP_CORE_ALGORITHM_ATTRIBUTES                                         \
    {                                                                          \
        0x01, 0x08, 0x00, 0x00, (OPGP_GLOBAL_PUBLIC_EXPONENT_LENGTH * 8), 0x00 \
    }
#define OPGP_CORE_ALGORITHM_ATTRIBUTES_LENGTH (0x06)

#define OPGP_CORE_PW_STATUS_LENGTH (0x07)

#define OPGP_CORE_IMPORT_KEY_HEADER                                                                                 \
    {                                                                                                               \
        0x4d, 0x82, 0x01, 0x16, 0x00, 0x00, 0x7f, 0x48, 0x08, 0x91, 0x04, 0x92, 0x81, 0x80, 0x93, 0x81, 0x80, 0x5f, \
            0x48, 0x82, 0x01, 0x04                                                                                  \
    }

#define OPGP_CORE_KEY_IMPORT_KEY_TYPE_OFFSET (0x04)

#define OPGP_CORE_IMPORT_KEY_HEADER_LENGTH (22)
#define OPGP_CORE_IMPORT_KEY_PUBLIC_EXPONENT_OFFSET (OPGP_CORE_IMPORT_KEY_HEADER_LENGTH)
#define OPGP_CORE_IMPORT_KEY_P_OFFSET (OPGP_CORE_IMPORT_KEY_PUBLIC_EXPONENT_OFFSET + OPGP_GLOBAL_PUBLIC_EXPONENT_LENGTH)
#define OPGP_CORE_IMPORT_KEY_Q_OFFSET (OPGP_CORE_IMPORT_KEY_P_OFFSET + OPGP_GLOBAL_PRIME_LENGTH)

#define OPGP_CORE_IMPORT_KEY_IMCOMING_DATA_LENGTH (OPGP_CORE_IMPORT_KEY_Q_OFFSET + OPGP_GLOBAL_PRIME_LENGTH)

#define OPGP_CORE_MAX_DSI_LENGTH (102)
#define OPGP_CORE_MAX_AUTHENTICATION_INPUT_LENGTH (102)

#define OPGP_CORE_DECIPHER_PADDING_INDICATOR (0x00)

#define OPGP_CORE_PUBLIC_KEY_DO_HEADER                       \
    {                                                        \
        0x7F, 0x49, 0x82, 0x01, 0x0A, 0x81, 0x82, 0x01, 0x00 \
    }
#define OPGP_CORE_PUBLIC_KEY_DO_MODULUS_OFFSET (0x09)
#define OPGP_CORE_PUBLIC_KEY_DO_PUBLIC_EXPONENT_TAG (0x82)
#define OPGP_CORE_PUBLIC_KEY_DO_PUBLIC_EXPONENT_TAG_OFFSET (0x109)
#define OPGP_CORE_PUBLIC_KEY_DO_PUBLIC_EXPONENT_LENGTH_OFFSET (0x10A)
#define OPGP_CORE_PUBLIC_KEY_DO_PUBLIC_EXPONENT_OFFSET (0x10B)
#define OPGP_CORE_PUBLIC_KEY_DO_LENGTH (0x10F)

#endif /* __SF_OPGP_CORE_INT_H__ */
