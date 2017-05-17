/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __BTC_GLOBAL_H__
#define __BTC_GLOBAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define BTC_TRUE 0x9999
#define BTC_FALSE 0x6666

#define BTC_NO_ERROR 0x9999
#define BTC_GENERAL_ERROR 0x6666

#define BTC_KEY_DERIVATION_ERROR 0xCCCC
#define BTC_TRANSACTION_PARSING_FAILED_ERROR 0x3333

#define BTC_INVALID_INPUT_ERROR 0x5555

#define BTC_INVALID_MAC_ERROR 0xAAAA

#define BTC_PIN_BLOCKED_ERROR 0x7777
#define BTC_INVALID_PIN_LENGTH_ERROR 0xBBBB
#define BTC_INVALID_PIN_ERROR 0xDDDD

#define BTC_MESSAGE_SIGNING_ERROR 0xEEEE

#define BTC_CMP_EQUAL 0x9999
#define BTC_CMP_NOT_EQUAL 0x6666

#define BTC_GLOBAL_MASTER_KEY_SIZE (0x40)
#define BTC_GLOBAL_MASTER_KEY_PRIVATE_KEY_OFFSET (0x00)
#define BTC_GLOBAL_MASTER_KEY_CHAIN_CODE_OFFSET (0x20)
#define BTC_GLOBAL_PRIVATE_KEY_SIZE (32)
#define BTC_GLOBAL_CHAIN_CODE_SIZE (32)
#define BTC_GLOBAL_PUBLIC_KEY_COORDINATE_SIZE (32)
#define BTC_GLOBAL_ENCODED_COMPRESSED_POINT_SIZE (33)
#define BTC_GLOBAL_ENCODED_FULL_POINT_SIZE (65)
#define BTC_GLOBAL_RIPEMD160_SIZE (20)
#define BTC_GLOBAL_SHA256_SIZE (32)
#define BTC_GLOBAL_SHA512_SIZE (64)
#define BTC_GLOBAL_SHA512_LEFT_PART_OFFSET (0)
#define BTC_GLOBAL_SHA512_RIGHT_PART_OFFSET (32)

#define BTC_GLOBAL_MAXIMAL_BITCOIN_BASE58_ADDRESS_LENGTH (35)
#define BTC_GLOBAL_MINIMAL_BITCOIN_BASE58_ADDRESS_LENGTH (26)

#define BTC_GLOBAL_PIN_HASH_LENGTH (0x20)

#define BTC_GLOBAL_OUTPOINT_LENGTH (36)
#define BTC_GLOBAL_TRUSTED_INPUT_BLOB_LENGTH (56)
#define BTC_GLOBAL_TRUSTED_INPUT_KEY_SIZE (0x10)

#define BTC_GLOBAL_TRUSTED_INPUT_BLOB_MAGIC_OFFSET (0)
#define BTC_GLOBAL_TRUSTED_INPUT_BLOB_MAGIC_LENGTH (4)
#define BTC_GLOBAL_TRUSTED_INPUT_BLOB_HASH_OFFSET (4)
#define BTC_GLOBAL_TRUSTED_INPUT_BLOB_HASH_LENGTH (32)
#define BTC_GLOBAL_TRUSTED_INPUT_BLOB_INDEX_OFFSET (36)
#define BTC_GLOBAL_TRUSTED_INPUT_BLOB_INDEX_LENGTH (4)
#define BTC_GLOBAL_TRUSTED_INPUT_BLOB_AMOUNT_OFFSET (40)
#define BTC_GLOBAL_TRUSTED_INPUT_BLOB_AMOUNT_LENGTH (8)
#define BTC_GLOBAL_TRUSTED_INPUT_BLOB_HASH_INDEX_AND_AMOUNT_OFFSET (4)
#define BTC_GLOBAL_TRUSTED_INPUT_BLOB_HASH_INDEX_AND_AMOUNT_LENGTH (44)
#define BTC_GLOBAL_TRUSTED_INPUT_BLOB_MAC_OFFSET (48)
#define BTC_GLOBAL_TRUSTED_INPUT_BLOB_MAC_LENGTH (8)

#define BTC_GLOBAL_TRUSTED_INPUT_BLOB_MAGIC (0x000000BB1)

#define BTC_GLOBAL_NO_USER_VALIDATION_REQUESTED (0x00)

#define BTC_HIBYTE(w) ((unsigned char)(((unsigned short)(w) >> 8) & 0xFF))
#define BTC_HIWORD(d) ((unsigned short)((((unsigned long)(d)) >> 16) & 0xFFFF))
#define BTC_LOBYTE(w) ((unsigned char)(w))
#define BTC_LOWORD(d) ((unsigned short)(d))
#define BTC_MAKEWORD(lb, hb) ((unsigned short)(((unsigned char)(lb)) | (((unsigned short)((unsigned char)(hb))) << 8)))
#define BTC_MAKEDWORD(lw, hw) \
    ((unsigned long)(((unsigned short)(lw)) | (((unsigned long)((unsigned short)(hw))) << 16)))
#define BTC_MAKEQWORD(ldw, hdw) \
    ((unsigned long long)(((unsigned long)(ldw)) | (((unsigned long long)((unsigned long)(hdw))) << 32)))

#ifndef NULL
#define NULL (0)
#endif

#ifdef __CC_ARM
#define BTC_MAKE_PACKED(x) __packed x
#elif __GNUC__
#define BTC_MAKE_PACKED(x) x __attribute__((packed))
#else
#error Unsupported platform
#endif

#ifdef __cplusplus
}
#endif

#endif /* __BTC_GLOBAL_H__ */
