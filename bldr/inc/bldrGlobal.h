/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __BLDR_GLOBAL_H__
#define __BLDR_GLOBAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define BLDR_TRUE 0x9999
#define BLDR_FALSE 0x6666

#define BLDR_NO_ERROR 0x9999
#define BLDR_GENERAL_ERROR 0x6666
#define BLDR_SIGNATURE_VERIFICATION_FAILED_ERROR 0xCCCC
#define BLDR_MAC_VERIFICATION_FAILED_ERROR 0x3333

#define BLDR_HIBYTE(w) ((unsigned char)(((unsigned short)(w) >> 8) & 0xFF))
#define BLDR_HIWORD(d) ((unsigned short)((((unsigned long)(d)) >> 16) & 0xFFFF))
#define BLDR_LOBYTE(w) ((unsigned char)(w))
#define BLDR_LOWORD(d) ((unsigned short)(d))
#define BLDR_MAKEWORD(lb, hb) ((unsigned short)(((unsigned char)(lb)) | (((unsigned short)((unsigned char)(hb))) << 8)))
#define BLDR_MAKEDWORD(lw, hw) \
    ((unsigned long)(((unsigned short)(lw)) | (((unsigned long)((unsigned short)(hw))) << 16)))

#ifndef NULL
#define NULL (0)
#endif

#ifdef __CC_ARM
#define BLDR_MAKE_PACKED(x) __packed x
#elif __GNUC__
#define BLDR_MAKE_PACKED(x) x __attribute__((packed))
#else
#error Unsupported platform
#endif

#ifdef __cplusplus
}
#endif

#endif /* __BLDR_GLOBAL_H__ */
