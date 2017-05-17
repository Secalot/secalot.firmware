/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SF_OPGP_GLOBAL_INT_H__
#define __SF_OPGP_GLOBAL_INT_H__

#include <stdint.h>

#define OPGP_HIBYTE(w) ((unsigned char)(((unsigned short)(w) >> 8) & 0xFF))
#define OPGP_HIWORD(d) ((unsigned short)((((unsigned long)(d)) >> 16) & 0xFFFF))
#define OPGP_LOBYTE(w) ((unsigned char)(w))
#define OPGP_LOWORD(d) ((unsigned short)(d))
#define OPGP_MAKEWORD(lb, hb) ((unsigned short)(((unsigned char)(lb)) | (((unsigned short)((unsigned char)(hb))) << 8)))
#define OPGP_MAKEDWORD(lw, hw) \
    ((unsigned long)(((unsigned short)(lw)) | (((unsigned long)((unsigned short)(hw))) << 16)))

#ifndef NULL
#define NULL (0)
#endif

#endif /* __SF_OPGP_GLOBAL_INT_H__ */
