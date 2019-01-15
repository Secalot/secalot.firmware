/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __XRP_GLOBAL_H__
#define __XRP_GLOBAL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define XRP_TRUE 0x9999
#define XRP_FALSE 0x6666

#define XRP_NO_ERROR 0x9999
#define XRP_GENERAL_ERROR 0x6666

#define XRP_PIN_BLOCKED_ERROR 0x7777
#define XRP_INVALID_PIN_LENGTH_ERROR 0xBBBB
#define XRP_INVALID_PIN_ERROR 0xDDDD

#define XRP_GLOBAL_PRIVATE_KEY_SIZE (32)
#define XRP_GLOBAL_SECRET_SIZE (16)
#define XRP_GLOBAL_SHA256_SIZE (32)
#define XRP_GLOBAL_SHA512_SIZE (64)
#define XRP_GLOBAL_ENCODED_FULL_POINT_SIZE (65)
#define XRP_GLOBAL_ENCODED_COMPRESSED_POINT_SIZE (33)
#define XRP_GLOBAL_PIN_HASH_LENGTH (0x20)

#define XRP_HIBYTE(w) ((unsigned char)(((unsigned short)(w) >> 8) & 0xFF))
#define XRP_HIWORD(d) ((unsigned short)((((unsigned long)(d)) >> 16) & 0xFFFF))
#define XRP_LOBYTE(w) ((unsigned char)(w))
#define XRP_LOWORD(d) ((unsigned short)(d))
#define XRP_MAKEWORD(lb, hb) ((unsigned short)(((unsigned char)(lb)) | (((unsigned short)((unsigned char)(hb))) << 8)))
#define XRP_MAKEDWORD(lw, hw) \
    ((unsigned long)(((unsigned short)(lw)) | (((unsigned long)((unsigned short)(hw))) << 16)))

#ifndef NULL
#define NULL (0)
#endif

#ifdef __CC_ARM
#define XRP_MAKE_PACKED(x) __packed x
#elif __GNUC__
#define XRP_MAKE_PACKED(x) x __attribute__((packed))
#else
#error Unsupported platform
#endif

#ifdef __cplusplus
}
#endif

#endif /* __XRP_GLOBAL_H__ */
