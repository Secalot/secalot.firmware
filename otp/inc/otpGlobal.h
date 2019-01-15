/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __OTP_GLOBAL_H__
#define __OTP_GLOBAL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define OTP_TRUE 0x9999
#define OTP_FALSE 0x6666

#define OTP_NO_ERROR 0x9999
#define OTP_GENERAL_ERROR 0x6666

#define OTP_KEY_NOT_SET_ERROR 0xCCCC

#define OTP_TIME_NOT_SET_ERROR 0x3333

#define OTP_GLOBAL_MIN_DIGITS (6)
#define OTP_GLOBAL_MAX_DIGITS (8)

#define OTP_GLOBAL_MAX_OTP_LENGTH (8)

#define OTP_GLOBAL_MIN_KEYLENGTH (10)
#define OTP_GLOBAL_MAX_KEYLENGTH (32)

#define OTP_GLOBAL_TYPE_HOTP (0x9999)
#define OTP_GLOBAL_TYPE_TOTP (0x6666)

#define OTP_HIBYTE(w) ((unsigned char)(((unsigned short)(w) >> 8) & 0xFF))
#define OTP_HIWORD(d) ((unsigned short)((((unsigned long)(d)) >> 16) & 0xFFFF))
#define OTP_LOBYTE(w) ((unsigned char)(w))
#define OTP_LOWORD(d) ((unsigned short)(d))
#define OTP_MAKEWORD(lb, hb) ((unsigned short)(((unsigned char)(lb)) | (((unsigned short)((unsigned char)(hb))) << 8)))
#define OTP_MAKEDWORD(lw, hw) \
    ((unsigned long)(((unsigned short)(lw)) | (((unsigned long)((unsigned short)(hw))) << 16)))

#ifndef NULL
#define NULL (0)
#endif

#ifdef __CC_ARM
#define OTP_MAKE_PACKED(x) __packed x
#elif __GNUC__
#define OTP_MAKE_PACKED(x) x __attribute__((packed))
#else
#error Unsupported platform
#endif

#ifdef __cplusplus
}
#endif

#endif /* __OTP_GLOBAL_H__ */
