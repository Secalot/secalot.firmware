/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SYM_GLOBAL_H__
#define __SYM_GLOBAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define SYM_TRUE 0x9999
#define SYM_FALSE 0x6666

#define SYM_NO_ERROR 0x9999
#define SYM_GENERAL_ERROR 0x6666

#define SYM_GLOBAL_MASTER_KEY_SIZE (32)
#define SYM_GLOBAL_PRIVATE_KEY_SIZE (32)
#define SYM_GLOBAL_SHA256_SIZE (32)

#define SYM_HIBYTE(w) ((unsigned char)(((unsigned short)(w) >> 8) & 0xFF))
#define SYM_HIWORD(d) ((unsigned short)((((unsigned long)(d)) >> 16) & 0xFFFF))
#define SYM_LOBYTE(w) ((unsigned char)(w))
#define SYM_LOWORD(d) ((unsigned short)(d))
#define SYM_MAKEWORD(lb, hb) ((unsigned short)(((unsigned char)(lb)) | (((unsigned short)((unsigned char)(hb))) << 8)))
#define SYM_MAKEDWORD(lw, hw) \
    ((unsigned long)(((unsigned short)(lw)) | (((unsigned long)((unsigned short)(hw))) << 16)))

#ifndef NULL
#define NULL (0)
#endif

#ifdef __CC_ARM
#define SYM_MAKE_PACKED(x) __packed x
#elif __GNUC__
#define SYM_MAKE_PACKED(x) x __attribute__((packed))
#else
#error Unsupported platform
#endif

#ifdef __cplusplus
}
#endif

#endif /* __SYM_GLOBAL_H__ */
