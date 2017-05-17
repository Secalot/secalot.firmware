/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SF_CCID_GLOBAL_H__
#define __SF_CCID_GLOBAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CCID_HIBYTE(w) ((unsigned char)(((unsigned short)(w) >> 8) & 0xFF))
#define CCID_HIWORD(d) ((unsigned short)((((unsigned long)(d)) >> 16) & 0xFFFF))
#define CCID_LOBYTE(w) ((unsigned char)(w))
#define CCID_LOWORD(d) ((unsigned short)(d))
#define CCID_MAKEWORD(lb, hb) ((unsigned short)(((unsigned char)(lb)) | (((unsigned short)((unsigned char)(hb))) << 8)))
#define CCID_MAKEDWORD(lw, hw) \
    ((unsigned long)(((unsigned short)(lw)) | (((unsigned long)((unsigned short)(hw))) << 16)))

#define CCID_MAX_APDU_DATA_SIZE (4096)
/* 4096 bytes of data + 9 byte CLA + INS + P1 + P2 + Le (max 3 bytes)+ Le (max 2 bytes) */
#define CCID_MAX_APDU_SIZE (CCID_MAX_APDU_DATA_SIZE + 9)
#define CCID_MESSAGE_HEADER_SIZE (10)
#define CCID_MAX_MESSAGE_LENGTH (CCID_MAX_APDU_SIZE + CCID_MESSAGE_HEADER_SIZE)
#define CCID_WTX_TIMEOUT_IN_MS (2000)

/* Change this if you are using a different USB endpoint packet size. */
#define CCID_MAX_PACKET_SIZE (64)

#ifdef __cplusplus
}
#endif

#endif /* __SF_CCID_GLOBAL_H__ */
