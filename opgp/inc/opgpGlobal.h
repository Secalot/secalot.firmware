/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SF_OPGP_GLOBAL_H__
#define __SF_OPGP_GLOBAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __CC_ARM
#define OPGP_MAKE_PACKED(x) __packed x
#elif __GNUC__
#define OPGP_MAKE_PACKED(x) x __attribute__((packed))
#else
#error Unsupported platform
#endif

#define OPGP_TRUE 0x9999
#define OPGP_FALSE 0x6666

#define OPGP_NO_ERROR 0x9999
#define OPGP_GENERAL_ERROR 0x6666

#define OPGP_INVALID_PIN_LENGTH_ERROR 0xCCCC
#define OPGP_INVALID_PIN_ERROR 0x3333
#define OPGP_PIN_BLOCKED_ERROR 0x5555

#define OPGP_CMP_EQUAL 0x9999
#define OPGP_CMP_NOT_EQUAL 0x6666

#define OPGP_INVALID_CRYPTO_DATA_ERROR 0xCCCC

#define OPGP_GLOBAL_MAX_PIN_LENGTH (0xFF)

#define OPGP_GLOBAL_PIN_ID_PW1_81 (0x81)
#define OPGP_GLOBAL_PIN_ID_PW1_82 (0x82)
#define OPGP_GLOBAL_PIN_ID_PW3 (0x83)
#define OPGP_GLOBAL_PIN_ID_RC (0x84)

#define OPGP_GLOBAL_PW1_MINIMUM_LENGTH (0x6)
#define OPGP_GLOBAL_PW3_MINIMUM_LENGTH (0x8)
#define OPGP_GLOBAL_RC_MINIMUM_LENGTH (0x8)

#define OPGP_GLOBAL_PIN_HASH_LENGTH (0x20)

#define OPGP_GLOBAL_PIN_BLOCKED_ERROR_COUNTER_VALUE (0x00)
#define OPGP_GLOBAL_PIN_INITIAL_ERROR_COUNTER_VALUE (0x03)

#define OPGP_GLOBAL_DO_PRIVATE_USE_1_MAX_LENGTH (0xFE)
#define OPGP_GLOBAL_DO_PRIVATE_USE_2_MAX_LENGTH (0xFE)
#define OPGP_GLOBAL_DO_PRIVATE_USE_3_MAX_LENGTH (0xFE)
#define OPGP_GLOBAL_DO_PRIVATE_USE_4_MAX_LENGTH (0xFE)
#define OPGP_GLOBAL_DO_LOGIN_DATA_MAX_LENGTH (0xFE)
#define OPGP_GLOBAL_DO_URL_MAX_LENGTH (0xFE)
#define OPGP_GLOBAL_DO_NAME_MAX_LENGTH (0x27)
#define OPGP_GLOBAL_DO_LANGUAGE_PREFERENCE_MAX_LENGTH (0x08)
#define OPGP_GLOBAL_DO_SEX_LENGTH (0x01)
#define OPGP_GLOBAL_DO_CERTIFICATE_MAX_LENGTH (0x800)
#define OPGP_GLOBAL_DO_FINGERPRINT_LENGTH (0x14)
#define OPGP_GLOBAL_DO_CA_FINGERPRINT_LENGTH (0x14)
#define OPGP_GLOBAL_DO_GENERATION_DATE_TIME_LENGTH (0x04)
#define OPGP_GLOBAL_DO_PW_STATUS_BYTE_LENGTH (0x01)

#define OPGP_GLOBAL_TAG_PRIVATE_USE_1 (0x0101)
#define OPGP_GLOBAL_TAG_PRIVATE_USE_2 (0x0102)
#define OPGP_GLOBAL_TAG_PRIVATE_USE_3 (0x0103)
#define OPGP_GLOBAL_TAG_PRIVATE_USE_4 (0x0104)
#define OPGP_GLOBAL_TAG_AID (0x004F)
#define OPGP_GLOBAL_TAG_LOGIN_DATA (0x005E)
#define OPGP_GLOBAL_TAG_URL (0x5F50)
#define OPGP_GLOBAL_TAG_HISTORICAL_BYTES (0x5F52)
#define OPGP_GLOBAL_TAG_CARDHOLDER_RELATED_DATA (0x0065)
#define OPGP_GLOBAL_TAG_NAME (0x005B)
#define OPGP_GLOBAL_TAG_LANGUAGE_PREFERENCE (0x5F2D)
#define OPGP_GLOBAL_TAG_SEX (0x5F35)
#define OPGP_GLOBAL_TAG_APPLICATION_RELATED_DATA (0x006E)
#define OPGP_GLOBAL_TAG_DISCRETIONARY_DATA_OBJECTS (0x0073)
#define OPGP_GLOBAL_TAG_EXTENDED_CAPABILITIES (0x00C0)
#define OPGP_GLOBAL_TAG_ALGORITHM_ATTRIBUTES_SIGNATURE (0x00C1)
#define OPGP_GLOBAL_TAG_ALGORITHM_ATTRIBUTES_DECRYPTION (0x00C2)
#define OPGP_GLOBAL_TAG_ALGORITHM_ATTRIBUTES_AUTHENTICATION (0x00C3)
#define OPGP_GLOBAL_TAG_PW_STATUS_BYTES (0x00C4)
#define OPGP_GLOBAL_TAG_FINGERPRINTS (0x00C5)
#define OPGP_GLOBAL_TAG_CA_FINGERPRINTS (0x00C6)
#define OPGP_GLOBAL_TAG_SIGNATURE_FINGERPRINT (0x00C7)
#define OPGP_GLOBAL_TAG_DECRYPTION_FINGERPRINT (0x00C8)
#define OPGP_GLOBAL_TAG_AUTHENTICATION_FINGERPRINT (0x00C9)
#define OPGP_GLOBAL_TAG_1ST_CA_FINGERPRINT (0x00CA)
#define OPGP_GLOBAL_TAG_2ND_CA_FINGERPRINT (0x00CB)
#define OPGP_GLOBAL_TAG_3RD_CA_FINGERPRINT (0x00CC)
#define OPGP_GLOBAL_TAG_GENERATION_DATE_TIMES (0x00CD)
#define OPGP_GLOBAL_TAG_GENERATION_DATE_TIME_SIGNATURE_KEY (0x00CE)
#define OPGP_GLOBAL_TAG_GENERATION_DATE_TIME_DECRYPTION_KEY (0x00CF)
#define OPGP_GLOBAL_TAG_GENERATION_DATE_TIME_AUTHENTICATION_KEY (0x00D0)
#define OPGP_GLOBAL_TAG_SECURITY_SUPPORT_TEMPLATE (0x007A)
#define OPGP_GLOBAL_TAG_DS_COUNTER (0x0093)
#define OPGP_GLOBAL_TAG_CARD_HOLDER_CERTIFICATE (0x7F21)
#define OPGP_GLOBAL_TAG_RESETTING_CODE (0x00D3)

#define OPGP_GLOBAL_TAG_EXTENDED_HEADER_LIST (0x004D)
#define OPGP_GLOBAL_TAG_CARD_HOLDER_PRIVATE_KEY_TEMPLATE (0x7F48)
#define OPGP_GLOBAL_TAG_CARD_HOLDER_PRIVATE_KEY (0x5F48)

#define OPGP_GLOBAL_CARD_STATE_INITIALIZATION (0x03)
#define OPGP_GLOBAL_CARD_STATE_OPERATIONAL (0x05)

#define OPGP_GLOBAL_PUBLIC_EXPONENT_LENGTH (0x04)
#define OPGP_GLOBAL_MODULUS_LENGTH (0x100)
#define OPGP_GLOBAL_PRIME_LENGTH (0x80)

#define OPGP_GLOBAL_KEY_TYPE_SIGNATURE (0xB600)
#define OPGP_GLOBAL_KEY_TYPE_CONFIDENTIALITY (0xB800)
#define OPGP_GLOBAL_KEY_TYPE_AUTHENTICATION (0xA400)

#define OPGP_GLOBAL_PW1_81_VALID_FOR_ONE_COMMAND (0x00)
#define OPGP_GLOBAL_PW1_81_VALID_FOR_MULTIPLE_COMMANDS (0x01)

#ifdef __cplusplus
}
#endif

#endif /* __SF_OPGP_GLOBAL_H__ */
