/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __MK82_FS_H__
#define __MK82_FS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MK82_FS_FILE_ID_OPGP_COUNTERS (1)
#define MK82_FS_FILE_ID_OPGP_CERTIFICATES (2)
#define MK82_FS_FILE_ID_OPGP_KEYS (3)
#define MK82_FS_FILE_ID_OPGP_DATA (4)
#define MK82_FS_FILE_ID_SF_COUNTERS (5)
#define MK82_FS_FILE_ID_KEYSAFE_DATA (6)
#define MK82_FS_FILE_ID_OTP_COUNTERS (7)
#define MK82_FS_FILE_ID_OTP_KEYS (8)
#define MK82_FS_FILE_ID_OTP_DATA (9)
#define MK82_FS_FILE_ID_BTC_COUNTERS (10)
#define MK82_FS_FILE_ID_BTC_KEYS (11)
#define MK82_FS_FILE_ID_BTC_DATA (12)
#define MK82_FS_FILE_ID_ETH_COUNTERS (13)
#define MK82_FS_FILE_ID_ETH_KEYS (14)
#define MK82_FS_FILE_ID_ETH_DATA (15)
#define MK82_FS_FILE_ID_SYM_KEYS (16)
#define MK82_FS_FILE_ID_SYM_DATA (17)

void mk82FsInit(void);
void mk82FsReadFile(uint8_t fileID, uint32_t offset, uint8_t* buffer, uint32_t length);
void mk82FsWriteFile(uint8_t fileID, uint32_t offset, uint8_t* buffer, uint32_t length);
void mk82FsCommitWrite(uint8_t fileID);

#ifdef __cplusplus
}
#endif

#endif /* __MK82_FS_H__ */
