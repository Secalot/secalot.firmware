/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SF_GLOBAL_H__
#define __SF_GLOBAL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define SF_TRUE 0x9999
#define SF_FALSE 0x6666

#define SF_HIBYTE(w) ((unsigned char)(((unsigned short)(w) >> 8) & 0xFF))
#define SF_HIWORD(d) ((unsigned short)((((unsigned long)(d)) >> 16) & 0xFFFF))
#define SF_LOBYTE(w) ((unsigned char)(w))
#define SF_LOWORD(d) ((unsigned short)(d))
#define SF_MAKEWORD(lb, hb) ((unsigned short)(((unsigned char)(lb)) | (((unsigned short)((unsigned char)(hb))) << 8)))
#define SF_MAKEDWORD(lw, hw) ((unsigned long)(((unsigned short)(lw)) | (((unsigned long)((unsigned short)(hw))) << 16)))

#ifndef NULL
#define NULL (0)
#endif

#ifdef __CC_ARM
#define SF_MAKE_PACKED(x) __packed x
#elif __GNUC__
#define SF_MAKE_PACKED(x) x __attribute__((packed))
#else
#error Unsupported platform
#endif

#ifdef __cplusplus
}
#endif

#endif /* __SF_GLOBAL_H__ */
