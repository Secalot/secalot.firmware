/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __ETH_GLOBAL_H__
#define __ETH_GLOBAL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define ETH_TRUE 0x9999
#define ETH_FALSE 0x6666

#define ETH_NO_ERROR 0x9999
#define ETH_GENERAL_ERROR 0x6666
#define ETH_KEY_DERIVATION_ERROR 0xCCCC

#define ETH_PIN_BLOCKED_ERROR 0x7777
#define ETH_INVALID_PIN_LENGTH_ERROR 0xBBBB
#define ETH_INVALID_PIN_ERROR 0xDDDD

#define ETH_GLOBAL_MASTER_KEY_SIZE (0x40)
#define ETH_GLOBAL_PRIVATE_KEY_SIZE (32)
#define ETH_GLOBAL_CHAIN_CODE_SIZE (32)
#define ETH_GLOBAL_ENCODED_FULL_POINT_SIZE (65)
#define ETH_GLOBAL_PIN_HASH_LENGTH (0x20)
#define ETH_GLOBAL_ADDRESS_SIZE (0x14)

#define ETH_HIBYTE(w) ((unsigned char)(((unsigned short)(w) >> 8) & 0xFF))
#define ETH_HIWORD(d) ((unsigned short)((((unsigned long)(d)) >> 16) & 0xFFFF))
#define ETH_LOBYTE(w) ((unsigned char)(w))
#define ETH_LOWORD(d) ((unsigned short)(d))
#define ETH_MAKEWORD(lb, hb) ((unsigned short)(((unsigned char)(lb)) | (((unsigned short)((unsigned char)(hb))) << 8)))
#define ETH_MAKEDWORD(lw, hw) \
    ((unsigned long)(((unsigned short)(lw)) | (((unsigned long)((unsigned short)(hw))) << 16)))

#ifndef NULL
#define NULL (0)
#endif

#ifdef __CC_ARM
#define ETH_MAKE_PACKED(x) __packed x
#elif __GNUC__
#define ETH_MAKE_PACKED(x) x __attribute__((packed))
#else
#error Unsupported platform
#endif

#ifdef __cplusplus
}
#endif

#endif /* __ETH_GLOBAL_H__ */
