/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __MK82_FS_INT_H__
#define __MK82_FS_INT_H__

#include "opgpGlobal.h"
#include "opgpHal.h"
#include "sfGlobal.h"
#include "sfHal.h"
#include "otpGlobal.h"
#include "otpHal.h"
#include "btcGlobal.h"
#include "btcHal.h"
#include "ethGlobal.h"
#include "ethHal.h"

#include "mk82KeySafe.h"

#define MK82_FS_TOTAL_BLOCKS (MK82_FLASH_FILE_SYSTEM_SIZE / MK82_FLASH_PAGE_SIZE)
#define MK82_FS_PAGE_DATA_SIZE (246)
#define MK82_FS_PAGE_SPARE_SIZE (10)
#define MK82_FS_PAGES_PER_BLOCK (16)
#define MK82_FS_PAGE_SIZE (MK82_FS_PAGE_DATA_SIZE + MK82_FS_PAGE_SPARE_SIZE)
#define MK82_FS_BLOCK_SIZE (MK82_FS_PAGE_SIZE * MK82_FS_PAGES_PER_BLOCK)

#define MK82_FS_INTERNAL_INFO_PER_PAGE (4)

MK82_MAKE_PACKED(typedef struct)
{
    OPGP_HAL_NVM_COUNTERS opgpCounters;
    SF_HAL_NVM_COUNTERS sfCounters;
    OTP_HAL_NVM_COUNTERS otpCounters;
    BTC_HAL_NVM_COUNTERS btcCounters;
    ETH_HAL_NVM_COUNTERS ethCounters;

    uint8_t padding[MK82_FS_PAGE_DATA_SIZE - sizeof(OPGP_HAL_NVM_COUNTERS) - sizeof(SF_HAL_NVM_COUNTERS) -
                    sizeof(OTP_HAL_NVM_COUNTERS) - sizeof(BTC_HAL_NVM_COUNTERS) - sizeof(ETH_HAL_NVM_COUNTERS) -
                    MK82_FS_INTERNAL_INFO_PER_PAGE];
}
MK82_FS_COUNTERS;

MK82_MAKE_PACKED(typedef struct)
{
    OPGP_HAL_NVM_CERTIFICATES opgpCertificates;

    uint8_t padding[MK82_FS_PAGE_DATA_SIZE * (MK82_FS_PAGES_PER_BLOCK - 1) - sizeof(OPGP_HAL_NVM_CERTIFICATES) -
                    MK82_FS_INTERNAL_INFO_PER_PAGE * (MK82_FS_PAGES_PER_BLOCK - 1)];
}
MK82_FS_CERTIFICATES;

MK82_MAKE_PACKED(typedef struct)
{
    OPGP_HAL_NVM_KEYS opgpKeys;
    OTP_HAL_NVM_KEYS otpKeys;
    BTC_HAL_NVM_KEYS btcKeys;
    ETH_HAL_NVM_KEYS ethKeys;

    uint8_t padding[MK82_FS_PAGE_DATA_SIZE * (MK82_FS_PAGES_PER_BLOCK - 1) - sizeof(OPGP_HAL_NVM_KEYS) -
                    sizeof(OTP_HAL_NVM_KEYS) - sizeof(BTC_HAL_NVM_KEYS) - sizeof(ETH_HAL_NVM_KEYS) -
                    MK82_FS_INTERNAL_INFO_PER_PAGE * (MK82_FS_PAGES_PER_BLOCK - 1)];
}
MK82_FS_KEYS;

MK82_MAKE_PACKED(typedef struct)
{
    OPGP_HAL_NVM_DATA opgpData;
    KEYSAFE_NVM_DATA keysafeData;
    OTP_HAL_NVM_DATA otpData;
    BTC_HAL_NVM_DATA btcData;
    ETH_HAL_NVM_DATA ethData;

    uint8_t padding[MK82_FS_PAGE_DATA_SIZE * (MK82_FS_PAGES_PER_BLOCK - 1) - sizeof(OPGP_HAL_NVM_DATA) -
                    sizeof(KEYSAFE_NVM_DATA) - sizeof(OTP_HAL_NVM_DATA) - sizeof(BTC_HAL_NVM_DATA) -
                    sizeof(ETH_HAL_NVM_DATA) - MK82_FS_INTERNAL_INFO_PER_PAGE * (MK82_FS_PAGES_PER_BLOCK - 1)];
}
MK82_FS_DATA;

#endif /* __MK82_FS_INT_H__ */
