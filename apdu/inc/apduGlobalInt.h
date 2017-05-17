/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __APDU_GLOBAL_INT_H__
#define __APDU_GLOBAL_INT_H__

#define APDU_HIBYTE(w) ((unsigned char)(((unsigned short)(w) >> 8) & 0xFF))
#define APDU_HIWORD(d) ((unsigned short)((((unsigned long)(d)) >> 16) & 0xFFFF))
#define APDU_LOBYTE(w) ((unsigned char)(w))
#define APDU_LOWORD(d) ((unsigned short)(d))
#define APDU_MAKEWORD(lb, hb) ((unsigned short)(((unsigned char)(lb)) | (((unsigned short)((unsigned char)(hb))) << 8)))
#define APDU_MAKEDWORD(lw, hw) \
    ((unsigned long)(((unsigned short)(lw)) | (((unsigned long)((unsigned short)(hw))) << 16)))

#ifndef NULL
#define NULL (0)
#endif

#endif /* __APDU_GLOBAL_INT_H__ */
